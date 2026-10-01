---
id: 33
title: The product had no whole-run execution and image ledger, so a run that died reported nothing
status: framework-change-pending
symptom: docs/issues/0028 and 0029 both record the same missing thing — a run that reached an
  authenticated image and then refused "did not print translated/fallback denominators", because
  nothing reported them at run end and the process ended on std::abort()
state_items: S003
tags: diagnostics,ledger,dynarec,image-publication,framework-change
created: 2026-09-30
updated: 2026-09-30
---

## What was missing, and why it was not "add a log line"

Three separate facts, and only the first is obvious:

1. **No run-end report at all.** The only whole-run numbers this product produced were the guest-call
   census (`reportGuestCallCensus`) and whatever the framework happened to print. Translated blocks,
   cache hits and misses, invalidations, budget exits and interpreter fallbacks by reason existed
   only as `psx::cpu::ExecutorCounters` fields, reachable live over the debug endpoint's `guest`
   command (`runtime/psx/dbg_server.cpp:620`) and never at the end of a run. A run that ended before
   anyone could ask therefore reported nothing — which is every run that failed.
2. **Nothing counted image publication as a run-level fact.** Every load logged its own
   `authenticated … image generation N` line at info level, so a long attract run produced 21 such
   lines and no census: no denominator of how many matched loads were offered, none of how many were
   refused, and no statement of what each publication cost the block cache.
3. **The framework's own shutdown telemetry does not cover a refusal.** `~LightrecExecutor` emits
   `Lightrec fallback telemetry [shutdown]` and `native_boot.cpp`'s comment says the store-observer
   report "is emitted … on every exit path". `std::abort()` does not unwind and does not run
   destructors, so on every refusal path in this title — which is a bare `std::abort()` after a
   `lucent::error` — that telemetry is absent. The comment is true for `return`/`exit` and false for
   `abort`, and the measured runs in issues 0028/0029 are the counterexamples.

## The owner

`game/diagnostics/run_ledger.{h,cpp}`, built as its own library `crashbash_run_ledger` because both
the execution seam and the image-publication owner feed it. It:

- **consumes** psxport's `ExecutorCounters` for every execution number (translated and executed
  blocks, executed instructions, cache hits/misses, invalidations, budget exits and their PC
  classification, host dispatches, faults, and all five fallback reasons plus all five refused
  reasons). It counts none of them itself.
- **records** the runtime's `invalidations` and `translatedBlocks` DELTA across each authenticated
  publication, so "this load invalidated the replaced range" is the runtime's number sampled either
  side of the activation rather than an assertion.
- **census** the CD publication boundary: every load that MATCHED a tracked specification is an
  offer; each offer must end as a publication or a recorded refusal, and a run that leaves one
  unaccounted is reported as `N load(s) reached guest RAM with NO authenticated image identity` —
  the state the executor would otherwise refuse 300 frames later as a typed fault.
- **is emitted on a refusal**: `refuseRun` is the product's one refusal route; it reports the ledger
  and then aborts. The title's execution-boundary refusals (no guest progress, past the turn cap, a
  resumed original whose image generation is gone, a guest call that did not return) all route
  through it, and the report names the outcome (`REFUSED`) and the reason.

## The measured blind spot: an ADMITTED fallback has no port-visible site

`Impl::fallbackAdmission` (`runtime/cpu/lightrec_executor.cpp:277`) receives
`lightrec_fallback_event{guest_pc, reason}` for every fallback, admitted or refused. It stores
`fallbackRefusalPc` **only on the refusal branch** and the struct is private to the `Impl`; nothing
in `lightrec_executor.h` exposes it. An admitted fallback is admitted inside one
`lightrec_execute` call and returns an ordinary result, so no port can see its PC at all. The
ledger reports the reason COUNTS for the whole set (framework counters) and the refusal SITES the
runtime does surface (recognised by the runtime's own fault text, `Lightrec fallback refused before
interpreter execution: reason=…`), and says plainly which half it cannot see.

## Required framework changes (exact)

1. **`runtime/cpu/lightrec_executor.h` — publish fallback sites.** Add to `ExecutorCounters`:

   ```cpp
   struct FallbackSite {
     std::uint32_t guestPc = 0;
     psx::cpu::InterpreterFallbackReason reason = InterpreterFallbackReason::UnsupportedBlock;
     std::uint64_t admitted = 0;
     std::uint64_t refused = 0;
   };
   inline constexpr std::size_t kMaxFallbackSites = 16;
   std::array<FallbackSite, kMaxFallbackSites> fallbackSites{};
   std::size_t fallbackSiteCount = 0;
   std::uint64_t fallbackSitesDropped = 0;   // sites past the cap: the denominator for "every site"
   ```

   filled in `Impl::fallbackAdmission` for BOTH return paths (`event->guest_pc`, `event->reason`,
   incrementing `admitted` or `refused`), and printed by `reportFallbackTelemetry`. Without the
   dropped counter, a bounded site list reads as a complete one.
2. **`runtime/cpu/native_dispatch.{h,cpp}` — publish override hits.** `NativeDispatcher::invoke` has
   no census, so "overrides hit" is not answerable by any port: `ExecutorCounters` counts host
   dispatches, which include BIOS/HLE. Needed: `struct NativeInvocationCounters { invocations,
   suppressed, unresolved; }` plus a bounded per-name table, incremented in `invoke` and readable
   through `core.nativeDispatcher()`.
3. **`runtime/psx/native_boot.cpp`, `runtime/cpu/native_dispatch.cpp`, `runtime/cpu/lightrec_executor.cpp`,
   `runtime/cpu/execution_exit.cpp` — a port-installable refusal hook.** Every framework `std::abort()`
   and `exit()` on a refusal path must call an installed hook first, so a port's run-end report runs
   on those paths too. Suggested: `runtime/psx/run_refusal.h` with
   `using RunRefusal = void (*)(std::string_view why, const Core &) noexcept;` and
   `void install_run_refusal(RunRefusal) noexcept;`, default `nullptr` and therefore behaviour-neutral
   for every current port. The call sites are `crt0_setup`'s two `exit(1)`s, `Impl::activeBoundary`'s
   `std::abort()`, `resolveHostDispatch`'s fault handling, and the `requireGuestReturn` failure log.
   This title's own refusals are already routed; the framework's are not, and that is the remaining
   half of "abnormal termination".

## Test

`tests/test_run_ledger.cpp`, registered as `crashbash_run_ledger`, seven hermetic cases plus one
image-backed case. Both classes are covered for every census: a run that executed nothing must say
it measured nothing (not print zeros quietly); a fault the runtime did not label a fallback refusal
must not count as one; a matched load left unpublished must be reported as an identity gap, and the
same loads published or refused must not; a refused run must emit the whole ledger; a second report
for one armed run must be refused; arming a second Core must throw. The image-backed case publishes
real BOOT bytes, then republishes corrupt bytes and shows the offer/publication/refusal censuses
diverging without any of them being wrong. A ninth case drives the real `loadResidentImage` refusal
routes: a wrong-SIZE and a wrong-SHA executable are two separate refusals, and neither is reported as
an unpublished module load, because bytes that never entered guest RAM are not an identity gap.

Gate: `tools/verify.py` — **32/32 tests passed, 0 failed**; the ledger binary reports **8/8, 48
checks** hermetic and **8/8, 52 checks** with `scratch/bin/crashbash/overlays/BOOT.BIN` passed to
it (both exit 0). Project state S003 carries the measured 1,500-frame run.

## MEASURED — `counters.invalidations` counts REQUESTS, not invalidated blocks

The first report line this owner wrote said "invalidation(s) reached the block cache" as if the
number were blocks. It is not, and the error was worth 4.5 orders of magnitude:

- `LightrecExecutor::counters.invalidations` is incremented once per **call** to `invalidate` and
  `invalidateAll` — `psxport/runtime/cpu/lightrec_executor.cpp:890,897`.
- `Core::writeGuestMemory` issues one `notifyExecutableWrite` per mapped guest **store** —
  `psxport/runtime/psx/guest_memory.cpp:101`.

So the 1,500-frame run's 32,210,048 is ~1 request per 7 executed instructions, against 6,900 blocks
translated in the whole run and 14,670,173 block executions. The report now prints "invalidation
REQUEST(s)" and names both producing lines, and the per-publication deltas say the runtime "issued
invalidation request(s)" rather than "invalidated translated block(s)". `ExecutorCounters` has no
field for blocks actually dropped from the cache; adding one belongs with the framework changes
above, not with a title-side rename.

A second measurement worth keeping: **0 interpreter fallbacks in 119,321 executor calls** is only
meaningful beside the reason census, so all five reasons and all five refused reasons print as zeros
and the reader can see the census is populated rather than uninitialised.

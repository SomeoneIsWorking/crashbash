// Crash Bash's measured framework compatibility facts. Unmeasured address groups stay zero by
// omission. CrashBashRuntime owns behavior; this table exists only while generic psxport algorithms
// still read Core::cfg.
#include "crashbash_guest.h"
#include "game_iface.h"
#include "legacy_game_interface.h"

#ifdef CRASHBASH_HAVE_SUBSTRATE
#include "overlay_table.h"
#endif

// PS-X EXE header, provisioned retail SCUS_945.70.
static constexpr uint32_t kPsExeEntry = 0x8002E7B0u;
static constexpr uint32_t kPsExeTextAddress = 0x80010000u;
static constexpr uint32_t kPsExeTextSize = 0x00069000u;
static constexpr uint32_t kRecMainLo = kPsExeTextAddress & 0x1FFFFFFFu;
static constexpr uint32_t kRecMainHi = (kPsExeTextAddress + kPsExeTextSize) & 0x1FFFFFFFu;

// The shared crt0 extractor derives the complete group below from the retail entry.  Ghidra's entry
// decompile independently identifies the second jal as guest main 0x8002718C.  tools/recomp_bootstrap.py
// compares every shipping constant and field binding to those executable measurements.
static constexpr uint32_t kCrt0BssZeroLo = 0x8006E9F0u;
static constexpr uint32_t kCrt0BssZeroHi = 0x80078C90u;
static constexpr uint32_t kCrt0StackTopBase = 0x8002E860u;
static constexpr uint32_t kCrt0StackTopBase2 = 0x8006D8B4u;
static constexpr uint32_t kCrt0HeapBase = 0x80078C90u;
static constexpr uint32_t kCrt0HeapSizePtr = 0u; // measured absent: value remains in a register
static constexpr uint32_t kCrt0HeapBasePtr = 0u; // measured absent: value remains in a register
static constexpr uint32_t kCrt0Gp = 0x8006E9ECu;
static constexpr uint32_t kCrt0LibcInit = 0x8003ACCCu;
static constexpr uint32_t kCrt0GameMain = 0x8002718Cu;
static constexpr uint32_t kCrt0Entry = 0x8002E7B0u;
static constexpr int32_t kCrt0StackBias = 0;

// The renderer allocates two packet pools from the guest heap. Setup functions 0x800274FC and
// 0x800276C4 write each allocation's inclusive base and exclusive end to these globals; the current
// packet pointer lives in the following word. These are descriptor addresses, not one observed heap
// allocation, so attribution remains correct when allocation order moves the live pools.
static constexpr uint32_t kPacketPoolBasePtrs[2] = {0x8005F790u, 0x8006379Cu};
static constexpr uint32_t kPacketPoolEndPtrs[2] = {0x8005F794u, 0x800637A0u};

static_assert(kPsExeEntry == kCrt0Entry);
static_assert(kCrt0BssZeroHi == kCrt0HeapBase);
static_assert(kCrt0Gp + 4u == kCrt0BssZeroLo);
static_assert(kRecMainLo == 0x00010000u && kRecMainHi == 0x00079000u);
#ifdef CRASHBASH_HAVE_SUBSTRATE
static_assert(kRecMainLo == REC_MAIN_LO && kRecMainHi == REC_MAIN_HI,
              "Crash Bash routing range drifted from the emitted substrate");
#endif

static const GameConfig kCrashBashConfig = {
    .bssZeroLo = kCrt0BssZeroLo,
    .bssZeroHi = kCrt0BssZeroHi,
    .stackTopBase = kCrt0StackTopBase,
    .stackTopBase2 = kCrt0StackTopBase2,
    .heapBase = kCrt0HeapBase,
    .heapSizePtr = kCrt0HeapSizePtr,
    .heapBasePtr = kCrt0HeapBasePtr,
    .gp = kCrt0Gp,
    .libcInit = kCrt0LibcInit,
    .gameMain = kCrt0GameMain,
    .crt0 = kCrt0Entry,
    .recMainLo = kRecMainLo,
    .recMainHi = kRecMainHi,
    .discEnvVar = "PSXPORT_CRASHBASH_DISC",
    .cdCommand = crashbash::guest::kCdCommand,
    .cdSync = crashbash::guest::kCdSync,
    .cdSearchFile = crashbash::guest::kCdSearchFile,
    .hle =
        {
            .windowLo = {crashbash::guest::kVSync.begin, crashbash::guest::kCdInitHandshake},
            .windowHi = {crashbash::guest::kVSync.end, crashbash::guest::kCdCommand + 4u},
            .vsyncTrap = crashbash::guest::kVSync.begin,
        },
    // The current title path still uses guest VRAM as a visible producer: the SCEA boot screen is
    // an upload-only image and later mixed native/guest frames retain their guest backdrop.  The
    // renderer must rebuild from those writes rather than clearing them as if native producers
    // owned every pixel.  Replace this static compatibility input with a title runtime policy once
    // the whole picture has one native owner.
    .preserveVramBackdrop = 1u,
    .windowTitle = "Crash Bash",
    .stackBias = {1, kCrt0StackBias},
    .packetPoolBasePtrs = {kPacketPoolBasePtrs[0], kPacketPoolBasePtrs[1]},
    .packetPoolEndPtrs = {kPacketPoolEndPtrs[0], kPacketPoolEndPtrs[1]},
};

const GameConfig &crashbash::legacy::measuredConfig = kCrashBashConfig;

// Crash Bash player entry: install the title runtime, authenticate the user's executable, and
// hand one fully wired Game to psxport's native boot/frame loop.

#include "c_subsys.h"
#include "core.h"
#include "game.h"
#include "hw_bind.h"
#include "title_adapter.h"

#include <lucent/log.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

// The framework owns the host loop and exposes this one product boundary from native_boot.cpp.
void native_boot_run(Core *core);

namespace {

std::vector<std::uint8_t> readExecutable(const std::filesystem::path &path, std::string &error) {
  std::error_code filesystemError;
  if (!std::filesystem::is_regular_file(path, filesystemError)) {
    error = "Crash Bash executable is not a regular file: " + path.string();
    return {};
  }
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    error = "cannot open Crash Bash executable: " + path.string();
    return {};
  }
  std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  if (!input.eof() && input.fail()) {
    error = "cannot read Crash Bash executable completely: " + path.string();
    return {};
  }
  return bytes;
}

} // namespace

int main(int argc, char **argv) {
  static crashbash::TitleAdapter runtime;
  psxport_install_game(runtime);

  const std::filesystem::path executable =
      argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::path("scratch/bin/crashbash/SCUS_945.70");
  std::string error;
  std::vector<std::uint8_t> bytes = readExecutable(executable, error);
  if (bytes.empty()) {
    lucent::error("boot", "{}", error);
    return 2;
  }

  // Game owns the complete PSX machine, including 2 MiB of guest RAM and renderer state; keep it
  // on the heap so the entry stack remains bounded on hosts with the usual 8 MiB thread stack.
  auto game = std::make_unique<Game>();
  Core &core = game->core;
  const auto loaded = runtime.loadExecutable(core, bytes);
  if (!loaded) {
    lucent::error("boot", "cannot authenticate Crash Bash executable '{}': {}", executable.string(), loaded.detail);
    return 2;
  }

  // These binders are per-Core and must be ready before crt0 or any title callback can execute.
  gte_init();
  gte_bind(&core);
  core.rsub.projprim.bind(&core);
  spu_bind(&core);
  mdec_bind(&core);
  xa_bind(&core);
  game->spu_audio.init();
  game->gpu.gpu_native_init();
  game->pad.overridesInit();
  runtime.registerOverrides(*game);

  watchdog_init();
  native_boot_run(&core);
  return 0;
}

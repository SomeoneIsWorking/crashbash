// Desktop entry composition; crashbash_application owns the title lifecycle shared with Android.
#include "crashbash_application.h"

#include <filesystem>

static constexpr const char *kDefaultExecutable = "scratch/bin/crashbash/SCUS_945.70";

int main(int argc, char **argv) {
  const auto executablePath = argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::path(kDefaultExecutable);
  return crashbash::runApplication(executablePath);
}

#pragma once

#include <filesystem>

namespace crashbash {

// Runs one Crash Bash process against a verified, provisioned retail executable. Platform entries
// supply the executable location; this owner installs the title seam and drives native boot.
[[nodiscard]] int runApplication(const std::filesystem::path &executablePath);

} // namespace crashbash

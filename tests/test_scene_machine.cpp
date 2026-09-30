// WHY THIS TEST EXISTS.
//
// The product was reported as "never leaving process state 0x8004E0B8", with the app-mode pointer
// `kAppModeVtable` (0x8004E0DC) as the evidence. That word cannot carry that claim, and this test
// derives WHY from the authenticated bytes instead of from a comment:
//
//   * exactly ONE instruction in the resident text stores to 0x8004E0DC, and it is 0x800101CC in the
//     resident application main — a write that happens before any mode exists and installs BOOT's
//     own load address as the shell's permanent root scene;
//   * the scene machine that DOES select boot / menu / attract / gameplay is 0x8009F658, and BOOT's
//     own update (0x80092BA0) is the code that passes it to the retail dispatcher 0x8001E610
//     together with the transition clock at 0x8009F644.
//
// So the old observer was a dead tap: it cannot distinguish "reached Crashball" from "never left the
// logo", and both read the same. The census below is a CENSUS, not a spot check — it resolves the
// `lui` + displacement form that a literal-word scan cannot see, over every word of both images, and
// it asserts the exact site count so a future second writer (which would make the word meaningful
// again) fails here instead of silently invalidating the reasoning in the issue.

#include "boot_image_identity.h"
#include "crashbash_guest.h"
#include "testutil.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::string residentPath;
std::string bootPath;

constexpr std::uint32_t kOpcodeLoad = 0x23u;
constexpr std::uint32_t kOpcodeStore = 0x2Bu;
// `lui $rt, imm` names rt in bits 20..16 and the immediate in bits 15..0. Reading rt out of
// the low bits of the word is the mistake that makes this census silently return 0 for every
// address, which is indistinguishable from "this word really has one writer".
constexpr std::uint32_t kOpcodeLui = 0x0Fu;
constexpr std::uint32_t kOpcodeAddiu = 0x09u;
constexpr std::uint32_t kOpcodeJal = 0x03u;
// How far back a `lui` may sit and still be the base of a load/store. Retail compiles
// `lui $rX, hi` immediately before the access it feeds, with at most an `addiu` between.
constexpr std::uint32_t kLuiLookback = 8u;

struct Image {
  std::string name;
  std::uint32_t textBase = 0;
  std::vector<std::uint32_t> words;
};

std::uint32_t word(const Image &image, std::uint32_t address) {
  const std::uint32_t index = (address - image.textBase) / 4u;
  return index < image.words.size() ? image.words[index] : 0u;
}

std::uint32_t signedImmediate(std::uint32_t value) {
  return (value & 0x8000u) != 0u ? (value | 0xFFFF0000u) : (value & 0xFFFFu);
}

std::uint32_t jumpTarget(std::uint32_t instruction, std::uint32_t pc) {
  const std::uint32_t index = instruction & 0x03FFFFFFu;
  return (pc & 0xF0000000u) | ((index << 2u) & 0x0FFFFFFFu);
}

// One resolved access to `target`: an `lw`/`sw` whose BASE register carries an upper half a `lui`
// established, and whose displacement completes the address. This is the form a literal 32-bit
// address never appears in, which is why the census is written this way.
struct Site {
  std::uint32_t address;
  bool store;
};

std::vector<Site> censusAccesses(const Image &image, std::uint32_t target) {
  std::vector<Site> sites;
  for (std::size_t index = 0; index < image.words.size(); ++index) {
    const std::uint32_t instruction = image.words[index];
    const std::uint32_t opcode = instruction >> 26u;
    if (opcode != kOpcodeLoad && opcode != kOpcodeStore) {
      continue;
    }
    const std::uint32_t base = (instruction >> 21u) & 0x1Fu;
    for (std::size_t back = 1; back <= kLuiLookback && back <= index; ++back) {
      const std::uint32_t candidate = image.words[index - back];
      if ((candidate >> 26u) != kOpcodeLui || ((candidate >> 16u) & 0x1Fu) != base) {
        continue;
      }
      std::uint32_t resolved = (candidate & 0xFFFFu) << 16u;
      // An intervening `addiu` that materialises the full address leaves the access's own
      // displacement at zero, so the completed address has to be read off the pair. Only the
      // instructions strictly BETWEEN the lui and the access are considered.
      for (std::size_t step = index - back + 1u; step < index; ++step) {
        const std::uint32_t middle = image.words[step];
        if ((middle >> 26u) == kOpcodeAddiu && ((middle >> 16u) & 0x1Fu) == base) {
          resolved += signedImmediate(middle & 0xFFFFu);
          break;
        }
      }
      if (resolved + signedImmediate(instruction & 0xFFFFu) == target) {
        sites.push_back({image.textBase + static_cast<std::uint32_t>(index) * 4u, opcode == kOpcodeStore});
      }
      break;
    }
  }
  return sites;
}

std::vector<std::uint8_t> readBytes(const std::string &path, std::size_t expectedSize) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input || input.tellg() != static_cast<std::streamsize>(expectedSize)) {
    return {};
  }
  std::vector<std::uint8_t> bytes(expectedSize);
  input.seekg(0);
  if (!input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) {
    return {};
  }
  return bytes;
}

// PS-X EXE: 0x800 header bytes, then the text at `t_addr`. Built the way the loader builds it,
// because mapping the file from its own start lands every text address 0xF800 low.
std::uint32_t readLittleWord(const std::vector<std::uint8_t> &bytes, std::size_t at) {
  return static_cast<std::uint32_t>(bytes[at]) | (static_cast<std::uint32_t>(bytes[at + 1]) << 8u) |
         (static_cast<std::uint32_t>(bytes[at + 2]) << 16u) | (static_cast<std::uint32_t>(bytes[at + 3]) << 24u);
}

bool loadResidentImage(const std::string &path, Image &image) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    return false;
  }
  const auto size = static_cast<std::size_t>(input.tellg());
  std::vector<std::uint8_t> bytes(size);
  input.seekg(0);
  if (!input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(size))) {
    return false;
  }
  if (size < 0x800u + 8u || std::string(bytes.begin(), bytes.begin() + 8) != "PS-X EXE") {
    return false;
  }
  // The header words live at file offset 0x10 (pc0, gp0, t_addr, t_size) — the 0x800 is where the
  // TEXT starts, not where the header starts. Reading t_addr/t_size at 0x800+0x18 takes the first
  // two text instructions instead and produces a text length of ~2.3e9, which walks off the buffer.
  const std::uint32_t textAddress = readLittleWord(bytes, 0x18);
  const std::uint32_t textBytes = readLittleWord(bytes, 0x1Cu);
  // The declared text span has to FIT the file. A length that does not is a header read from the
  // wrong offset, and pushing that many words is how the first version of this test segfaulted.
  if (textBytes == 0u || 0x800u + textBytes > bytes.size()) {
    return false;
  }
  image = Image{.name = "resident", .textBase = textAddress};
  for (std::size_t offset = 0; offset + 4u <= textBytes; offset += 4u) {
    image.words.push_back(readLittleWord(bytes, 0x800u + offset));
  }
  return !image.words.empty();
}

bool loadRawModule(const std::string &path, std::uint32_t base, Image &image) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) {
    return false;
  }
  const auto size = static_cast<std::size_t>(input.tellg());
  std::vector<std::uint8_t> bytes(size);
  input.seekg(0);
  if (!input.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(size))) {
    return false;
  }
  image = Image{.name = "boot", .textBase = base};
  for (std::size_t offset = 0; offset + 4u <= size; offset += 4u) {
    image.words.push_back(static_cast<std::uint32_t>(bytes[offset]) |
                          (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
                          (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
                          (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u));
  }
  return !image.words.empty();
}

// THE DEAD TAP, as a fact over both images. One store site, at the resident application main.
void test_app_mode_pointer_has_exactly_one_writer() {
  Image resident;
  Image boot;
  if (!loadResidentImage(residentPath, resident) ||
      !loadRawModule(bootPath, crashbash::boot_image::kLoadAddress, boot)) {
    std::fprintf(stderr, "no usable authenticated images at %s / %s\n", residentPath.c_str(), bootPath.c_str());
    std::exit(2);
  }
  CHECK(resident.words.size() > 100000u);
  CHECK(boot.words.size() > 90000u);

  std::vector<Site> stores;
  for (const Image *image : {&resident, &boot}) {
    for (const Site &site : censusAccesses(*image, crashbash::guest::kAppModeVtable)) {
      if (site.store) {
        stores.push_back(site);
      }
    }
  }
  CHECK_EQ(stores.size(), std::size_t{1});
  if (stores.size() == 1u) {
    CHECK_EQ(stores.front().address, 0x800101CCu);
    // `sw $s0, -0x1f24($v0)` with $v0 = 0x80050000: the store's own immediate, decoded.
    const std::uint32_t instruction = word(resident, 0x800101CCu);
    CHECK_EQ(instruction >> 26u, kOpcodeStore);
    CHECK_EQ(static_cast<std::int32_t>(static_cast<std::int16_t>(instruction & 0xFFFFu)), -0x1F24);
  }

  // NEGATIVE CASE, and the one that would invalidate the reasoning above: the census must be able to
  // return a NON-trivial answer. The same scan over the scene machine the title now observes finds
  // three accesses in BOOT, two of them stores, one of them the retail enter clearing the current
  // scene (`sw $zero, -0x9a8($v0)` in the delay slot of `jal 0x8001E934` at 0x80092BF0). So a
  // single-writer result on `kAppModeVtable` is a property of that word and not of a scan that cannot
  // see anything.
  const std::vector<Site> sceneStores = censusAccesses(boot, crashbash::guest::kSceneTransition);
  CHECK_EQ(sceneStores.size(), std::size_t{3});
  std::uint32_t sceneStoreCount = 0;
  bool sawEnterClear = false;
  for (const Site &site : sceneStores) {
    sceneStoreCount += site.store ? 1u : 0u;
    sawEnterClear = sawEnterClear || (site.store && site.address == 0x80092BF4u);
  }
  CHECK_EQ(sceneStoreCount, std::uint32_t{2});
  CHECK(sawEnterClear);
}

// THE MACHINE THAT DOES SELECT MODES, pinned to BOOT's own bytes rather than to a note. BOOT's
// update at 0x80092BA0 materialises the clock and the scene record and hands both to 0x8001E610.
void test_boot_update_passes_the_scene_record_to_the_retail_dispatcher() {
  Image boot;
  if (!loadRawModule(bootPath, crashbash::boot_image::kLoadAddress, boot)) {
    std::fprintf(stderr, "no usable authenticated BOOT image at %s\n", bootPath.c_str());
    std::exit(2);
  }
  // lui $s0, 0x800A ; addiu $s0, $s0, -0x9BC   -> the transition clock
  CHECK_EQ((word(boot, 0x80092BA8u) >> 26u), kOpcodeLui);
  CHECK_EQ((word(boot, 0x80092BACu) >> 26u), kOpcodeAddiu);
  CHECK_EQ(0x800A0000u + signedImmediate(word(boot, 0x80092BACu) & 0xFFFFu), crashbash::guest::kSceneTransitionClock);
  // lui $a0, 0x800A ; addiu $a0, $a0, -0x9A8   -> the scene record, the first argument
  CHECK_EQ((word(boot, 0x80092BBCu) >> 26u), kOpcodeLui);
  CHECK_EQ((word(boot, 0x80092BC0u) >> 26u), kOpcodeAddiu);
  CHECK_EQ(0x800A0000u + signedImmediate(word(boot, 0x80092BC0u) & 0xFFFFu), crashbash::guest::kSceneTransition);
  // jal 0x8001E610, with $a1 = the clock.
  CHECK_EQ((word(boot, 0x80092BC4u) >> 26u), kOpcodeJal);
  CHECK_EQ(jumpTarget(word(boot, 0x80092BC4u), 0x80092BC4u), crashbash::guest::kSceneTransitionRequest + 0x88u);
  // NEGATIVE: the scene record and the clock are ADJACENT, four bytes apart, so a test that only
  // checked "some 0x8009F6xx address" would pass for the wrong one. They must differ by exactly
  // one word and neither may equal the root app scene pointer.
  CHECK_EQ(crashbash::guest::kSceneTransitionClock + 0x14u, crashbash::guest::kSceneTransition);
  CHECK(crashbash::guest::kSceneTransition != crashbash::guest::kAppModeVtable);
}

// The harness has no argument-taking registration, and these two cases are the whole point of the
// file, so they are always run — a case that only runs when someone remembers a flag is a case that
// silently stops gating. The images are PROVISIONED INPUTS (argv, for the reason
// test_nested_module_image.cpp states) and a missing one is a refusal, never a pass.
} // namespace

int main(int argc, char **argv) {
  if (argc != 3) {
    std::fprintf(stderr,
                 "REFUSED: this test reads the AUTHENTICATED resident executable and BOOT payload, so "
                 "it gates nothing without them.\n"
                 "        provision them (tools/provision.py) and re-run: %s <SCUS_945.70> <BOOT.BIN>\n",
                 argv[0]);
    return 2;
  }
  residentPath = argv[1];
  bootPath = argv[2];
  RUN(app_mode_pointer_has_exactly_one_writer);
  RUN(boot_update_passes_the_scene_record_to_the_retail_dispatcher);
  return pt_summary();
}

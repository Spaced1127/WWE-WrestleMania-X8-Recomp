#include "frontend_config.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;
using moderngekko::frontend::ControllerLayout;

namespace {
std::string ReadFile(const fs::path &path) {
  std::ifstream input(path);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

bool Contains(const std::string &text, const std::string &needle) {
  return text.find(needle) != std::string::npos;
}
} // namespace

int main() {
  int failures = 0;
  const auto expect = [&](bool condition, const char *what) {
    if (!condition) {
      std::cerr << "FAIL: " << what << '\n';
      ++failures;
    }
  };

  expect(moderngekko::frontend::DetectControllerLayout(
             "DualSense Wireless Controller") == ControllerLayout::Standard,
         "DualSense uses the standard layout");
  expect(moderngekko::frontend::DetectControllerLayout("PS4 Controller") ==
             ControllerLayout::Standard,
         "DualShock 4 uses the standard layout");
  expect(moderngekko::frontend::DetectControllerLayout(
             "Xbox 360 Controller") == ControllerLayout::Standard,
         "Xbox 360 uses the standard layout");
  expect(moderngekko::frontend::DetectControllerLayout(
             "Steam Deck Controller") == ControllerLayout::Standard,
         "Steam Deck uses the standard layout");

  // Real SDL3 GameControllerDB rows (vendor/dolphin Externals SDL
  // SDL_gamepad_db.h), fed through the same call the launcher and runner make.
  struct LayoutCase {
    const char *device;
    const char *mapping;
    ControllerLayout layout;
    bool dpad_swap;
  };
  const LayoutCase matrix[] = {
      {"SDL/0/X360 Controller",
       "030000005e0400008e02000000000000,X360 Controller,a:b0,b:b1,"
       "dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,leftx:a0,lefty:a1,x:b2,y:b3,",
       ControllerLayout::Standard, false},
      {"SDL/0/Xbox 360 Controller",
       "030000005e0400008e02000010010000,Xbox 360 Controller,a:b0,b:b1,"
       "dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftx:a0,lefty:a1~,"
       "x:b2,y:b3,",
       ControllerLayout::Standard, false},
      {"SDL/0/Xbox One Wired Controller",
       "030000005e040000e302000000000000,Xbox One Wired Controller,a:b0,b:b1,"
       "dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,leftx:a0,lefty:a1,x:b2,y:b3,",
       ControllerLayout::Standard, false},
      {"SDL/0/Xbox Wireless Controller",
       "030000005e040000ea02000000000000,Xbox Wireless Controller,a:b0,b:b1,"
       "dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,leftx:a0,lefty:a1,x:b2,y:b3,",
       ControllerLayout::Standard, false},
      {"SDL/0/Xbox One S Controller",
       "050000005e040000e002000000783f00,Xbox One S Controller,a:b0,b:b1,"
       "dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,leftx:a0,lefty:a1,x:b2,y:b3,",
       ControllerLayout::Standard, false},
      {"SDL/0/Xbox Series X Controller",
       "050000005e040000120b000000783f80,Xbox Series X Controller,a:b0,b:b1,"
       "misc1:b15,dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,leftx:a0,lefty:a1,"
       "x:b2,y:b3,",
       ControllerLayout::Standard, false},
      {"SDL/0/Xbox Series X Controller (BT)",
       "050000005e040000130b0000ffff3f00,Xbox Series X Controller,a:b0,b:b1,"
       "misc1:b15,dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,leftx:a0,lefty:a1,"
       "x:b2,y:b3,",
       ControllerLayout::Standard, false},
      {"SDL/0/Xbox Elite Wireless Controller",
       "030000005e040000050b000003090000,Xbox Elite Wireless Controller,"
       "a:b0,b:b1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftx:a0,"
       "lefty:a1,x:b3,y:b4,",
       ControllerLayout::Standard, false},
      {"SDL/0/Xbox One Elite 2 Controller",
       "050000005e040000000b000000783f80,Xbox One Elite 2 Controller,a:b0,b:b1,"
       "dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,leftx:a0,lefty:a1,x:b2,y:b3,",
       ControllerLayout::Standard, false},
      {"SDL/0/Xbox Elite Series 2 (0B22)",
       "030000005e040000220b000000000000,Xbox Elite Series 2 Controller,"
       "a:b0,b:b1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftx:a0,"
       "lefty:a1,x:b3,y:b4,",
       ControllerLayout::Standard, false},
      {"SDL/0/PS5 Controller",
       "030000004c050000e60c000000000000,PS5 Controller,a:b0,b:b1,"
       "dpdown:b14,dpleft:b15,dpright:b13,dpup:b12,leftx:a0,lefty:a1,x:b2,y:b3,",
       ControllerLayout::Standard, false},
      {"SDL/0/DualSense Edge Wireless Controller",
       "030000004c050000f20d000000000000,DualSense Edge Wireless Controller,"
       "a:b0,b:b1,dpdown:b14,dpleft:b15,dpright:b13,dpup:b12,paddle1:b16,"
       "leftx:a0,lefty:a1,touchpad:b13,x:b2,y:b3,misc1:b20,",
       ControllerLayout::Standard, false},
      {"SDL/0/DualSense Edge Wireless Controller (BT)",
       "050000004c050000f20d000000010000,DualSense Edge Wireless Controller,"
       "a:b0,b:b1,dpdown:b14,dpleft:b15,dpright:b13,dpup:b12,leftx:a0,lefty:a1,"
       "x:b2,y:b3,",
       ControllerLayout::Standard, false},
      {"SDL/0/8BitDo Pro 2",
       "03000000c82d00000660000011010000,8BitDo Pro 2,a:b1,b:b0,x:b4,y:b3,"
       "dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,leftx:a0,lefty:a1,",
       ControllerLayout::Nintendo, false},
      {"SDL/0/8BitDo Pro 2 (Switch mode)",
       "05000000ac05000004000000fd216d04,8BitDo Pro 2,a:b3,b:b2,x:b8,y:b7,"
       "dpdown:b15,dpleft:b16,dpright:b17,dpup:b14,leftx:a0,lefty:a1,",
       ControllerLayout::Nintendo, false},
      {"SDL/0/8BitDo Ultimate Wireless Controller",
       "03000000c82d00001330000011010000,8BitDo Ultimate Wireless Controller,"
       "a:b0,b:b1,x:b3,y:b4,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,",
       ControllerLayout::Standard, false},
      {"SDL/0/8BitDo 64",
       "03000000c82d00001930000000000000,8BitDo 64 Bluetooth Controller,"
       "a:b0,b:b1,x:b3,y:b4,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,",
       ControllerLayout::Standard, false},
      {"SDL/0/8BitDo Micro gamepad",
       "03000000c82d00002090000000000000,8BitDo Micro gamepad,a:b1,b:b0,"
       "dpleft:h0.2,dpright:h0.8,dpdown:h0.4,dpup:h0.1,x:b3,y:b4,",
       ControllerLayout::Nintendo, true},
      {"SDL/0/Nintendo Switch Pro Controller",
       "030000007e0500000920000011000000,Nintendo Switch Pro Controller,"
       "a:b0,b:b1,dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,leftx:a0,lefty:a1,"
       "x:b3,y:b2,",
       ControllerLayout::Nintendo, false},
      // Non-8BitDo rows that use a:b1: must stay standard. The old blanket
      // a>b rule flipped 219 of the DB's 812 rows, these included.
      {"SDL/0/Steam Virtual Gamepad",
       "030000005e0400008e02000001000000,Steam Virtual Gamepad,a:b0,b:b1,"
       "dpdown:b12,dpleft:b13,dpright:b14,dpup:b11,",
       ControllerLayout::Standard, false},
      {"SDL/0/Betop Controller",
       "03000000bc2000006412000000000000,Betop Controller,a:b1,b:b0,"
       "dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,",
       ControllerLayout::Standard, false},
      {"SDL/0/Defender Joystick Cobra R4",
       "03000000790000000600000000000000,Defender Joystick Cobra R4,a:b1,b:b0,"
       "dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,",
       ControllerLayout::Standard, false},
  };
  for (const LayoutCase &test : matrix) {
    const auto detected = moderngekko::frontend::DetectControllerMapping(
        test.device, test.mapping);
    if (detected.layout != test.layout) {
      std::cerr << "FAIL: layout for " << test.device << '\n';
      ++failures;
    }
    if (detected.swap_dpad_lr != test.dpad_swap) {
      std::cerr << "FAIL: d-pad correction for " << test.device << '\n';
      ++failures;
    }
  }
  const auto pro2_mapping =
      "03000000c82d00000660000011010000,8BitDo Pro 2,a:b1,b:b0,"
      "dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,x:b4,y:b3";
  const auto ultimate_mapping =
      "03000000c82d00001330000011010000,8BitDo Ultimate Wireless Controller,"
      "a:b0,b:b1,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,x:b3,y:b4";
  const auto pro2_switch_mapping =
      "05000000ac05000004000000fd216d04,8BitDo Pro 2,a:b3,b:b2,"
      "dpleft:b10,dpright:b11,leftx:a0,lefty:a1~,x:b8,y:b7";
  const auto micro_reversed_dpad =
      "03000000c82d00002090000000000000,8BitDo Micro gamepad,a:b1,b:b0,"
      "dpleft:h0.2,dpright:h0.8";

  expect(moderngekko::frontend::DetectControllerLayout("8BitDo Pro 2") ==
             ControllerLayout::Standard,
         "name-only fallback cannot classify 8BitDo Pro 2");

  expect(moderngekko::frontend::DetectControllerMapping("8BitDo Pro 2",
                                                        pro2_mapping)
                 .layout == ControllerLayout::Nintendo,
         "8BitDo Pro 2 mapping uses the Nintendo layout");
  expect(moderngekko::frontend::DetectControllerMapping(
             "8BitDo Ultimate Wireless Controller", ultimate_mapping)
                 .layout == ControllerLayout::Standard,
         "8BitDo Ultimate mapping uses the standard layout");
  expect(moderngekko::frontend::DetectControllerMapping("8BitDo Pro 2",
                                                        pro2_switch_mapping)
                 .layout == ControllerLayout::Nintendo,
         "8BitDo Pro 2 Switch-mode mapping uses the Nintendo layout");
  const auto micro = moderngekko::frontend::DetectControllerMapping(
      "8BitDo Micro gamepad", micro_reversed_dpad);
  expect(micro.layout == ControllerLayout::Nintendo,
         "8BitDo Micro uses the Nintendo layout");
  expect(micro.swap_dpad_lr,
         "reversed 8BitDo D-pad mapping is normalized");
  expect(!moderngekko::frontend::DetectControllerMapping("8BitDo Pro 2",
                                                         pro2_mapping)
              .swap_dpad_lr,
         "standard 8BitDo D-pad mapping is left alone");

  const auto edge_mapping = "a:b1,b:b2,x:b0,y:b3";
  const auto xbox_mapping = "a:b0,b:b1,x:b2,y:b3";
  expect(moderngekko::frontend::DetectControllerMapping(
             "DualSense Edge Wireless Controller", edge_mapping,
             "050000004c050000f20d000000010000")
                 .layout == ControllerLayout::Standard,
         "DualSense Edge Bluetooth GUID is standard");
  expect(moderngekko::frontend::DetectControllerMapping(
             "DualSense Edge Wireless Controller", edge_mapping,
             "030000004c050000f20d000000000000")
                 .layout == ControllerLayout::Standard,
         "DualSense Edge USB GUID is standard");
  expect(moderngekko::frontend::DetectControllerMapping(
             "Xbox Elite Wireless Controller", xbox_mapping,
             "030000005e040000e302000000000000")
                 .layout == ControllerLayout::Standard,
         "Xbox Elite 02E3 GUID is standard");
  expect(moderngekko::frontend::DetectControllerMapping(
             "Xbox One Elite 2 Controller", xbox_mapping,
             "050000005e040000000b000000783f80")
                 .layout == ControllerLayout::Standard,
         "Xbox Elite 0B00 GUID is standard");
  expect(moderngekko::frontend::DetectControllerMapping(
             "Xbox Elite Wireless Controller", xbox_mapping,
             "030000005e040000050b000003090000")
                 .layout == ControllerLayout::Standard,
         "Xbox Elite 0B05 GUID is standard");
  expect(moderngekko::frontend::DetectControllerMapping(
             "Nintendo Switch Pro Controller", xbox_mapping,
             "030000005e040000e302000000000000")
                 .layout == ControllerLayout::Standard,
         "Elite GUID overrides a Nintendo-looking name");
  expect(moderngekko::frontend::DetectControllerMapping(
             "Nintendo Switch Pro Controller", edge_mapping,
             "050000004c050000f20d000000010000")
                 .layout == ControllerLayout::Standard,
         "Edge GUID overrides a Nintendo-looking name");
  expect(moderngekko::frontend::DetectControllerMapping(
             "PS5 Controller", edge_mapping,
             "030000004c050000e60c000000007700")
                 .layout == ControllerLayout::Standard,
         "standard DualSense remains standard");
  expect(moderngekko::frontend::DetectControllerMapping(
             "Xbox Series X Controller", xbox_mapping,
             "050000005e040000120b000000783f80")
                 .layout == ControllerLayout::Standard,
         "standard Xbox remains standard");
  expect(moderngekko::frontend::DetectControllerLayout(
             "Nintendo Switch Pro Controller") == ControllerLayout::Nintendo,
         "Switch Pro uses the Nintendo layout");
  expect(moderngekko::frontend::DetectControllerLayout("Joy-Con (L)") ==
             ControllerLayout::Nintendo,
         "Joy-Con uses the Nintendo layout");
  expect(moderngekko::frontend::DetectControllerLayout("Wii U Pro Controller") ==
             ControllerLayout::Nintendo,
         "Wii U Pro uses the Nintendo layout");
  expect(moderngekko::frontend::DetectControllerLayout(
             "DInput/0/Keyboard Mouse") == ControllerLayout::Keyboard,
         "keyboard device is detected");

  const fs::path directory =
      fs::temp_directory_path() / "wmx8-controller-layout-test";
  std::error_code ec;
  fs::remove_all(directory, ec);

  std::string message;
  expect(moderngekko::frontend::GenerateControllerConfig(
             directory, "SDL/0/Nintendo Switch Pro Controller",
             ControllerLayout::Auto, &message),
         "generate Switch Pro profile");
  std::string generated = ReadFile(directory / "Config" / "GCPadNew.ini");
  expect(Contains(generated, "Device = SDL/0/Nintendo Switch Pro Controller\n"),
         "Switch Pro device name");
  expect(Contains(generated, "Buttons/A = `Button B`\n"),
         "Switch Pro A is physical B");
  expect(Contains(generated, "Buttons/B = `Button A`\n"),
         "Switch Pro B is physical A");
  expect(Contains(generated, "Buttons/X = `Button Y`\n"),
         "Switch Pro X is physical Y");
  expect(Contains(generated, "Buttons/Y = `Button X`\n"),
         "Switch Pro Y is physical X");

  expect(moderngekko::frontend::GenerateControllerConfig(
             directory, "SDL/0/8BitDo Pro 2",
             moderngekko::frontend::ControllerMapping{
                 ControllerLayout::Nintendo, false},
             &message),
         "generate 8BitDo Pro 2 Nintendo profile");
  generated = ReadFile(directory / "Config" / "GCPadNew.ini");
  expect(Contains(generated, "Buttons/A = `Button B`\n"),
         "8BitDo Pro 2 A is physical B");
  expect(Contains(generated, "Buttons/B = `Button A`\n"),
         "8BitDo Pro 2 B is physical A");
  expect(Contains(generated, "Buttons/X = `Button Y`\n"),
         "8BitDo Pro 2 X is physical Y");
  expect(Contains(generated, "Buttons/Y = `Button X`\n"),
         "8BitDo Pro 2 Y is physical X");

  expect(moderngekko::frontend::GenerateControllerConfig(
             directory, "SDL/0/8BitDo Micro",
             moderngekko::frontend::ControllerMapping{
                 ControllerLayout::Nintendo, true},
             &message),
         "generate 8BitDo Micro corrected D-pad profile");
  generated = ReadFile(directory / "Config" / "GCPadNew.ini");
  expect(Contains(generated, "D-Pad/Left = `Pad E` & `Shoulder L`\n"),
         "8BitDo D-pad left corrected");
  expect(Contains(generated, "D-Pad/Right = `Pad W` & `Shoulder L`\n"),
         "8BitDo D-pad right corrected");

  expect(moderngekko::frontend::GenerateControllerConfig(
             directory, "SDL/0/DualSense Edge Wireless Controller",
             moderngekko::frontend::ControllerMapping{
                 ControllerLayout::Standard, false},
             &message),
         "generate DualSense Edge profile");
  generated = ReadFile(directory / "Config" / "GCPadNew.ini");
  expect(Contains(generated, "Buttons/A = `Button A`\n"), "Edge A");
  expect(Contains(generated, "Buttons/B = `Button B`\n"), "Edge B");
  expect(Contains(generated, "Buttons/X = `Button X`\n"), "Edge X");
  expect(Contains(generated, "Buttons/Y = `Button Y`\n"), "Edge Y");
  expect(generated.find("paddle") == std::string::npos &&
             generated.find("misc") == std::string::npos &&
             generated.find("touchpad") == std::string::npos,
         "Edge paddles/Fn/touchpad are not bound");

  expect(moderngekko::frontend::GenerateControllerConfig(
             directory, "SDL/0/Xbox Elite Wireless Controller",
             moderngekko::frontend::ControllerMapping{
                 ControllerLayout::Standard, false},
             &message),
         "generate Xbox Elite profile");
  generated = ReadFile(directory / "Config" / "GCPadNew.ini");
  expect(Contains(generated, "Buttons/A = `Button A`\n"), "Elite A");
  expect(Contains(generated, "Buttons/B = `Button B`\n"), "Elite B");
  expect(Contains(generated, "Buttons/X = `Button X`\n"), "Elite X");
  expect(Contains(generated, "Buttons/Y = `Button Y`\n"), "Elite Y");
  expect(generated.find("paddle") == std::string::npos &&
             generated.find("misc") == std::string::npos,
         "Elite paddles are not bound");

  const std::string default_standard =
      "Buttons/A = `Button A`\nButtons/B = `Button B`\n"
      "Buttons/X = `Button X`\nButtons/Y = `Button Y`\n"
      "D-Pad/Left = `Pad W` & `Shoulder L`\n"
      "D-Pad/Right = `Pad E` & `Shoulder L`\n";
  const std::string default_nintendo =
      "Buttons/A = `Button B`\nButtons/B = `Button A`\n"
      "Buttons/X = `Button Y`\nButtons/Y = `Button X`\n"
      "D-Pad/Left = `Pad W` & `Shoulder L`\n"
      "D-Pad/Right = `Pad E` & `Shoulder L`\n";
  expect(moderngekko::frontend::ControllerProfileIsDefault(default_standard,
                                                           ControllerLayout::Standard),
         "standard generated profile is recognized as default");
  expect(moderngekko::frontend::ControllerProfileIsDefault(default_nintendo,
                                                           ControllerLayout::Nintendo),
         "Nintendo generated profile is recognized as default");
  expect(!moderngekko::frontend::ControllerProfileIsDefault(
             "Buttons/A = `Button A`\nButtons/B = Custom\n",
             ControllerLayout::Standard),
         "custom profile is not treated as default");
  expect(moderngekko::frontend::ControllerProfileDpadSwapped(
             "D-Pad/Left = `Pad E` & `Shoulder L`\n"
             "D-Pad/Right = `Pad W` & `Shoulder L`\n"),
         "swapped D-pad profile is detected");
  expect(!moderngekko::frontend::ControllerProfileDpadSwapped(default_standard),
         "standard D-pad profile is not flagged as swapped");

  const auto buggy_8bitdo = moderngekko::frontend::DetectControllerMapping(
      "8BitDo Pro 2", pro2_mapping);
  expect(buggy_8bitdo.layout == ControllerLayout::Nintendo &&
             moderngekko::frontend::ControllerProfileIsDefault(
                 default_standard, ControllerLayout::Standard),
         "existing default 8BitDo profile is migrated to Nintendo");

  // Rewrite rules: the runner's decision is shared with these tests.
  using moderngekko::frontend::ControllerConfigNeedsRewrite;
  using moderngekko::frontend::ControllerMapping;
  const ControllerMapping standard_pad{ControllerLayout::Standard, false};
  const ControllerMapping nintendo_pad{ControllerLayout::Nintendo, false};

  expect(ControllerConfigNeedsRewrite("", "", standard_pad, ControllerLayout::Auto),
         "no profile yet is written");
  expect(!ControllerConfigNeedsRewrite(default_standard, "", standard_pad,
                                       ControllerLayout::Auto),
         "empty scan keeps the existing profile");
  expect(!ControllerConfigNeedsRewrite(
             "Device = SDL/0/Xbox Elite Wireless Controller\n" + default_standard,
             "SDL/0/Xbox Elite Wireless Controller", standard_pad,
             ControllerLayout::Auto),
         "matching Xbox device keeps its standard profile");
  expect(ControllerConfigNeedsRewrite(default_standard, "SDL/0/8BitDo Pro 2",
                                      nintendo_pad, ControllerLayout::Auto),
         "8BitDo Nintendo layout migrates the stock standard profile");
  expect(ControllerConfigNeedsRewrite(default_standard,
                                      "SDL/0/Xbox One Controller", standard_pad,
                                      ControllerLayout::Nintendo),
         "an explicit layout override rewrites");
  expect(!ControllerConfigNeedsRewrite(
             "Device = SDL/0/8BitDo Pro 2\nButtons/A = `Button A`\n"
             "Buttons/B = Custom\n",
             "SDL/0/8BitDo Pro 2", nintendo_pad, ControllerLayout::Auto),
         "hand-edited profile is preserved");
  expect(ControllerConfigNeedsRewrite(
             default_standard, "SDL/0/Nintendo Switch Pro Controller",
             standard_pad, ControllerLayout::Auto),
         "swapped profile is regenerated for a standard pad");
  expect(ControllerConfigNeedsRewrite(
             default_standard, "SDL/0/Xbox One Controller", standard_pad,
             ControllerLayout::Auto),
         "a device that reappears under the same name keeps its profile");
  expect(ControllerConfigNeedsRewrite(
             "Device = SDL/0/Xbox One Controller\nButtons/A = `Button A`\n"
             "Buttons/B = `Button B`\nButtons/X = `Button X`\n"
             "Buttons/Y = `Button Y`\n",
             "SDL/0/Xbox Wireless Controller", standard_pad,
             ControllerLayout::Auto),
         "missing configured device rewrites to the pad that was found");

  expect(moderngekko::frontend::GenerateControllerConfig(
             directory, "DInput/0/Keyboard Mouse", ControllerLayout::Auto,
             &message),
         "generate keyboard profile");
  generated = ReadFile(directory / "Config" / "GCPadNew.ini");
  expect(Contains(generated, "Buttons/A = `RETURN`\n"), "keyboard A");
  expect(Contains(generated, "Buttons/B = `BACKSPACE`\n"), "keyboard B");
  expect(Contains(generated, "Buttons/Z = `LSHIFT`\n"), "keyboard Z");
  expect(Contains(generated, "Main Stick/Up = `W`\n"), "keyboard stick");

  expect(moderngekko::frontend::GenerateControllerConfig(
             directory, "SDL/0/Xbox Wireless Controller",
             ControllerLayout::Standard, &message),
         "generate Xbox profile");
  generated = ReadFile(directory / "Config" / "GCPadNew.ini");
  expect(Contains(generated, "Buttons/A = `Button A`\n"), "Xbox A");
  expect(Contains(generated, "Buttons/B = `Button B`\n"), "Xbox B");
  expect(Contains(generated, "Buttons/X = `Button X`\n"), "Xbox X");
  expect(Contains(generated, "Buttons/Y = `Button Y`\n"), "Xbox Y");

  // Memory card: [Core] SlotA = 1 (raw), SlotB = 255 (None).
  {
    const fs::path slot_dir = directory / "memcard";
    fs::remove_all(slot_dir, ec);
    bool changed = false;
    expect(moderngekko::frontend::EnsureMemoryCardSlots(slot_dir, &changed),
           "memory card ini is written");
    expect(changed, "a fresh Dolphin.ini reports a change");
    std::string ini = ReadFile(slot_dir / "Config" / "Dolphin.ini");
    expect(Contains(ini, "[Core]\n"), "Dolphin.ini gets a [Core] section");
    expect(Contains(ini, "SlotA = 1\n"), "Slot A is a raw memory card");
    expect(Contains(ini, "SlotB = 255\n"), "Slot B is empty (None)");

    changed = false;
    expect(moderngekko::frontend::EnsureMemoryCardSlots(slot_dir, &changed) &&
               !changed,
           "a correct Dolphin.ini is left alone");

    // Existing install with the GCI folder (8) and the Dummy device (0) must
    // be corrected, and unrelated keys kept.
    std::ofstream(slot_dir / "Config" / "Dolphin.ini", std::ios::trunc)
        << "[Core]\nCPUCore = 4\nSlotA = 8\nSlotB = 0\nDSPHLE = False\n"
           "\n[Display]\nFullscreen = True\n";
    changed = false;
    expect(moderngekko::frontend::EnsureMemoryCardSlots(slot_dir, &changed) &&
               changed,
           "an existing install is migrated");
    ini = ReadFile(slot_dir / "Config" / "Dolphin.ini");
    expect(Contains(ini, "SlotA = 1\n") && Contains(ini, "SlotB = 255\n"),
           "migration overwrites SlotA=8 and SlotB=0");
    expect(Contains(ini, "CPUCore = 4\n") && Contains(ini, "DSPHLE = False\n") &&
               Contains(ini, "Fullscreen = True\n"),
           "migration keeps unrelated keys and sections");
    expect(!Contains(ini, "SlotA = 8") && !Contains(ini, "SlotB = 0"),
           "no stale slot values remain");

    // [Core] present but missing the slot keys.
    std::ofstream(slot_dir / "Config" / "Dolphin.ini", std::ios::trunc)
        << "[Core]\nCPUCore = 4\n";
    expect(moderngekko::frontend::EnsureMemoryCardSlots(slot_dir, nullptr),
           "missing slot keys are added");
    ini = ReadFile(slot_dir / "Config" / "Dolphin.ini");
    expect(Contains(ini, "SlotA = 1\n") && Contains(ini, "SlotB = 255\n"),
           "added slot keys have the raw/none values");
    expect(ini.find("[Core]\nSlotA") != std::string::npos ||
               ini.find("CPUCore = 4\nSlotA") != std::string::npos,
           "slot keys land inside [Core]");
  }

  moderngekko::frontend::ConfigResult config;
  config.resolution = "1920x1080";
  config.controller = "SDL/0/Test Controller";
  config.controllers = {config.controller};
  config.controller_layout = "nintendo";
  expect(moderngekko::frontend::SaveConfig(directory, config, &message),
         "save controller_layout");
  const auto loaded = moderngekko::frontend::LoadConfig(directory, false);
  expect(loaded && loaded.controller_layout == "nintendo",
         "controller_layout round-trips");

  fs::remove_all(directory, ec);
  if (failures == 0)
    std::cout << "controller layout tests passed\n";
  return failures == 0 ? 0 : 1;
}

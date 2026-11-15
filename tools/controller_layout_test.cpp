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

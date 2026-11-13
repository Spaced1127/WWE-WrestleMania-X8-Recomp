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
  expect(moderngekko::frontend::DetectControllerLayout("8BitDo Pro 2") ==
             ControllerLayout::Standard,
         "8BitDo XInput uses the standard layout");
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

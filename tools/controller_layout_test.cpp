#include "frontend_config.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;
using moderngekko::frontend::ControllerClass;

namespace {
std::string ReadFile(const fs::path &path) {
  std::ifstream input(path);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

bool Contains(const std::string &text, const std::string &needle) {
  return text.find(needle) != std::string::npos;
}

void WriteFile(const fs::path &path, const std::string &text) {
  fs::create_directories(path.parent_path());
  std::ofstream(path, std::ios::trunc) << text;
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

  // --- layout classes: exactly two, plus auto -------------------------------
  ControllerClass parsed = ControllerClass::Auto;
  expect(moderngekko::frontend::ParseControllerClass("auto", &parsed) &&
             parsed == ControllerClass::Auto,
         "auto parses");
  expect(moderngekko::frontend::ParseControllerClass("positional", &parsed) &&
             parsed == ControllerClass::Positional,
         "positional parses");
  expect(moderngekko::frontend::ParseControllerClass("nintendo", &parsed) &&
             parsed == ControllerClass::Nintendo,
         "nintendo parses");
  expect(moderngekko::frontend::ParseControllerClass("standard", &parsed) &&
             parsed == ControllerClass::Positional,
         "legacy 'standard' maps to positional");
  expect(moderngekko::frontend::ParseControllerClass("switch", &parsed) &&
             parsed == ControllerClass::Nintendo,
         "legacy 'switch' maps to nintendo");
  expect(!moderngekko::frontend::ParseControllerClass("keyboard", &parsed),
         "keyboard is a device, not a class");
  expect(!moderngekko::frontend::ParseControllerClass("garbage", &parsed),
         "unknown class is rejected");
  expect(std::string(moderngekko::frontend::ControllerClassName(
             ControllerClass::Nintendo)) == "nintendo",
         "class name round-trips");

  // --- canonical profile ----------------------------------------------------
  const std::string profile =
      moderngekko::frontend::CanonicalPadProfile("SDL/0/Xbox One Controller");
  expect(Contains(profile, "[GCPad1]\n"), "profile has the GCPad1 section");
  expect(Contains(profile, "Device = SDL/0/Xbox One Controller\n"),
         "profile carries the device line");
  expect(Contains(profile, "Buttons/A = `Button A`\n") &&
             Contains(profile, "Buttons/B = `Button B`\n"),
         "profile A/B are positional");
  expect(Contains(profile, "Buttons/X = `Button X`\n") &&
             Contains(profile, "Buttons/Y = `Button Y`\n"),
         "profile X/Y are positional");
  expect(!Contains(profile, "Button B`\nButtons/B = `Button A`"),
         "no nintendo swap baked into the profile");
  expect(Contains(profile, "D-Pad/Left = `Pad W`\n") &&
             Contains(profile, "D-Pad/Right = `Pad E`\n"),
         "D-pad is canonical left/right");
  expect(!Contains(profile, "& `Shoulder L`"),
         "D-pad is not gated behind a shoulder button");
  const std::string keyboard =
      moderngekko::frontend::CanonicalPadProfile("DInput/0/Keyboard Mouse");
  expect(Contains(keyboard, "Buttons/A = `RETURN`\n"),
         "keyboard device gets the keyboard profile");
  expect(Contains(keyboard, "Device = DInput/0/Keyboard Mouse\n"),
         "keyboard profile device line");

  // --- bootstrap writes once, never rewrites -------------------------------
  const fs::path directory = fs::temp_directory_path() / "wmx8-controller-test";
  std::error_code ec;
  fs::remove_all(directory, ec);
  std::string message;
  expect(moderngekko::frontend::EnsureCanonicalPadProfile(
             directory, "SDL/0/Xbox One Controller", &message),
         "canonical profile is written on first run");
  const std::string written = ReadFile(directory / "Config" / "GCPadNew.ini");
  expect(written == profile, "written profile is the canonical text");

  const std::string custom = "[GCPad1]\nDevice = SDL/0/My Pad\nButtons/A = `X`\n";
  WriteFile(directory / "Config" / "GCPadNew.ini", custom);
  expect(moderngekko::frontend::EnsureCanonicalPadProfile(
             directory, "SDL/0/Xbox One Controller", &message),
         "existing profile is accepted");
  expect(ReadFile(directory / "Config" / "GCPadNew.ini") == custom,
         "existing profile is never rewritten at launch");
  expect(moderngekko::frontend::ControllerConfigExists(directory),
         "profile presence is reported");

  // --- configured device read back -----------------------------------------
  WriteFile(directory / "Config" / "GCPadNew.ini",
            "[GCPad1]\nDevice = SDL/0/Xbox One Controller\nButtons/A = `Button A`\n");
  const auto controllers = moderngekko::frontend::ReadConfiguredControllers(directory);
  expect(controllers.size() == 1 &&
             controllers.front() == "SDL/0/Xbox One Controller",
         "configured device is read back for the substitution log");

  // --- config key: override precedence surfaces in config.ini ---------------
  moderngekko::frontend::ConfigResult config;
  config.resolution = "1920x1080";
  config.controller = "SDL/0/Test Controller";
  config.controllers = {config.controller};
  config.controller_class = "nintendo";
  expect(moderngekko::frontend::SaveConfig(directory, config, &message),
         "save controller_class");
  const auto loaded = moderngekko::frontend::LoadConfig(directory, false);
  expect(loaded && loaded.controller_class == "nintendo",
         "controller_class round-trips");
  const auto legacy = moderngekko::frontend::ParseConfig(
      "resolution=1920x1080\ncontroller_layout=switch\n");
  expect(legacy && legacy.controller_class == "nintendo",
         "legacy controller_layout=switch becomes nintendo");
  const auto keyboard_key = moderngekko::frontend::ParseConfig(
      "resolution=1920x1080\ncontroller_class=keyboard\n");
  expect(keyboard_key && keyboard_key.controller_class == "keyboard",
         "controller_class=keyboard is accepted");
  const auto bad = moderngekko::frontend::ParseConfig(
      "resolution=1920x1080\ncontroller_class=sideways\n");
  expect(!bad, "unknown controller_class is rejected");

  // --- memory card keys (unchanged) ----------------------------------------
  {
    const fs::path slot_dir = directory / "memcard";
    fs::remove_all(slot_dir, ec);
    bool changed = false;
    expect(moderngekko::frontend::EnsureMemoryCardSlots(slot_dir, &changed),
           "memory card ini is written");
    std::string ini = ReadFile(slot_dir / "Config" / "Dolphin.ini");
    expect(Contains(ini, "SlotA = 1\n") && Contains(ini, "SlotB = 255\n"),
           "slot keys are raw/none");
    WriteFile(slot_dir / "Config" / "Dolphin.ini",
              "[Core]\nCPUCore = 4\nSlotA = 8\nSlotB = 0\n");
    expect(moderngekko::frontend::EnsureMemoryCardSlots(slot_dir, &changed) &&
               changed,
           "existing install migrates");
    ini = ReadFile(slot_dir / "Config" / "Dolphin.ini");
    expect(Contains(ini, "SlotA = 1\n") && Contains(ini, "SlotB = 255\n") &&
               Contains(ini, "CPUCore = 4\n"),
           "migration overwrites slots and keeps other keys");
  }

  fs::remove_all(directory, ec);
  if (failures == 0)
    std::cout << "controller layout tests passed\n";
  return failures == 0 ? 0 : 1;
}

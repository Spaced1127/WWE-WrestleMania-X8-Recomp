#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace moderngekko::frontend {
// Canonical controller system (reference implementation).
//
// ONE static GCPadNew.ini holds the SDL positional bindings and is never
// rewritten at launch. Which physical button is "A" is decided in the runtime
// input layer from the SDL gamepad type (see ciface::SDL::LayoutClass), with a
// user override from config.ini (`controller_class = auto|positional|nintendo`).
enum class ControllerClass {
  Auto,
  Positional,
  Nintendo,
};

const char *ControllerClassName(ControllerClass controller_class);
bool ParseControllerClass(std::string_view text, ControllerClass *out);

// The one canonical profile text (SDL positional mapping).
std::string CanonicalPadProfile(std::string_view device);

// Writes the canonical profile only when it does not exist yet (first run).
// Never called on a launch that already has a profile.
bool EnsureCanonicalPadProfile(const std::filesystem::path &user_directory,
                               std::string_view device, std::string *message);
bool ControllerConfigExists(const std::filesystem::path &user_directory);

// Forces [Core] SlotA = 1 (raw memory card) and SlotB = 255 (None) in
// <user>/Config/Dolphin.ini, creating the file or the section when missing.
bool EnsureMemoryCardSlots(const std::filesystem::path &user_directory,
                           bool *changed = nullptr);

struct ResolutionOption {
  const char *text;
  int dolphin_scale;
};

struct GraphicsBackendOption {
  const char *text;
  const char *value;
};

struct ConfigResult {
  int dolphin_scale = 0;
  std::string resolution;
  std::string graphics_backend = "Vulkan";
  std::string controller;
  std::vector<std::string> controllers;
  std::string controller_class = "auto";
  bool show_fps_in_title = true;
  bool fullscreen = false;
  int msaa = 1;
  bool ssaa = false;
  int anisotropy = 0;
  bool vsync = false;
  int aspect = 0;
  bool widescreen_hack = false;
  bool efb_access = true;
  bool mute = false;
  int volume = 100;
  float emulation_speed = 1.0f;
  // "static" runs the shipped DolRecomp module (the default for this port).
  // "jit" is the Dolphin JIT fallback and ignores the module.
  std::string cpu_core = "static";
  std::string netplay_nickname = "Player";
  std::string netplay_address = "127.0.0.1";
  std::uint16_t netplay_port = 2626;
  std::string netplay_buffer = "auto";
  std::string error;

  explicit operator bool() const { return error.empty(); }
};

const std::vector<ResolutionOption> &SupportedResolutions();
const std::vector<GraphicsBackendOption> &SupportedGraphicsBackends();
ConfigResult LoadConfig(const std::filesystem::path &user_directory,
                        bool create_if_missing);
ConfigResult ParseConfig(std::string_view text);
bool SaveConfig(const std::filesystem::path &user_directory,
                const ConfigResult &config, std::string *error);
bool SaveConfig(const std::filesystem::path &user_directory,
                std::string_view resolution, bool show_fps_in_title,
                std::string_view controller, std::string *error);
std::string
ReadConfiguredController(const std::filesystem::path &user_directory);
std::vector<std::string>
ReadConfiguredControllers(const std::filesystem::path &user_directory);
bool ControllerConfigExists(const std::filesystem::path &user_directory);
} // namespace moderngekko::frontend

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace moderngekko::frontend {
enum class ControllerLayout {
  Auto,
  Standard,
  Nintendo,
  Keyboard,
};

const char *ControllerLayoutName(ControllerLayout layout);
ControllerLayout DetectControllerLayout(std::string_view device);

// A controller's button family plus any D-pad left/right correction needed by
// its SDL mapping. DetectControllerMapping parses the live SDL mapping string,
// so 8BitDo Switch-mode and XInput-mode pads are handled per GUID/report, not
// by a name guess.
struct ControllerMapping {
  ControllerLayout layout = ControllerLayout::Standard;
  bool swap_dpad_lr = false;
};
ControllerMapping DetectControllerMapping(std::string_view device,
                                          std::string_view sdl_mapping = {},
                                          std::string_view sdl_guid = {});
bool ControllerProfileIsDefault(std::string_view text, ControllerLayout layout);
bool ControllerProfileDpadSwapped(std::string_view text);

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
  std::string controller_layout = "auto";
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
bool GenerateControllerConfig(const std::filesystem::path &user_directory,
                              std::span<const std::string> controllers,
                              std::string *message);
bool GenerateControllerConfig(const std::filesystem::path &user_directory,
                              std::span<const std::string> controllers,
                              ControllerLayout layout, std::string *message);
bool GenerateControllerConfig(const std::filesystem::path &user_directory,
                              std::span<const std::string> controllers,
                              ControllerMapping mapping, std::string *message);
bool GenerateControllerConfig(const std::filesystem::path &user_directory,
                              std::string_view controller,
                              std::string *message);
bool GenerateControllerConfig(const std::filesystem::path &user_directory,
                              std::string_view controller,
                              ControllerLayout layout, std::string *message);
bool GenerateControllerConfig(const std::filesystem::path &user_directory,
                              std::string_view controller,
                              ControllerMapping mapping, std::string *message);
bool EnsureControllerConfig(const std::filesystem::path &user_directory,
                            std::span<const std::string> controllers,
                            std::string *message);
bool EnsureControllerConfig(const std::filesystem::path &user_directory,
                            std::string_view controller, std::string *message);
} // namespace moderngekko::frontend

#include "frontend_config.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string_view>

namespace fs = std::filesystem;

namespace moderngekko::frontend {
namespace {
std::string Trim(std::string value) {
  const auto not_space = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(),
              value.end());
  return value;
}

std::string Lower(std::string value) {
  std::ranges::transform(value, value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

bool ValidNetplayAddress(std::string_view value) {
  if (value.empty() || value.size() > 253)
    return false;
  return std::ranges::all_of(value, [](unsigned char c) {
    return std::isalnum(c) || c == '.' || c == '-' || c == '_';
  });
}

std::string NormalizeGraphicsBackend(std::string value) {
  const std::string lower = Lower(Trim(std::move(value)));
  if (lower == "vulkan")
    return "Vulkan";
  if (lower == "opengl" || lower == "ogl")
    return "OGL";
  return {};
}

bool ParseBoolean(const std::string &value, bool *result) {
  if (value == "true" || value == "1" || value == "yes" || value == "on") {
    *result = true;
    return true;
  }
  if (value == "false" || value == "0" || value == "no" || value == "off") {
    *result = false;
    return true;
  }
  return false;
}

fs::path ControllerConfigPath(const fs::path &user_directory) {
#ifdef MODERNGEKKO_GAMECUBE_CONTROLLERS
  return user_directory / "Config" / "GCPadNew.ini";
#else
  return user_directory / "Config" / "WiimoteNew.ini";
#endif
}

std::string_view ControllerSectionPrefix() {
#ifdef MODERNGEKKO_GAMECUBE_CONTROLLERS
  return "[GCPad";
#else
  return "[Wiimote";
#endif
}

int HexNibble(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

std::optional<std::uint16_t> GuidWord(std::string_view guid,
                                      std::size_t offset) {
  if (guid.size() < offset + 4)
    return std::nullopt;
  const int low = HexNibble(guid[offset]);
  const int low2 = HexNibble(guid[offset + 1]);
  const int high = HexNibble(guid[offset + 2]);
  const int high2 = HexNibble(guid[offset + 3]);
  if (low < 0 || low2 < 0 || high < 0 || high2 < 0)
    return std::nullopt;
  return static_cast<std::uint16_t>((low << 4) | low2 |
                                    (((high << 4) | high2) << 8));
}

enum class ControllerGuidFamily { Unknown, DualSenseEdge, XboxElite };

ControllerGuidFamily ClassifyControllerGuid(std::string_view guid) {
  const std::optional<std::uint16_t> vendor = GuidWord(guid, 8);
  const std::optional<std::uint16_t> product = GuidWord(guid, 16);
  if (!vendor || !product)
    return ControllerGuidFamily::Unknown;
  if (*vendor == 0x054c && *product == 0x0df2)
    return ControllerGuidFamily::DualSenseEdge;
  if (*vendor == 0x045e &&
      (*product == 0x02e3 || *product == 0x0b00 || *product == 0x0b05))
    return ControllerGuidFamily::XboxElite;
  return ControllerGuidFamily::Unknown;
}

std::string_view MappingValue(std::string_view mapping, std::string_view key) {
  std::string needle(key);
  needle += ':';
  std::size_t position = 0;
  while ((position = mapping.find(needle, position)) != std::string_view::npos) {
    if (position == 0 || mapping[position - 1] == ',') {
      const std::size_t begin = position + needle.size();
      const std::size_t end = mapping.find(',', begin);
      return mapping.substr(begin, end == std::string_view::npos
                                      ? std::string_view::npos
                                      : end - begin);
    }
    position += needle.size();
  }
  return {};
}

std::optional<int> ButtonIndex(std::string_view value) {
  if (value.size() < 2 || value.front() != 'b')
    return std::nullopt;
  int index = 0;
  const auto parsed =
      std::from_chars(value.data() + 1, value.data() + value.size(), index);
  if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
    return std::nullopt;
  return index;
}

void WriteGameCubeMapping(std::ostream &output, ControllerMapping mapping) {
  const bool nintendo = mapping.layout == ControllerLayout::Nintendo;
  const char *dpad_left = mapping.swap_dpad_lr ? "`Pad E`" : "`Pad W`";
  const char *dpad_right = mapping.swap_dpad_lr ? "`Pad W`" : "`Pad E`";
  output << "Buttons/A = " << (nintendo ? "`Button B`" : "`Button A`") << '\n'
         << "Buttons/B = " << (nintendo ? "`Button A`" : "`Button B`") << '\n'
         << "Buttons/X = " << (nintendo ? "`Button Y`" : "`Button X`") << '\n'
         << "Buttons/Y = " << (nintendo ? "`Button X`" : "`Button Y`") << '\n'
         << "Buttons/Z = `Shoulder R`\n"
            "Buttons/Start = Start\n"
            "Main Stick/Up = `Pad N` | `Left Y+`\n"
            "Main Stick/Down = `Pad S` | `Left Y-`\n"
            "Main Stick/Left = `Pad W` | `Left X-`\n"
            "Main Stick/Right = `Pad E` | `Left X+`\n"
            "Main Stick/Calibration = 100.00\n"
            "C-Stick/Up = `Right Y+`\n"
            "C-Stick/Down = `Right Y-`\n"
            "C-Stick/Left = `Right X-`\n"
            "C-Stick/Right = `Right X+`\n"
            "C-Stick/Calibration = 100.00\n"
            "Triggers/L = `Trigger L`\n"
            "Triggers/R = `Trigger R`\n"
            "Triggers/L-Analog = `Trigger L`\n"
            "Triggers/R-Analog = `Trigger R`\n"
            "D-Pad/Up = `Pad N` & `Shoulder L`\n"
            "D-Pad/Down = `Pad S` & `Shoulder L`\n";
  output << "D-Pad/Left = " << dpad_left << " & `Shoulder L`\n"
         << "D-Pad/Right = " << dpad_right << " & `Shoulder L`\n"
         << "Rumble/Motor = `Motor L` | `Motor R`\n";
}

void WriteKeyboardMapping(std::ostream &output) {
  output << "Buttons/A = `RETURN`\n"
            "Buttons/B = `BACKSPACE`\n"
            "Buttons/X = `X`\n"
            "Buttons/Y = `Y`\n"
            "Buttons/Z = `LSHIFT`\n"
            "Buttons/Start = `SPACE`\n"
            "D-Pad/Up = `UP`\n"
            "D-Pad/Down = `DOWN`\n"
            "D-Pad/Left = `LEFT`\n"
            "D-Pad/Right = `RIGHT`\n"
            "Main Stick/Up = `W`\n"
            "Main Stick/Down = `S`\n"
            "Main Stick/Left = `A`\n"
            "Main Stick/Right = `D`\n"
            "Main Stick/Calibration = 100.00\n"
            "C-Stick/Up = `I`\n"
            "C-Stick/Down = `K`\n"
            "C-Stick/Left = `J`\n"
            "C-Stick/Right = `L`\n"
            "C-Stick/Calibration = 100.00\n"
            "Triggers/L = `Q`\n"
            "Triggers/R = `E`\n"
            "Triggers/L-Analog = `Q`\n"
            "Triggers/R-Analog = `E`\n";
}

} // namespace

const char *ControllerLayoutName(ControllerLayout layout) {
  switch (layout) {
  case ControllerLayout::Auto:
    return "auto";
  case ControllerLayout::Standard:
    return "standard";
  case ControllerLayout::Nintendo:
    return "nintendo";
  case ControllerLayout::Keyboard:
    return "keyboard";
  }
  return "auto";
}

ControllerLayout DetectControllerLayout(std::string_view device) {
  const std::string lower = Lower(std::string(device));
  if (lower.find("keyboard") != std::string::npos)
    return ControllerLayout::Keyboard;
  // Nintendo's face buttons are labelled in the opposite order from the
  // GameCube pad, so SDL's Button A/B and X/Y need swapping. This covers the
  // Switch Pro Controller, Joy-Con, Switch Online pads and Wii U Pro pads.
  if (lower.find("switch") != std::string::npos ||
      lower.find("joy-con") != std::string::npos ||
      lower.find("joycon") != std::string::npos ||
      lower.find("nintendo") != std::string::npos ||
      lower.find("pro controller") != std::string::npos ||
      lower.find("wii u") != std::string::npos)
    return ControllerLayout::Nintendo;
  // Xbox, PlayStation, Steam Deck, 8BitDo in XInput mode, the GameCube
  // adapter and generic SDL pads all use the standard positional layout.
  return ControllerLayout::Standard;
}

ControllerMapping DetectControllerMapping(std::string_view device,
                                          std::string_view sdl_mapping,
                                          std::string_view sdl_guid) {
  ControllerMapping result;
  const std::string lower = Lower(std::string(device));
  if (lower.find("keyboard") != std::string::npos) {
    result.layout = ControllerLayout::Keyboard;
    return result;
  }

  const ControllerGuidFamily family = ClassifyControllerGuid(sdl_guid);
  if (family == ControllerGuidFamily::DualSenseEdge ||
      family == ControllerGuidFamily::XboxElite) {
    // Edge/Elite are standard-position pads. Their extra paddles/Fn buttons
    // are intentionally not bound; the standard face buttons, sticks, D-pad
    // and triggers behave exactly like the base DualSense/Xbox pads.
    result.layout = ControllerLayout::Standard;
  } else if (lower.find("switch") != std::string::npos ||
             lower.find("joy-con") != std::string::npos ||
             lower.find("joycon") != std::string::npos ||
             lower.find("nintendo") != std::string::npos ||
             lower.find("pro controller") != std::string::npos ||
             lower.find("wii u") != std::string::npos) {
    // Keep the established name rule for Nintendo first-party pads. Their SDL
    // DB entries are not consistent about a/b ordering across firmware, but the
    // project has always treated the printed labels as the reference.
    result.layout = ControllerLayout::Nintendo;
  } else {
    // 8BitDo is the important case: Pro 2/SN30/SF30/N30 and the Switch-mode
    // report use a:b1,b:b0 (or a:b3,b:b2), while the Ultimate and 64 families
    // use a:b0,b:b1. Read the live SDL mapping rather than guessing from the
    // product name, so XInput and Switch modes are both correct.
    const std::optional<int> a = ButtonIndex(MappingValue(sdl_mapping, "a"));
    const std::optional<int> b = ButtonIndex(MappingValue(sdl_mapping, "b"));
    if (a && b && *a != *b)
      result.layout = *a > *b ? ControllerLayout::Nintendo
                              : ControllerLayout::Standard;
    else
      result.layout = DetectControllerLayout(device);
  }

  // Some 8BitDo reports expose the D-pad in the opposite left/right order.
  // Correct it from the mapping itself; the standard 8BitDo entries in the
  // shipped SDL DB are already left/right correct and stay unchanged.
  const std::string_view left = MappingValue(sdl_mapping, "dpleft");
  const std::string_view right = MappingValue(sdl_mapping, "dpright");
  if (!left.empty() && !right.empty()) {
    if (left.starts_with("h") && right.starts_with("h")) {
      result.swap_dpad_lr = left == "h0.2" && right == "h0.8";
    } else {
      const std::optional<int> left_button = ButtonIndex(left);
      const std::optional<int> right_button = ButtonIndex(right);
      if (left_button && right_button)
        result.swap_dpad_lr = *left_button > *right_button;
      else if (left.starts_with("+") && right.starts_with("-"))
        result.swap_dpad_lr = true;
    }
  }
  return result;
}

bool ControllerProfileDpadSwapped(std::string_view text) {
  return text.find("D-Pad/Left = `Pad E`") != std::string_view::npos &&
         text.find("D-Pad/Right = `Pad W`") != std::string_view::npos;
}

bool ControllerProfileIsDefault(std::string_view text,
                                ControllerLayout layout) {
  if (layout == ControllerLayout::Keyboard)
    return text.find("Buttons/A = `RETURN`") != std::string_view::npos &&
           text.find("Buttons/B = `BACKSPACE`") != std::string_view::npos;
  const bool nintendo = layout == ControllerLayout::Nintendo;
  return text.find(nintendo ? "Buttons/A = `Button B`" : "Buttons/A = `Button A`") !=
             std::string_view::npos &&
         text.find(nintendo ? "Buttons/B = `Button A`" : "Buttons/B = `Button B`") !=
             std::string_view::npos &&
         text.find(nintendo ? "Buttons/X = `Button Y`" : "Buttons/X = `Button X`") !=
             std::string_view::npos &&
         text.find(nintendo ? "Buttons/Y = `Button X`" : "Buttons/Y = `Button Y`") !=
             std::string_view::npos;
}

const std::vector<ResolutionOption> &SupportedResolutions() {
  // These are the output-resolution labels used by Dolphin's integer EFB
  // scales.
  static const std::vector<ResolutionOption> resolutions = {
      {"640x528", 1},   {"1280x720", 2},  {"1920x1080", 3},  {"2560x1440", 4},
      {"3840x2160", 6}, {"5120x2880", 8}, {"7680x4320", 12},
  };
  return resolutions;
}

const std::vector<GraphicsBackendOption> &SupportedGraphicsBackends() {
  static const std::vector<GraphicsBackendOption> backends = {
      {"Vulkan", "Vulkan"},
      {"OpenGL", "OGL"},
  };
  return backends;
}

ConfigResult LoadConfig(const fs::path &user_directory,
                        bool create_if_missing) {
  const fs::path path = user_directory / "config.ini";
  if (!fs::exists(path) && create_if_missing) {
    std::string error;
    if (!SaveConfig(user_directory, "1920x1080", true, {}, &error))
      return {.error = std::move(error)};
  }

  std::ifstream file(path);
  if (!file)
    return {.error = "can't open " + path.string()};
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
  return ParseConfig(text);
}

ConfigResult ParseConfig(std::string_view text) {
  ConfigResult config;
  std::istringstream file{std::string(text)};
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';' ||
        trimmed[0] == '[')
      continue;
    const std::size_t separator = trimmed.find('=');
    if (separator == std::string::npos)
      return {.error = "invalid config.ini line: " + trimmed};
    const std::string key = Lower(Trim(trimmed.substr(0, separator)));
    const std::string raw_value = Trim(trimmed.substr(separator + 1));
    const std::string value = Lower(raw_value);
    if (key == "resolution")
      config.resolution = value;
    else if (key == "backend" || key == "graphics_backend") {
      config.graphics_backend = NormalizeGraphicsBackend(raw_value);
      if (config.graphics_backend.empty())
        return {.error = "graphics backend must be Vulkan or OpenGL"};
    }
    else if (key == "controller")
      config.controller = raw_value;
    else if (key == "controller_layout") {
      const std::string layout = Lower(raw_value);
      if (layout == "auto" || layout == "standard" || layout == "xbox" ||
          layout == "playstation")
        config.controller_layout = layout == "auto" ? "auto" : "standard";
      else if (layout == "nintendo" || layout == "switch")
        config.controller_layout = "nintendo";
      else if (layout == "keyboard")
        config.controller_layout = "keyboard";
      else
        return {.error =
                    "controller_layout must be auto, standard, nintendo or "
                    "keyboard"};
    }
    else if (key.starts_with("controller") && key.size() == 11 &&
             key.back() >= '1' && key.back() <= '4') {
      const std::size_t index = static_cast<std::size_t>(key.back() - '1');
      if (config.controllers.size() <= index)
        config.controllers.resize(index + 1);
      config.controllers[index] = raw_value;
    } else if (key == "show_fps_in_title") {
      if (!ParseBoolean(value, &config.show_fps_in_title))
        return {.error = "show_fps_in_title must be true or false"};
    } else if (key == "fullscreen") {
      if (!ParseBoolean(value, &config.fullscreen))
        return {.error = "fullscreen must be true or false"};
    } else if (key == "msaa") {
      unsigned int msaa = 0;
      const auto parsed =
          std::from_chars(raw_value.data(), raw_value.data() + raw_value.size(), msaa);
      if (parsed.ec != std::errc{} ||
          parsed.ptr != raw_value.data() + raw_value.size() ||
          (msaa != 1 && msaa != 2 && msaa != 4 && msaa != 8))
        return {.error = "msaa must be 1, 2, 4 or 8"};
      config.msaa = static_cast<int>(msaa);
    } else if (key == "ssaa") {
      if (!ParseBoolean(value, &config.ssaa))
        return {.error = "ssaa must be true or false"};
    } else if (key == "anisotropy") {
      unsigned int anisotropy = 0;
      const auto parsed = std::from_chars(raw_value.data(),
                                          raw_value.data() + raw_value.size(),
                                          anisotropy);
      if (parsed.ec != std::errc{} ||
          parsed.ptr != raw_value.data() + raw_value.size() || anisotropy > 4)
        return {.error = "anisotropy must be 0, 1, 2, 3 or 4"};
      config.anisotropy = static_cast<int>(anisotropy);
    } else if (key == "vsync") {
      if (!ParseBoolean(value, &config.vsync))
        return {.error = "vsync must be true or false"};
    } else if (key == "aspect") {
      if (value == "auto")
        config.aspect = 0;
      else if (value == "wide" || value == "16:9")
        config.aspect = 1;
      else if (value == "standard" || value == "4:3")
        config.aspect = 2;
      else if (value == "stretch")
        config.aspect = 3;
      else
        return {.error = "aspect must be auto, wide, standard or stretch"};
    } else if (key == "widescreen_hack") {
      if (!ParseBoolean(value, &config.widescreen_hack))
        return {.error = "widescreen_hack must be true or false"};
    } else if (key == "efb_access") {
      if (!ParseBoolean(value, &config.efb_access))
        return {.error = "efb_access must be true or false"};
    } else if (key == "mute") {
      if (!ParseBoolean(value, &config.mute))
        return {.error = "mute must be true or false"};
    } else if (key == "volume") {
      unsigned int volume = 0;
      const auto parsed = std::from_chars(raw_value.data(),
                                          raw_value.data() + raw_value.size(), volume);
      if (parsed.ec != std::errc{} ||
          parsed.ptr != raw_value.data() + raw_value.size() || volume > 100)
        return {.error = "volume must be between 0 and 100"};
      config.volume = static_cast<int>(volume);
    } else if (key == "emulation_speed") {
      char *end = nullptr;
      const float speed = std::strtof(raw_value.c_str(), &end);
      if (end != raw_value.c_str() + raw_value.size() || !std::isfinite(speed) ||
          speed < 0.0f || speed > 4.0f)
        return {.error = "emulation_speed must be between 0 and 4"};
      // The game is fixed to the console's 60 Hz timing. Older configs may
      // still say 0 (unlocked) or 1.5/2.0; load them at 1.0 rather than
      // refusing to start, and never expose a faster-than-60 option again.
      config.emulation_speed = 1.0f;
    } else if (key == "cpu_core") {
      const std::string core = Lower(raw_value);
      if (core != "jit" && core != "static")
        return {.error = "cpu_core must be jit or static"};
      config.cpu_core = core;
    } else if (key == "nickname")
      config.netplay_nickname = raw_value;
    else if (key == "address")
      config.netplay_address = raw_value;
    else if (key == "port") {
      unsigned int port = 0;
      const auto parsed = std::from_chars(
          raw_value.data(), raw_value.data() + raw_value.size(), port);
      if (parsed.ec != std::errc{} ||
          parsed.ptr != raw_value.data() + raw_value.size() || port == 0 ||
          port > 65535)
        return {.error = "netplay port must be between 1 and 65535"};
      config.netplay_port = static_cast<std::uint16_t>(port);
    } else if (key == "buffer") {
      if (value != "auto") {
        unsigned int frames = 0;
        const auto parsed =
            std::from_chars(value.data(), value.data() + value.size(), frames);
        if (parsed.ec != std::errc{} ||
            parsed.ptr != value.data() + value.size() || frames < 1 ||
            frames > 20)
          return {.error =
                      "netplay buffer must be auto or a value from 1 to 20"};
      }
      config.netplay_buffer = value;
    }
  }
  if (config.resolution.empty())
    return {.error = "config.ini is missing resolution=<width>x<height>"};

  std::erase(config.controllers, std::string{});
  if (config.controllers.empty() && !config.controller.empty())
    config.controllers.push_back(config.controller);
  if (config.controller.empty() && !config.controllers.empty())
    config.controller = config.controllers.front();
  if (config.netplay_nickname.empty())
    return {.error = "netplay nickname cannot be empty"};
  if (config.netplay_nickname.size() > 30)
    return {.error = "netplay nickname cannot exceed 30 characters"};
  if (!ValidNetplayAddress(config.netplay_address))
    return {.error = "netplay address must be an IPv4 address or hostname"};

  for (const ResolutionOption &option : SupportedResolutions()) {
    if (config.resolution == option.text) {
      config.dolphin_scale = option.dolphin_scale;
      return config;
    }
  }

  // Dolphin also accepts exact raw EFB multiples even when they do not have a
  // common display label.
  for (int scale = 1; scale <= 12; ++scale) {
    const std::string raw =
        std::to_string(640 * scale) + "x" + std::to_string(528 * scale);
    if (config.resolution == raw) {
      config.dolphin_scale = scale;
      return config;
    }
  }

  return {.error = "unsupported Dolphin internal resolution '" +
                   config.resolution +
                   "'; use a listed display resolution or an exact 640x528 "
                   "multiple up to 12x"};
}

bool SaveConfig(const fs::path &user_directory, const ConfigResult &config,
                std::string *error) {
  const std::string graphics_backend =
      NormalizeGraphicsBackend(config.graphics_backend);
  if (config.resolution.empty() || graphics_backend.empty() ||
      config.netplay_nickname.empty() ||
      config.netplay_nickname.size() > 30 ||
      config.netplay_nickname.find_first_of("\r\n") != std::string::npos ||
      !ValidNetplayAddress(config.netplay_address) ||
      config.netplay_address.find_first_of("\r\n") != std::string::npos ||
      config.netplay_port == 0 ||
      (config.msaa != 1 && config.msaa != 2 && config.msaa != 4 && config.msaa != 8) ||
      config.anisotropy < 0 || config.anisotropy > 4 || config.aspect < 0 ||
      config.aspect > 3 || config.emulation_speed < 0.0f ||
      config.emulation_speed > 4.0f ||
      (config.cpu_core != "jit" && config.cpu_core != "static") ||
      config.volume < 0 || config.volume > 100) {
    if (error)
      *error = "invalid frontend settings";
    return false;
  }
  if (config.netplay_buffer != "auto") {
    unsigned int frames = 0;
    const auto parsed = std::from_chars(
        config.netplay_buffer.data(),
        config.netplay_buffer.data() + config.netplay_buffer.size(), frames);
    if (parsed.ec != std::errc{} ||
        parsed.ptr !=
            config.netplay_buffer.data() + config.netplay_buffer.size() ||
        frames < 1 || frames > 20) {
      if (error)
        *error = "netplay buffer must be auto or a value from 1 to 20";
      return false;
    }
  }
  std::error_code ec;
  fs::create_directories(user_directory, ec);
  if (ec) {
    if (error)
      *error = "can't create user directory: " + ec.message();
    return false;
  }
  std::ofstream file(user_directory / "config.ini", std::ios::trunc);
  if (!file) {
    if (error)
      *error = "can't write " + (user_directory / "config.ini").string();
    return false;
  }
  file << "# ModernGekko frontend settings\n"
          "# This is Dolphin's internal render target, not the window size.\n"
          "[Video]\n"
          "resolution="
       << config.resolution << '\n'
       << "backend=" << graphics_backend << '\n'
       << "fullscreen=" << (config.fullscreen ? "true" : "false") << '\n'
       << "show_fps_in_title=" << (config.show_fps_in_title ? "true" : "false")
       << '\n'
       << "msaa=" << config.msaa << '\n'
       << "ssaa=" << (config.ssaa ? "true" : "false") << '\n'
       << "anisotropy=" << config.anisotropy << '\n'
       << "vsync=" << (config.vsync ? "true" : "false") << '\n'
       << "aspect="
       << (config.aspect == 0 ? "auto"
           : config.aspect == 1 ? "wide"
           : config.aspect == 2 ? "standard"
                                : "stretch")
       << '\n'
       << "widescreen_hack=" << (config.widescreen_hack ? "true" : "false")
       << '\n'
       << "efb_access=" << (config.efb_access ? "true" : "false") << '\n'
       << "mute=" << (config.mute ? "true" : "false") << '\n'
       << "volume=" << config.volume << '\n'
       << "emulation_speed=" << config.emulation_speed << '\n'
       << "cpu_core=" << config.cpu_core << '\n'
       << "[Input]\n";
  for (std::size_t i = 0; i < config.controllers.size() && i < 4; ++i) {
    if (config.controllers[i].find_first_of("\r\n") != std::string::npos) {
      if (error)
        *error = "controller device cannot contain a newline";
      return false;
    }
    file << "controller" << i + 1 << '=' << config.controllers[i] << '\n';
  }
  file << "controller_layout=" << config.controller_layout << '\n'
       << "[Netplay]\n"
       << "nickname=" << config.netplay_nickname << '\n'
       << "address=" << config.netplay_address << '\n'
       << "port=" << config.netplay_port << '\n'
       << "buffer=" << config.netplay_buffer << '\n';
  return true;
}

bool SaveConfig(const fs::path &user_directory, std::string_view resolution,
                bool show_fps_in_title, std::string_view controller,
                std::string *error) {
  ConfigResult config = LoadConfig(user_directory, false);
  if (!config)
    config = {};
  config.resolution = resolution;
  config.show_fps_in_title = show_fps_in_title;
  config.controller = controller;
  config.controllers.clear();
  if (!controller.empty())
    config.controllers.emplace_back(controller);
  return SaveConfig(user_directory, config, error);
}

std::string ReadConfiguredController(const fs::path &user_directory) {
  const std::vector<std::string> controllers =
      ReadConfiguredControllers(user_directory);
  return controllers.empty() ? std::string{} : controllers.front();
}

std::vector<std::string>
ReadConfiguredControllers(const fs::path &user_directory) {
  std::ifstream input(ControllerConfigPath(user_directory));
  std::vector<std::string> controllers;
  std::string line;
  std::size_t controller_index = 4;
  const std::string_view section_prefix = ControllerSectionPrefix();
  while (std::getline(input, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.starts_with('[') && trimmed.ends_with(']')) {
      controller_index = 4;
      if (trimmed.size() == section_prefix.size() + 2 &&
          trimmed.starts_with(section_prefix) &&
          trimmed[section_prefix.size()] >= '1' &&
          trimmed[section_prefix.size()] <= '4')
        controller_index = static_cast<std::size_t>(
            trimmed[section_prefix.size()] - '1');
      continue;
    }
    if (controller_index >= 4)
      continue;
    const std::size_t separator = trimmed.find('=');
    if (separator != std::string::npos &&
        Trim(trimmed.substr(0, separator)) == "Device") {
      const std::string device = Trim(trimmed.substr(separator + 1));
      if (!device.empty()) {
        if (controllers.size() <= controller_index)
          controllers.resize(controller_index + 1);
        controllers[controller_index] = device;
      }
    }
  }
  std::erase(controllers, std::string{});
  return controllers;
}

bool ControllerConfigExists(const fs::path &user_directory) {
  std::error_code ec;
  return fs::is_regular_file(ControllerConfigPath(user_directory), ec);
}

bool GenerateControllerConfig(const fs::path &user_directory,
                              std::span<const std::string> controllers,
                              ControllerMapping mapping, std::string *message) {
  if (controllers.empty() || controllers.size() > 4) {
    if (message)
      *message = "select between one and four connected SDL gamepads";
    return false;
  }
  for (const std::string &controller : controllers) {
    if (controller.empty() ||
        controller.find_first_of("\r\n") != std::string_view::npos) {
      if (message)
        *message = "select connected SDL gamepads";
      return false;
    }
  }

  const fs::path destination = ControllerConfigPath(user_directory);
  std::error_code ec;
  fs::create_directories(destination.parent_path(), ec);
  if (ec) {
    if (message)
      *message = "can't create controller config directory: " + ec.message();
    return false;
  }
  std::ofstream output(destination, std::ios::trunc);
  if (!output) {
    if (message)
      *message = "can't write " + destination.string();
    return false;
  }
  for (std::size_t i = 0; i < 4; ++i) {
#ifdef MODERNGEKKO_GAMECUBE_CONTROLLERS
    output << "[GCPad" << i + 1 << "]\n";
    if (i >= controllers.size())
      continue;
    output << "Device = " << controllers[i] << '\n';
    const ControllerLayout detected = DetectControllerLayout(controllers[i]);
    ControllerMapping resolved = mapping;
    if (detected == ControllerLayout::Keyboard)
      resolved.layout = ControllerLayout::Keyboard;
    else if (resolved.layout == ControllerLayout::Auto)
      resolved.layout = detected;
    if (resolved.layout == ControllerLayout::Keyboard)
      WriteKeyboardMapping(output);
    else
      WriteGameCubeMapping(output, resolved);
#else
    (void)mapping;
    output << "[Wiimote" << i + 1 << "]\n";
    if (i >= controllers.size())
      continue;
    output << "Device = " << controllers[i] << '\n'
           << "Buttons/A = `Shoulder L`\n"
              "Buttons/B = `Shoulder R`\n"
              "Buttons/1 = `Button W`\n"
              "Buttons/2 = `Button S`\n"
              "Buttons/- = Back\n"
              "Buttons/+ = Start\n"
              "Buttons/Home = Guide\n"
              "D-Pad/Up = `Pad N` | `Left Y+`\n"
              "D-Pad/Down = `Pad S` | `Left Y-`\n"
              "D-Pad/Left = `Pad W` | `Left X-`\n"
              "D-Pad/Right = `Pad E` | `Left X+`\n"
              "IR/Up = `Cursor Y-`\n"
              "IR/Down = `Cursor Y+`\n"
              "IR/Left = `Cursor X-`\n"
              "IR/Right = `Cursor X+`\n"
              "Shake/X = `Trigger L`\n"
              "Shake/Y = `Trigger R`\n"
              "Shake/Z = `Trigger L`\n"
              "IRPassthrough/Object 1 X = `IR Object 1 X`\n"
              "IRPassthrough/Object 1 Y = `IR Object 1 Y`\n"
              "IRPassthrough/Object 1 Size = `IR Object 1 Size`\n"
              "IRPassthrough/Object 2 X = `IR Object 2 X`\n"
              "IRPassthrough/Object 2 Y = `IR Object 2 Y`\n"
              "IRPassthrough/Object 2 Size = `IR Object 2 Size`\n"
              "IRPassthrough/Object 3 X = `IR Object 3 X`\n"
              "IRPassthrough/Object 3 Y = `IR Object 3 Y`\n"
              "IRPassthrough/Object 3 Size = `IR Object 3 Size`\n"
              "IRPassthrough/Object 4 X = `IR Object 4 X`\n"
              "IRPassthrough/Object 4 Y = `IR Object 4 Y`\n"
              "IRPassthrough/Object 4 Size = `IR Object 4 Size`\n"
              "IMUAccelerometer/Up = `Accel Up`\n"
              "IMUAccelerometer/Down = `Accel Down`\n"
              "IMUAccelerometer/Left = `Accel Left`\n"
              "IMUAccelerometer/Right = `Accel Right`\n"
              "IMUAccelerometer/Forward = `Accel Forward`\n"
              "IMUAccelerometer/Backward = `Accel Backward`\n"
              "IMUGyroscope/Pitch Up = `Gyro Pitch Up`\n"
              "IMUGyroscope/Pitch Down = `Gyro Pitch Down`\n"
              "IMUGyroscope/Roll Left = `Gyro Roll Left`\n"
              "IMUGyroscope/Roll Right = `Gyro Roll Right`\n"
              "IMUGyroscope/Yaw Left = `Gyro Yaw Left`\n"
              "IMUGyroscope/Yaw Right = `Gyro Yaw Right`\n"
              "Rumble/Motor = Motor\n"
              "Extension = None\n"
              "Options/Sideways Wiimote = True\n";
#endif
  }
#ifndef MODERNGEKKO_GAMECUBE_CONTROLLERS
  output << "[BalanceBoard]\n";
#endif
  if (!output) {
    if (message)
      *message = "can't write " + destination.string();
    return false;
  }
  if (message) {
    *message = std::to_string(controllers.size()) +
#ifdef MODERNGEKKO_GAMECUBE_CONTROLLERS
               " GameCube controller" +
#else
               " sideways Wii Remote" +
#endif
               (controllers.size() == 1 ? " mapped" : "s mapped");
#ifdef MODERNGEKKO_GAMECUBE_CONTROLLERS
    if (mapping.layout != ControllerLayout::Auto)
      *message += std::string(" (") + ControllerLayoutName(mapping.layout) + " layout)";
    if (mapping.swap_dpad_lr)
      *message += " (d-pad left/right corrected)";
#endif
  }
  return true;
}

bool GenerateControllerConfig(const fs::path &user_directory,
                              std::span<const std::string> controllers,
                              std::string *message) {
  return GenerateControllerConfig(user_directory, controllers,
                                  ControllerMapping{ControllerLayout::Auto, false},
                                  message);
}

bool GenerateControllerConfig(const fs::path &user_directory,
                              std::span<const std::string> controllers,
                              ControllerLayout layout, std::string *message) {
  return GenerateControllerConfig(user_directory, controllers,
                                  ControllerMapping{layout, false}, message);
}

bool GenerateControllerConfig(const fs::path &user_directory,
                              std::string_view controller,
                              ControllerMapping mapping, std::string *message) {
  const std::string value(controller);
  return GenerateControllerConfig(
      user_directory, std::span<const std::string>(&value, 1), mapping, message);
}

bool GenerateControllerConfig(const fs::path &user_directory,
                              std::string_view controller,
                              ControllerLayout layout, std::string *message) {
  return GenerateControllerConfig(user_directory, controller,
                                  ControllerMapping{layout, false}, message);
}

bool GenerateControllerConfig(const fs::path &user_directory,
                              std::string_view controller,
                              std::string *message) {
  return GenerateControllerConfig(user_directory, controller,
                                  ControllerMapping{ControllerLayout::Auto, false},
                                  message);
}

bool EnsureControllerConfig(const fs::path &user_directory,
                            std::span<const std::string> controllers,
                            std::string *message) {
  if (ControllerConfigExists(user_directory)) {
    if (message)
      *message = "using existing controller profile";
    return true;
  }
  return GenerateControllerConfig(user_directory, controllers, message);
}

bool EnsureControllerConfig(const fs::path &user_directory,
                            std::string_view controller, std::string *message) {
  const std::string value(controller);
  return EnsureControllerConfig(
      user_directory, std::span<const std::string>(&value, 1), message);
}
} // namespace moderngekko::frontend

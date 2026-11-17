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

void WritePositionalGameCubeMapping(std::ostream &output) {
  // Canonical SDL positional bindings. The runtime swaps A/B and X/Y in the
  // input layer for Nintendo-class pads, so this text never changes.
  output << "Buttons/A = `Button A`\n"
            "Buttons/B = `Button B`\n"
            "Buttons/X = `Button X`\n"
            "Buttons/Y = `Button Y`\n"
            "Buttons/Z = `Shoulder R`\n"
            "Buttons/Start = Start\n"
            "Main Stick/Up = `Left Y+`\n"
            "Main Stick/Down = `Left Y-`\n"
            "Main Stick/Left = `Left X-`\n"
            "Main Stick/Right = `Left X+`\n"
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
            "D-Pad/Up = `Pad N`\n"
            "D-Pad/Down = `Pad S`\n"
            "D-Pad/Left = `Pad W`\n"
            "D-Pad/Right = `Pad E`\n"
            "Rumble/Motor = `Motor L` | `Motor R`\n";
}

} // namespace

const char *ControllerClassName(ControllerClass controller_class) {
  switch (controller_class) {
  case ControllerClass::Auto:
    return "auto";
  case ControllerClass::Positional:
    return "positional";
  case ControllerClass::Nintendo:
    return "nintendo";
  }
  return "auto";
}

bool ParseControllerClass(std::string_view text, ControllerClass *out) {
  if (!out)
    return false;
  const std::string lower = Lower(Trim(std::string(text)));
  if (lower == "auto") {
    *out = ControllerClass::Auto;
    return true;
  }
  if (lower == "positional" || lower == "standard" || lower == "default" ||
      lower == "xbox" || lower == "playstation") {
    *out = ControllerClass::Positional;
    return true;
  }
  if (lower == "nintendo" || lower == "switch") {
    *out = ControllerClass::Nintendo;
    return true;
  }
  return false;
}

std::string CanonicalPadProfile(std::string_view device) {
  std::ostringstream output;
  output << "[GCPad1]\n";
  output << "Device = " << device << '\n';
  if (device == "DInput/0/Keyboard Mouse")
    WriteKeyboardMapping(output);
  else
    WritePositionalGameCubeMapping(output);
  return output.str();
}

bool EnsureCanonicalPadProfile(const std::filesystem::path &user_directory,
                               std::string_view device, std::string *message) {
  const fs::path destination = ControllerConfigPath(user_directory);
  std::error_code ec;
  if (fs::is_regular_file(destination, ec)) {
    if (message)
      *message = "existing " + destination.filename().string() + " kept";
    return true;
  }
  fs::create_directories(destination.parent_path(), ec);
  std::ofstream output(destination, std::ios::trunc);
  if (!output) {
    if (message)
      *message = "can't write " + destination.string();
    return false;
  }
  output << CanonicalPadProfile(device);
  if (!output) {
    if (message)
      *message = "can't write " + destination.string();
    return false;
  }
  if (message)
    *message = "1 GameCube controller mapped (positional profile)";
  return true;
}


std::vector<std::string>
ReadConfiguredControllers(const fs::path &user_directory) {
  std::ifstream input(ControllerConfigPath(user_directory));
  std::vector<std::string> controllers;
  std::string line;
  bool in_section = false;
  std::string device;
  while (std::getline(input, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed.starts_with('[') && trimmed.ends_with(']')) {
      if (in_section && !device.empty())
        controllers.push_back(device);
      in_section = trimmed == "[GCPad1]";
      device.clear();
      continue;
    }
    if (!in_section)
      continue;
    const std::size_t separator = trimmed.find('=');
    if (separator != std::string::npos &&
        Trim(trimmed.substr(0, separator)) == "Device")
      device = Trim(trimmed.substr(separator + 1));
  }
  if (in_section && !device.empty())
    controllers.push_back(device);
  return controllers;
}

std::string ReadConfiguredController(const fs::path &user_directory) {
  const std::vector<std::string> controllers =
      ReadConfiguredControllers(user_directory);
  return controllers.empty() ? std::string{} : controllers.front();
}


std::string ControllerProfileDevice(std::string_view text) {
  constexpr std::string_view marker = "Device = ";
  const std::size_t start = text.find(marker);
  if (start == std::string_view::npos)
    return {};
  const std::size_t begin = start + marker.size();
  const std::size_t end = text.find_first_of("\r\n", begin);
  return std::string(text.substr(begin, end == std::string_view::npos
                                            ? std::string_view::npos
                                            : end - begin));
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
    else if (key == "controller_class" || key == "controller_layout") {
      if (Lower(Trim(raw_value)) == "keyboard") {
        config.controller_class = "keyboard";
      } else {
        ControllerClass parsed = ControllerClass::Auto;
        if (!ParseControllerClass(raw_value, &parsed))
          return {.error = "controller_class must be auto, positional, "
                           "nintendo or keyboard"};
        config.controller_class = ControllerClassName(parsed);
      }
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
  file << "controller_class=" << config.controller_class << '\n'
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



bool ControllerConfigExists(const fs::path &user_directory) {
  std::error_code ec;
  return fs::is_regular_file(ControllerConfigPath(user_directory), ec);
}

bool EnsureMemoryCardSlots(const std::filesystem::path &user_directory,
                           bool *changed) {
  const fs::path path = user_directory / "Config" / "Dolphin.ini";
  std::string text;
  {
    std::ifstream input(path);
    if (input)
      text.assign(std::istreambuf_iterator<char>(input),
                  std::istreambuf_iterator<char>());
  }

  std::vector<std::string> lines;
  {
    std::string line;
    std::istringstream input(text);
    while (std::getline(input, line)) {
      if (!line.empty() && line.back() == '\r')
        line.pop_back();
      lines.emplace_back(std::move(line));
    }
  }

  bool in_core = false;
  bool section_found = false;
  bool slot_a_set = false;
  bool slot_b_set = false;
  std::size_t core_last_key = 0;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string trimmed = Trim(lines[i]);
    if (trimmed.starts_with('[') && trimmed.ends_with(']')) {
      in_core = Lower(trimmed) == "[core]";
      if (in_core) {
        section_found = true;
        core_last_key = i;
      }
      continue;
    }
    if (!in_core || trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';')
      continue;
    const std::size_t separator = trimmed.find('=');
    if (separator == std::string::npos)
      continue;
    const std::string key = Lower(Trim(trimmed.substr(0, separator)));
    if (key == "slota") {
      lines[i] = "SlotA = 1";
      slot_a_set = true;
      core_last_key = i;
    } else if (key == "slotb") {
      lines[i] = "SlotB = 255";
      slot_b_set = true;
      core_last_key = i;
    } else {
      core_last_key = i;
    }
  }

  if (!section_found) {
    if (!lines.empty() && lines.back().empty())
      lines.pop_back();
    lines.emplace_back("[Core]");
    core_last_key = lines.size() - 1;
  }
  std::vector<std::string> inserts;
  if (!slot_a_set)
    inserts.emplace_back("SlotA = 1");
  if (!slot_b_set)
    inserts.emplace_back("SlotB = 255");
  if (!inserts.empty())
    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(core_last_key) + 1,
                 inserts.begin(), inserts.end());

  std::string output;
  for (const std::string &line : lines) {
    output += line;
    output += '\n';
  }
  if (output == text) {
    if (changed)
      *changed = false;
    return true;
  }
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  std::ofstream stream(path, std::ios::trunc);
  if (!stream) {
    if (changed)
      *changed = false;
    return false;
  }
  stream << output;
  if (!stream) {
    if (changed)
      *changed = false;
    return false;
  }
  if (changed)
    *changed = true;
  return true;
}

} // namespace moderngekko::frontend

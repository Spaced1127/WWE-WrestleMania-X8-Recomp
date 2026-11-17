#include "cache_affinity.hpp"
#include "frontend_config.hpp"
#include "dol_patch.hpp"
#include "moderngekko/game.hpp"
#include "moderngekko/runtime.hpp"
#include "netplay_session.hpp"

#include "Common/StringUtil.h"

#include <SDL3/SDL.h>


#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if defined(_WIN32)
// For the affinity/priority setup in main(). WIN32_LEAN_AND_MEAN keeps the
// winsock and GDI surface out of a translation unit that only needs the
// processor-topology and process calls.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {
#ifndef MODERNGEKKO_RUNNER_NAME
#define MODERNGEKKO_RUNNER_NAME "moderngekko-run"
#endif

#ifndef MODERNGEKKO_USER_DIRECTORY_NAME
#define MODERNGEKKO_USER_DIRECTORY_NAME "moderngekko"
#endif

volatile std::sig_atomic_t s_stop_requested = 0;

void HandleStopSignal(int) { s_stop_requested = 1; }

// A fresh RecompLauncher install never opens the custom launcher, so no
// GCPadNew.ini exists and RecompLauncher has no controller page. Detect the
// connected pad here, choose its button family, and generate the same profile
// the launcher would have made. An explicit controller_layout in config.ini
// (or --controller-layout) forces a layout; Auto keeps an existing profile for
// the same device so hand-edited profiles are not overwritten.
std::string ReadTextFile(const std::filesystem::path &path) {
  std::ifstream input(path);
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

// Profile parsing and the rewrite decision live in frontend_config so the
// offline tests exercise the same rules the runner applies.

struct ControllerDevice {
  std::string device;
  std::string guid;
};

// Dolphin's SDL input backend (ControllerInterface/SDL/SDL.cpp) applies these
// hints before it initialises SDL. The same pad can surface under a different
// name depending on which Windows driver path wins (XInput, WGI, DirectInput,
// HIDAPI), and the profile stores that name verbatim, so this scan has to run
// with the runtime's driver set. Joystick initialization is one-shot and hint
// driven: initialising here first is what keeps both enumerations identical.
void ApplyRuntimeSdlHints() {
  SDL_SetHint(SDL_HINT_JOYSTICK_DIRECTINPUT, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_GAMECUBE, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_COMBINE_JOY_CONS, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_VERTICAL_JOY_CONS, "0");
}

// Picks the configured pad when it is present, otherwise the first enumerated
// one. Nothing is written to disk here; the runtime binds the same pad through
// the emulated controller's default device, including after a hot-plug.
ControllerDevice ChooseControllerDevice(std::string_view preferred) {
  constexpr std::string_view keyboard = "DInput/0/Keyboard Mouse";
  ControllerDevice scan;
  if (preferred == keyboard)
    return scan;

  int count = 0;
  SDL_JoystickID *ids = SDL_GetJoysticks(&count);
  ControllerDevice first;
  ControllerDevice match;
  if (ids) {
    for (int i = 0; i < count; ++i) {
      if (!SDL_IsGamepad(ids[i]))
        continue;
      const char *name = SDL_GetGamepadNameForID(ids[i]);
      if (!name || !*name)
        continue;
      ControllerDevice candidate;
      candidate.device = "SDL/0/" + std::string(name);
      const SDL_GUID guid = SDL_GetGamepadGUIDForID(ids[i]);
      char guid_text[33]{};
      SDL_GUIDToString(guid, guid_text, static_cast<int>(sizeof(guid_text)));
      candidate.guid = guid_text;
      if (first.device.empty())
        first = candidate;
      if (candidate.device == preferred)
        match = candidate;
    }
  }
  SDL_free(ids);
  scan = !match.device.empty() ? match : first;
  if (!scan.device.empty() && !preferred.empty() && preferred != keyboard &&
      scan.device != preferred)
    std::cerr << "controller: configured device \"" << preferred
              << "\" not found; binding \"" << scan.device << "\"\n";
  return scan;
}

void EnsureControllerSetup(const std::filesystem::path &user_directory,
                           std::string_view controller_class,
                           std::string_view preferred_device) {

  ApplyRuntimeSdlHints();
  if (!SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD)) {
    std::cerr << "controller: SDL init failed: " << SDL_GetError() << '\n';
    return;
  }

  constexpr std::string_view keyboard_device = "DInput/0/Keyboard Mouse";
  const ControllerDevice chosen = ChooseControllerDevice(preferred_device);
  std::string device = chosen.device.empty() ? std::string(keyboard_device)
                                             : chosen.device;

  // One static profile. Written only when missing; never regenerated at launch.
  std::string message;
  if (!moderngekko::frontend::EnsureCanonicalPadProfile(user_directory, device,
                                                       &message))
    std::cerr << "controller configuration failed: " << message << '\n';
  else if (!chosen.device.empty())
    std::cerr << "controller configuration: " << message << '\n';

  // Layout class: user override wins; auto lets each pad decide from its SDL
  // type in the input layer (no table, no name heuristic, no profile rewrite).
  // The class travels to the runtime through RuntimeConfig; the input layer
  // applies it once, per pad, at init.

  std::cerr << "controller: name=\"" << device << "\" guid=" << chosen.guid
            << " class=" << controller_class
            << " dpad=normal profile="
            << (moderngekko::frontend::ControllerConfigExists(user_directory)
                    ? "static"
                    : "missing")
            << '\n';
  // Deliberately no SDL_Quit() here: the runtime's own SDL input backend calls
  // SDL_Init with the same hints and reuses this driver set.
}

void Usage() {
  std::cerr << "usage: " MODERNGEKKO_RUNNER_NAME
               " [--game <extracted-root>] [--module <path>]\n"
               "       [--user-dir <path>] [--title <text>] [--load-state <path>]\n"
               "       [--core <jit|static>] [--graphics <backend>] [--audio <backend>]\n"
               "       [--mods <directory>] [--no-mods]\n"
               "       [--wayland] [-X11] [--headless] [--allow-interpreter]\n"
               "       [--netplay-host | --netplay-join <host>] "
               "[--netplay-port <port>]\n"
               "       [--nickname <name>] [--buffer <auto|1-20>] "
               "[--controller <device>]...\n"
               "       [--controller-layout <auto|standard|nintendo|keyboard>]\n"
               "       With no --game, boots the path in "
               "<user-dir>/default-game.txt.\n";
}

std::filesystem::path
ReadDefaultGame(const std::filesystem::path &user_directory,
                const std::filesystem::path &executable_directory) {
#ifdef MODERNGEKKO_PORTABLE_DEFAULT_GAME
  const std::filesystem::path default_file =
      executable_directory / "default-game.txt";
#else
  const std::filesystem::path default_file =
      user_directory / "default-game.txt";
#endif
  std::ifstream file(default_file);
  std::string path;
  std::getline(file, path);
  if (!path.empty() && path.back() == '\r')
    path.pop_back();
  std::filesystem::path result(path);
  if (result.is_relative())
    result = executable_directory / result;
  return result;
}

std::filesystem::path DefaultUserDirectory() {
#ifdef MODERNGEKKO_USER_DIRECTORY_IN_DOCUMENTS
#if defined(_WIN32)
  if (const char *user_profile = std::getenv("USERPROFILE"))
    return std::filesystem::path(user_profile) / "Documents" /
           MODERNGEKKO_USER_DIRECTORY_NAME;
#endif
  if (const char *home = std::getenv("HOME"))
    return std::filesystem::path(home) / "Documents" /
           MODERNGEKKO_USER_DIRECTORY_NAME;
#endif
#if defined(_WIN32)
  if (const char *local_app_data = std::getenv("LOCALAPPDATA"))
    return std::filesystem::path(local_app_data) /
           MODERNGEKKO_USER_DIRECTORY_NAME;
#endif
  if (const char *xdg = std::getenv("XDG_DATA_HOME"))
    return std::filesystem::path(xdg) / MODERNGEKKO_USER_DIRECTORY_NAME;
  if (const char *home = std::getenv("HOME"))
    return std::filesystem::path(home) / ".local" / "share" /
           MODERNGEKKO_USER_DIRECTORY_NAME;
  return std::string(MODERNGEKKO_USER_DIRECTORY_NAME) + "-user";
}

std::string LibrarySuffix() {
#if defined(_WIN32)
  return ".dll";
#elif defined(__APPLE__)
  return ".dylib";
#else
  return ".so";
#endif
}

std::filesystem::path ExecutableDirectory(const char *argv0) {
  std::error_code ec;
#if defined(__linux__)
  const std::filesystem::path proc_executable =
      std::filesystem::read_symlink("/proc/self/exe", ec);
  if (!ec)
    return proc_executable.parent_path();
  ec.clear();
#endif
  const std::filesystem::path executable =
      std::filesystem::weakly_canonical(argv0, ec);
  return ec ? std::filesystem::current_path() : executable.parent_path();
}
} // namespace

int RunMain(int argc, char **argv) {
#if defined(_WIN32)
  if (!std::getenv("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD"))
    _putenv_s("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD", "1");
#else
  setenv("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD", "1", 0);
#endif
  moderngekko::RuntimeConfig config;
  config.user_directory = DefaultUserDirectory();
  const std::filesystem::path executable_directory =
      ExecutableDirectory(argv[0]);
#ifdef MODERNGEKKO_DEFAULT_WINDOW_TITLE
  config.window_title = MODERNGEKKO_DEFAULT_WINDOW_TITLE;
#endif
  std::filesystem::path module_path;
  std::string core_mode;
  bool use_default_mods = true;
  std::optional<moderngekko::frontend::NetplayRole> netplay_role;
  std::string netplay_address;
  std::optional<std::uint16_t> netplay_port;
  std::string netplay_nickname;
  std::string netplay_buffer;
  std::vector<std::string> netplay_controllers;
  std::string controller_layout_override;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto value = [&](const char *option) -> const char * {
      if (i + 1 >= argc) {
        std::cerr << option << " requires a value\n";
        std::exit(2);
      }
      return argv[++i];
    };
    if (arg == "--game")
      config.game_root = value("--game");
    else if (arg == "--module")
      module_path = value("--module");
    else if (arg == "--core") {
      const std::string selected = value("--core");
      if (selected != "jit" && selected != "static") {
        std::cerr << "--core must be jit or static\n";
        return 2;
      }
      core_mode = selected;
    } else if (arg == "--user-dir")
      config.user_directory = value("--user-dir");
    else if (arg == "--title")
      config.window_title = value("--title");
    else if (arg == "--load-state")
    {
      // Checked here rather than left to the boot path: a state that is not
      // there would otherwise boot to the title screen and look like it worked.
      std::filesystem::path state = StringToPath(value("--load-state"));
      if (!std::filesystem::is_regular_file(state))
      {
        std::cerr << "savestate not found: " << PathToString(state) << '\n';
        return 2;
      }
      config.load_state_path = std::move(state);
    }
    else if (arg == "--graphics")
      config.graphics.backend = value("--graphics");
    else if (arg == "--audio")
      config.audio.backend = value("--audio");
    else if (arg == "--mods")
      config.mod_directories.emplace_back(value("--mods"));
    else if (arg == "--no-mods")
      use_default_mods = false;
    else if (arg == "-X11" || arg == "--x11")
      config.window_system = moderngekko::WindowSystem::X11;
    else if (arg == "--wayland")
      config.window_system = moderngekko::WindowSystem::Wayland;
    else if (arg == "--mute")
      config.audio.mute = true;
    else if (arg == "--automation-dir")
      config.automation.directory = value("--automation-dir");
    else if (arg == "--headless")
      config.headless = true;
    else if (arg == "--allow-interpreter")
      config.allow_interpreter = true;
    else if (arg == "--netplay-host")
      netplay_role = moderngekko::frontend::NetplayRole::Host;
    else if (arg == "--netplay-join") {
      netplay_role = moderngekko::frontend::NetplayRole::Join;
      netplay_address = value("--netplay-join");
    } else if (arg == "--netplay-port") {
      const std::string port_value = value("--netplay-port");
      unsigned int port = 0;
      const auto parsed = std::from_chars(
          port_value.data(), port_value.data() + port_value.size(), port);
      if (parsed.ec != std::errc{} ||
          parsed.ptr != port_value.data() + port_value.size() || port == 0 ||
          port > 65535) {
        std::cerr << "--netplay-port must be between 1 and 65535\n";
        return 2;
      }
      netplay_port = static_cast<std::uint16_t>(port);
    } else if (arg == "--nickname")
      netplay_nickname = value("--nickname");
    else if (arg == "--buffer")
      netplay_buffer = value("--buffer");
    else if (arg == "--controller")
      netplay_controllers.emplace_back(value("--controller"));
    else if (arg == "--controller-layout") {
      controller_layout_override = value("--controller-layout");
      if (controller_layout_override == "xbox" ||
          controller_layout_override == "playstation")
        controller_layout_override = "standard";
      else if (controller_layout_override == "switch")
        controller_layout_override = "nintendo";
      if (controller_layout_override != "auto" &&
          controller_layout_override != "standard" &&
          controller_layout_override != "nintendo" &&
          controller_layout_override != "keyboard") {
        std::cerr << "--controller-layout must be auto, standard, nintendo or "
                     "keyboard\n";
        return 2;
      }
    }
    else if (arg == "--help" || arg == "-h") {
      Usage();
      return 0;
    } else {
      std::cerr << "unknown option: " << arg << '\n';
      Usage();
      return 2;
    }
  }
  if (config.game_root.empty())
    config.game_root =
        ReadDefaultGame(config.user_directory, executable_directory);
  if (config.game_root.empty()) {
    std::cerr << "no game configured; use --game once or create "
              << (config.user_directory / "default-game.txt") << '\n';
    Usage();
    return 2;
  }

  auto frontend_config =
      moderngekko::frontend::LoadConfig(config.user_directory, true);
  if (!frontend_config) {
    std::cerr << "invalid config.ini: " << frontend_config.error << '\n';
    return 2;
  }
  if (core_mode.empty())
    core_mode = frontend_config.cpu_core;
#if defined(_WIN32)
  _putenv_s("MODERNGEKKO_STATICRECOMP", core_mode == "static" ? "1" : "0");
#else
  setenv("MODERNGEKKO_STATICRECOMP", core_mode == "static" ? "1" : "0", 1);
#endif
  // JIT mode runs the game's DOL through Dolphin's JIT; the runtime needs the
  // explicit interpreter permission when no static module is supplied.
  if (core_mode != "static")
    config.allow_interpreter = true;
  config.graphics.internal_resolution_scale = frontend_config.dolphin_scale;
  config.graphics.msaa = frontend_config.msaa;
  config.graphics.ssaa = frontend_config.ssaa;
  config.graphics.anisotropy = frontend_config.anisotropy;
  config.graphics.vsync = frontend_config.vsync;
  config.graphics.aspect = frontend_config.aspect;
  config.graphics.widescreen_hack = frontend_config.widescreen_hack;
  config.graphics.efb_access = frontend_config.efb_access;
  config.graphics.emulation_speed = frontend_config.emulation_speed;
  config.audio.mute = config.audio.mute || frontend_config.mute;
  config.audio.volume = frontend_config.volume;
  if (config.graphics.backend.empty())
    config.graphics.backend = frontend_config.graphics_backend;
  config.fullscreen = frontend_config.fullscreen;
  config.show_fps_in_title = frontend_config.show_fps_in_title;
  if (use_default_mods) {
    config.mod_directories.push_back(executable_directory / "Mods");
    config.mod_directories.push_back(config.user_directory / "Mods");
  }

  if (!netplay_role) {
    using moderngekko::frontend::ControllerClass;
    ControllerClass controller_class = ControllerClass::Auto;
    std::string preferred_device = frontend_config.controller;
    const std::string &class_value =
        controller_layout_override.empty()
            ? frontend_config.controller_class
            : controller_layout_override;
    const std::string class_lower = [&] {
      std::string value = class_value;
      for (char &c : value)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      return value;
    }();
    if (class_lower == "keyboard") {
      preferred_device = "DInput/0/Keyboard Mouse";
    } else if (!moderngekko::frontend::ParseControllerClass(class_lower,
                                                            &controller_class)) {
      controller_class = ControllerClass::Auto;
    }
    config.controller_class = class_lower.empty() ? "auto" : class_lower;
    EnsureControllerSetup(config.user_directory, config.controller_class,
                          preferred_device);
  }

#ifdef MODERNGEKKO_DOL_PATCH_MANIFEST
  bool dol_changed = false;
  std::string dol_patch_error;
  if (!moderngekko::frontend::ApplyDolPatchManifest(
          config.game_root / "sys" / "main.dol",
          executable_directory / MODERNGEKKO_DOL_PATCH_MANIFEST, &dol_changed,
          &dol_patch_error)) {
    std::cerr << "DOL patching failed: " << dol_patch_error << '\n';
    return 2;
  }
  if (dol_changed)
    std::cout << "Applied native DOL patches\n";
#endif
  const auto inspected = moderngekko::InspectGame(config.game_root);
  if (!inspected) {
    std::cerr << "invalid game: " << inspected.error << '\n';
    return 2;
  }

#ifdef MODERNGEKKO_REQUIRED_DISC_ID
  if (inspected.metadata->disc_id != MODERNGEKKO_REQUIRED_DISC_ID) {
    std::cerr << "unsupported disc ID: expected "
              << MODERNGEKKO_REQUIRED_DISC_ID << ", got "
              << inspected.metadata->disc_id << '\n';
    return 2;
  }
#endif
#ifdef MODERNGEKKO_REQUIRED_DOL_SHA256
  if (inspected.metadata->dol_sha256 != MODERNGEKKO_REQUIRED_DOL_SHA256) {
    std::cerr << "unsupported main DOL: this release requires its pinned game build\n";
    return 2;
  }
#endif
#ifdef MODERNGEKKO_REQUIRED_REL_SHA256
  if (inspected.metadata->rel_sha256 != MODERNGEKKO_REQUIRED_REL_SHA256) {
    std::cerr << "unsupported _Main.rel: this release requires its pinned game build\n";
    return 2;
  }
#endif
#ifdef MODERNGEKKO_REQUIRED_ASSETS_SHA256
  if (inspected.metadata->assets_sha256 !=
      MODERNGEKKO_REQUIRED_ASSETS_SHA256) {
    std::cerr << "unsupported game assets: this release requires its pinned game build\n";
    return 2;
  }
#endif

  // Compatibility discovery belongs to the runner, never the runtime library.
  // JIT mode deliberately ignores the static module: SelectCPUCore() chooses
  // Dolphin's JIT64/JITArm64 core from MODERNGEKKO_STATICRECOMP=0.
  if (core_mode != "jit" && module_path.empty()) {
    if (const char *env = std::getenv("STATICRECOMP_MODULE"))
      module_path = env;
    else {
      const std::string module_name =
          "g" + inspected.metadata->disc_id + "_recomp" + LibrarySuffix();
      const auto bundled = executable_directory / module_name;
      const auto user_module =
          config.user_directory / "StaticRecompModules" / module_name;
      if (std::filesystem::is_regular_file(bundled))
        module_path = bundled;
      else if (std::filesystem::is_regular_file(user_module))
        module_path = user_module;
    }
  }
  if (core_mode != "jit" && !module_path.empty())
    config.module =
        moderngekko::ModuleSource::DynamicPath(std::move(module_path));

#if defined(__linux__) || defined(_WIN32)
  if (!config.headless && config.graphics.backend.empty())
    config.graphics.backend = "Vulkan";
#endif

  if (netplay_role) {
    moderngekko::frontend::NetplayOptions options;
    options.role = *netplay_role;
    options.address = netplay_address.empty() ? frontend_config.netplay_address
                                              : netplay_address;
    options.port = netplay_port.value_or(frontend_config.netplay_port);
    options.nickname = netplay_nickname.empty()
                           ? frontend_config.netplay_nickname
                           : netplay_nickname;
    options.buffer = netplay_buffer.empty() ? frontend_config.netplay_buffer
                                            : netplay_buffer;
    const std::vector<std::string> configured_controllers =
        moderngekko::frontend::ReadConfiguredControllers(config.user_directory);
    options.controllers =
        netplay_controllers.empty()
            ? (configured_controllers.empty() ? frontend_config.controllers
                                              : configured_controllers)
            : netplay_controllers;
    if (options.controllers.empty() && !frontend_config.controller.empty())
      options.controllers.push_back(frontend_config.controller);
    if (options.controllers.empty()) {
      std::cerr << "netplay requires at least one selected controller\n";
      return 2;
    }
    frontend_config.netplay_address = options.address;
    frontend_config.netplay_port = options.port;
    frontend_config.netplay_nickname = options.nickname;
    frontend_config.netplay_buffer = options.buffer;
    frontend_config.controllers = options.controllers;
    frontend_config.controller = options.controllers.front();
    std::string controller_message;
    if (!moderngekko::frontend::EnsureCanonicalPadProfile(
            config.user_directory, options.controllers.front(),
            &controller_message)) {
      std::cerr << "controller configuration: " << controller_message << '\n';
      return 2;
    }
    std::string save_error;
    if (!moderngekko::frontend::SaveConfig(config.user_directory,
                                           frontend_config, &save_error)) {
      std::cerr << "configuration: " << save_error << '\n';
      return 2;
    }
    std::cerr << "netplay: runner started\n";
    return moderngekko::frontend::RunNetplayLobby(
        std::move(config), std::move(frontend_config), std::move(options));
  }

  // Slot A as a raw memory card, slot B empty, on every entry path (launcher,
  // RecompLauncher manifest or a bare runner). Same keys the launcher writes.
  {
    bool memcard_changed = false;
    if (moderngekko::frontend::EnsureMemoryCardSlots(config.user_directory,
                                                    &memcard_changed) &&
        memcard_changed)
      std::cerr << "memory card: set SlotA=1 (raw card), SlotB=255 (None)\n";
  }

  auto created = moderngekko::Runtime::Create(std::move(config));
  if (!created) {
    std::cerr << "initialization failed: " << created.error->message << '\n';
    return 1;
  }
  std::cout << "audio backend: " << created.runtime->GetConfig().audio.backend
            << '\n';

  std::signal(SIGINT, HandleStopSignal);
  std::signal(SIGTERM, HandleStopSignal);
  std::jthread signal_watcher([&](std::stop_token stop_token) {
    while (!stop_token.stop_requested()) {
      if (s_stop_requested) {
        s_stop_requested = 0;
        created.runtime->RequestStop();
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  });
  const moderngekko::RuntimeRunResult result = created.runtime->Run();
  signal_watcher.request_stop();
  if (result.error) {
    std::cerr << "runtime failed: " << result.error->message << '\n';
    return 1;
  }
  return 0;
}

#if defined(_WIN32)
// Optionally confine the process to the cores that share the largest L3.
//
// The emulated CPU is a single serial instruction stream, so nothing here is
// about parallelism -- core usage stays at 1.00 either way. It is about cache
// residency. A recompiled module is one dispatch switch spanning the whole
// game, which lives or dies on staying in L3, and on a chip with 3D V-Cache on
// one CCD only, Windows migrating the thread between dies makes it repeatedly
// lose its working set. Measured on a 9950X3D: 55.41 -> 70.42 fps, +27%.
//
// This applies to every Dolphin thread, not only the emulated CPU thread, so it
// is opt-in. Set MODERNGEKKO_CACHE_AFFINITY=1 to enable it after benchmarking
// the target machine.
void PinProcessToLargestCache() {
  if (!moderngekko::frontend::AffinityEnabled(
          std::getenv("MODERNGEKKO_CACHE_AFFINITY")))
    return;

  DWORD length = 0;
  GetLogicalProcessorInformationEx(RelationCache, nullptr, &length);
  if (length == 0)
    return;
  std::vector<char> buffer(length);
  if (!GetLogicalProcessorInformationEx(
          RelationCache,
          reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &length))
    return;

  const moderngekko::frontend::CacheDomain domain =
      moderngekko::frontend::LargestSharedCache(buffer.data(), length);
  if (!domain)
    return;

  if (moderngekko::frontend::ApplyCacheDomain(GetCurrentProcess(), domain).affinity_set) {
    std::cout << "[perf] pinned to the cores sharing the largest L3 (" << (domain.size >> 20)
              << " MB), mask 0x" << std::hex << static_cast<unsigned long long>(domain.mask)
              << std::dec << '\n';
  }
}
#endif

int main(int argc, char **argv) {
  try {
#if defined(_WIN32)
    if (HWND console = GetConsoleWindow())
      ShowWindow(console, SW_HIDE);
    PinProcessToLargestCache();
#endif
    return RunMain(argc, argv);
  } catch (const std::exception &error) {
    std::cerr << "fatal error: " << error.what() << '\n';
  } catch (...) {
    std::cerr << "fatal error: unknown exception\n";
  }
  return 1;
}

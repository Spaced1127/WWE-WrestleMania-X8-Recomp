#include "frontend_config.hpp"
#include "dol_patch.hpp"
#include "launcher_savestates.hpp"
#include "moderngekko/game.hpp"
#include "netplay_session.hpp"

#include "Common/Image.h"
#include "DiscIO/DiscExtractor.h"
#include "DiscIO/Filesystem.h"
#include "DiscIO/Volume.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace
{
#ifndef MODERNGEKKO_FRONTEND_NAME
#define MODERNGEKKO_FRONTEND_NAME "ModernGekko"
#endif

#ifndef MODERNGEKKO_RUNNER_FILENAME
#define MODERNGEKKO_RUNNER_FILENAME "moderngekko-run"
#endif

#ifndef MODERNGEKKO_USER_DIRECTORY_NAME
#define MODERNGEKKO_USER_DIRECTORY_NAME "moderngekko"
#endif

#ifndef MODERNGEKKO_LOG_FILENAME
#define MODERNGEKKO_LOG_FILENAME "ModernGekko.log"
#endif

#ifdef MODERNGEKKO_GAMECUBE_CONTROLLERS
constexpr std::string_view CONTROLLER_PROFILE_NAME = "GCPadNew.ini";
#else
constexpr std::string_view CONTROLLER_PROFILE_NAME = "WiimoteNew.ini";
#endif

#ifndef WWE_RECOMP_VERSION
#define WWE_RECOMP_VERSION "0.1.4"
#endif

struct LauncherTheme
{
  std::string product = "WWE WrestleMania X8";
  std::string subtitle = "STATIC RECOMPILATION";
  std::string disc_id = "GW3E78";
  std::string version = WWE_RECOMP_VERSION;
  std::string backdrop = "arena";
  std::string background;
  ImVec4 accent = ImVec4(0.98f, 0.85f, 0.20f, 1.0f);
};

struct HostStatus
{
  bool shader_cache = false;
  bool high_priority = false;
  bool affinity = false;
  bool dsp_rom = false;
  bool dsp_coef = false;
  std::string affinity_text = "default";
};

inline std::string TrimText(std::string value)
{
  const auto not_space = [](unsigned char c) { return std::isspace(c) == 0; };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

inline LauncherTheme LoadLauncherTheme(const fs::path& release_directory,
                                       const fs::path& user_directory)
{
  LauncherTheme theme;
  fs::path path = release_directory / "theme.ini";
  if (!fs::is_regular_file(path))
    path = user_directory / "theme.ini";
  std::ifstream input(path);
  if (!input)
    return theme;
  std::string line;
  while (std::getline(input, line))
  {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    const std::string trimmed = TrimText(line);
    if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == '[')
      continue;
    const std::size_t equals = trimmed.find('=');
    if (equals == std::string::npos)
      continue;
    const std::string key = TrimText(trimmed.substr(0, equals));
    const std::string value = TrimText(trimmed.substr(equals + 1));
    if (key == "product" && !value.empty())
      theme.product = value;
    else if (key == "subtitle" && !value.empty())
      theme.subtitle = value;
    else if (key == "disc_id" && !value.empty())
      theme.disc_id = value;
    else if (key == "version" && !value.empty())
      theme.version = value;
    else if (key == "backdrop" && !value.empty())
      theme.backdrop = value;
    else if (key == "background")
      theme.background = value;
    else if (key == "accent")
    {
      int r = 0, g = 0, b = 0;
      if (std::sscanf(value.c_str(), "%d,%d,%d", &r, &g, &b) == 3)
      {
        theme.accent = ImVec4(static_cast<float>(std::clamp(r, 0, 255)) / 255.0f,
                              static_cast<float>(std::clamp(g, 0, 255)) / 255.0f,
                              static_cast<float>(std::clamp(b, 0, 255)) / 255.0f, 1.0f);
      }
    }
  }
  return theme;
}

inline HostStatus PrepareHost(const fs::path& user_directory,
                              const fs::path& release_directory)
{
  HostStatus status;
  std::error_code ec;
  const fs::path shader_cache = user_directory / "Cache" / "Shaders";
  fs::create_directories(shader_cache, ec);
  status.shader_cache = fs::is_directory(shader_cache, ec) && !ec;
#if defined(_WIN32)
  status.high_priority = SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS) != 0;

  DWORD length = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
  if (length != 0)
  {
    std::vector<char> buffer(length);
    if (GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &length))
    {
      KAFFINITY mask = 0;
      for (std::size_t offset = 0;
           offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) <= length;)
      {
        const auto* entry = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
            buffer.data() + offset);
        if (entry->Size == 0)
          break;
        if (entry->Relationship == RelationProcessorCore)
        {
          const KAFFINITY core = entry->Processor.GroupMask[0].Mask;
          if (core != 0)
            mask |= core & (~core + 1);
        }
        offset += entry->Size;
      }
      if (mask != 0)
      {
        status.affinity = SetProcessAffinityMask(GetCurrentProcess(), mask) != 0;
        char text[32]{};
        std::snprintf(text, sizeof(text), "0x%llX",
                      static_cast<unsigned long long>(mask));
        status.affinity_text = text;
      }
    }
  }
#else
  status.affinity_text = "n/a";
#endif
  const fs::path dsp_rom = release_directory / "Sys" / "GC" / "dsp_rom.bin";
  const fs::path dsp_coef = release_directory / "Sys" / "GC" / "dsp_coef.bin";
  status.dsp_rom = fs::is_regular_file(dsp_rom, ec) && !ec && fs::file_size(dsp_rom, ec) > 0;
  status.dsp_coef =
      fs::is_regular_file(dsp_coef, ec) && !ec && fs::file_size(dsp_coef, ec) > 0;
  return status;
}

inline std::string ReadLogTail(const fs::path& path, int max_lines)
{
  std::ifstream input(path);
  if (!input)
    return {};
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(input, line))
  {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    lines.push_back(std::move(line));
    if (lines.size() > 4000)
      lines.erase(lines.begin(), lines.begin() + 1000);
  }
  const std::size_t start =
      lines.size() > static_cast<std::size_t>(max_lines)
          ? lines.size() - static_cast<std::size_t>(max_lines)
          : 0;
  std::string out;
  for (std::size_t i = start; i < lines.size(); ++i)
    out += lines[i] + '\n';
  return out;
}

inline std::string FindVideoLine(const std::string& log)
{
  const std::size_t marker = log.find("[video]");
  if (marker == std::string::npos)
    return {};
  const std::size_t end = log.find('\n', marker);
  return log.substr(marker, end == std::string::npos ? std::string::npos : end - marker);
}

inline void OpenFolder(const fs::path& path)
{
  std::error_code ec;
  fs::create_directories(path, ec);
#if defined(_WIN32)
  ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE__)
  std::system(("open \"" + path.string() + "\"").c_str());
#else
  std::system(("xdg-open \"" + path.string() + "\"").c_str());
#endif
}

struct LauncherArtwork
{
  SDL_Texture* background = nullptr;
  int background_width = 0;
  int background_height = 0;

  ~LauncherArtwork()
  {
    if (background)
      SDL_DestroyTexture(background);
  }
};

inline SDL_Texture* LoadPngTexture(SDL_Renderer* renderer, const fs::path& path, int* width_out,
                                   int* height_out)
{
  std::ifstream input(path, std::ios::binary);
  if (!input)
    return nullptr;
  std::vector<u8> bytes((std::istreambuf_iterator<char>(input)),
                        std::istreambuf_iterator<char>());
  if (bytes.empty())
    return nullptr;

  Common::UniqueBuffer<u8> pixels;
  u32 width = 0;
  u32 height = 0;
  if (!Common::LoadPNG(std::span<const u8>(bytes), &pixels, &width, &height) || pixels.empty())
    return nullptr;

  SDL_Texture* texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                        static_cast<int>(width), static_cast<int>(height));
  if (!texture)
    return nullptr;
  if (!SDL_UpdateTexture(texture, nullptr, pixels.data(), static_cast<int>(width) * 4))
  {
    SDL_DestroyTexture(texture);
    return nullptr;
  }
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  *width_out = static_cast<int>(width);
  *height_out = static_cast<int>(height);
  return texture;
}

inline fs::path FolderCardPath(const fs::path& user_directory)
{
  return user_directory / "GC" / "USA" / "Card A";
}

// Slot A is a raw memory card (Core/SlotA = 1), so the game's save is this
// file. The GCI folder above is only read when a savestate restores the old
// folder device.
inline fs::path RawCardPath(const fs::path& user_directory)
{
  return user_directory / "GC" / "MemoryCardA.USA.raw";
}

inline fs::path BackupFolderCard(const fs::path& user_directory)
{
  const fs::path raw_card = RawCardPath(user_directory);
  const fs::path folder_card = FolderCardPath(user_directory);
  const bool have_raw = fs::is_regular_file(raw_card);
  const bool have_folder = fs::is_directory(folder_card);
  if (!have_raw && !have_folder)
    return {};
  const std::time_t now = std::time(nullptr);
  std::tm local{};
#if defined(_WIN32)
  localtime_s(&local, &now);
#else
  localtime_r(&now, &local);
#endif
  char stamp[32]{};
  std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &local);
  const fs::path backup =
      user_directory / "Backups" / (std::string("CardA-") + stamp);
  std::error_code ec;
  fs::create_directories(backup.parent_path(), ec);
  if (have_raw)
    fs::copy_file(raw_card, backup / raw_card.filename(),
                  fs::copy_options::overwrite_existing, ec);
  if (have_folder)
    fs::copy(folder_card, backup / "Card A",
             fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
  return ec ? fs::path{} : backup;
}

struct ExtractionState
{
  std::atomic<bool> running{false};
  std::atomic<unsigned> completed{0};
  std::atomic<unsigned> total{1};
  std::mutex mutex;
  std::string status;
  std::string error;
  std::optional<fs::path> finished_game;
  // Set by the --extract command line so RecompLauncher can follow the setup
  // step. The GUI leaves this off.
  bool report_progress = false;
};

struct DialogState
{
  std::mutex mutex;
  std::optional<fs::path> selected;
  std::optional<fs::path> selected_folder;
  std::string error;
};

struct ControllerOption
{
  std::string label;
  std::string device;
  std::string guid;
  std::string mapping;
};

constexpr moderngekko::frontend::ControllerClass kControllerLayouts[] = {
    moderngekko::frontend::ControllerClass::Auto,
    moderngekko::frontend::ControllerClass::Positional,
    moderngekko::frontend::ControllerClass::Nintendo,
};

constexpr const char* kControllerLayoutLabels[] = {
    "Auto (per pad type)",
    "Positional (Xbox / PlayStation)",
    "Nintendo (A/B swap)",
};

constexpr const char* kControllerLayoutShortLabels[] = {
    "Auto",
    "Positional",
    "Nintendo",
};

constexpr int kControllerLayoutCount =
    static_cast<int>(std::size(kControllerLayouts));

constexpr std::string_view KEYBOARD_DEVICE = "DInput/0/Keyboard Mouse";

std::string RequiredGameError(const moderngekko::GameMetadata& metadata)
{
#ifdef MODERNGEKKO_REQUIRED_DISC_ID
  if (metadata.disc_id != MODERNGEKKO_REQUIRED_DISC_ID)
    return "expected disc ID " MODERNGEKKO_REQUIRED_DISC_ID ", found " + metadata.disc_id;
#endif
#ifdef MODERNGEKKO_REQUIRED_DOL_SHA256
  if (metadata.dol_sha256 != MODERNGEKKO_REQUIRED_DOL_SHA256)
    return "main DOL does not match the pinned release";
#endif
#ifdef MODERNGEKKO_REQUIRED_REL_SHA256
  if (metadata.rel_sha256 != MODERNGEKKO_REQUIRED_REL_SHA256)
    return "_Main.rel does not match the pinned release";
#endif
#ifdef MODERNGEKKO_REQUIRED_ASSETS_SHA256
  if (metadata.assets_sha256 != MODERNGEKKO_REQUIRED_ASSETS_SHA256)
    return "game assets do not match the pinned release";
#endif
  return {};
}

std::vector<ControllerOption> EnumerateControllers()
{
  std::vector<ControllerOption> result;
  std::unordered_map<std::string, int> device_ids;
  int count = 0;
  SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&count);
  for (int i = 0; i < count; ++i)
  {
    const SDL_JoystickID joystick_id = joystick_ids[i];
    const bool gamepad = SDL_IsGamepad(joystick_id);
    const char* name_value =
        gamepad ? SDL_GetGamepadNameForID(joystick_id) : SDL_GetJoystickNameForID(joystick_id);
    const std::string name = name_value && *name_value ? name_value : "Unknown Controller";
    const int device_id = device_ids[name]++;
    if (!gamepad)
      continue;
    ControllerOption option;
    option.label = device_id == 0 ? name : name + " (" + std::to_string(device_id + 1) + ")";
    option.device = "SDL/" + std::to_string(device_id) + "/" + name;
    const SDL_GUID guid = SDL_GetGamepadGUIDForID(joystick_id);
    char guid_text[33]{};
    SDL_GUIDToString(guid, guid_text, static_cast<int>(sizeof(guid_text)));
    option.guid = guid_text;
    std::cerr << "controller: name=\"" << name << "\" guid=" << option.guid
              << " class=auto\n";
    result.emplace_back(std::move(option));
  }
  SDL_free(joystick_ids);

  // Keyboard is always available, even with no gamepad connected. The device
  // string is Dolphin's keyboard/mouse device; the layout selector writes the
  // matching key bindings instead of the gamepad mapping.
  ControllerOption keyboard;
  keyboard.label = "Keyboard (WASD)";
  keyboard.device = std::string(KEYBOARD_DEVICE);
  result.emplace_back(std::move(keyboard));
  return result;
}

int FindController(const std::vector<ControllerOption>& controllers, std::string_view device)
{
  const auto found = std::ranges::find(controllers, device, &ControllerOption::device);
  return found == controllers.end() ? -1 : static_cast<int>(found - controllers.begin());
}

fs::path DefaultUserDirectory()
{
#ifdef MODERNGEKKO_USER_DIRECTORY_IN_DOCUMENTS
#if defined(_WIN32)
  if (const char* user_profile = std::getenv("USERPROFILE"))
    return fs::path(user_profile) / "Documents" / MODERNGEKKO_USER_DIRECTORY_NAME;
#endif
  if (const char* home = std::getenv("HOME"))
    return fs::path(home) / "Documents" / MODERNGEKKO_USER_DIRECTORY_NAME;
#endif
#if defined(_WIN32)
  if (const char* local_app_data = std::getenv("LOCALAPPDATA"))
    return fs::path(local_app_data) / MODERNGEKKO_USER_DIRECTORY_NAME;
#endif
  if (const char* xdg = std::getenv("XDG_DATA_HOME"))
    return fs::path(xdg) / MODERNGEKKO_USER_DIRECTORY_NAME;
  if (const char* home = std::getenv("HOME"))
    return fs::path(home) / ".local/share" / MODERNGEKKO_USER_DIRECTORY_NAME;
  return std::string(MODERNGEKKO_USER_DIRECTORY_NAME) + "-user";
}

fs::path DocumentsDirectory()
{
#if defined(_WIN32)
  if (const char* user_profile = std::getenv("USERPROFILE"))
    return fs::path(user_profile) / "Documents";
#endif
  if (const char* home = std::getenv("HOME"))
    return fs::path(home) / "Documents";
  return fs::current_path();
}

fs::path ExecutableDirectory(const char* argv0)
{
  std::error_code ec;
#if defined(__linux__)
  const fs::path proc_executable = fs::read_symlink("/proc/self/exe", ec);
  if (!ec)
    return proc_executable.parent_path();
  ec.clear();
#endif
  const fs::path executable = fs::weakly_canonical(argv0, ec);
  return ec ? fs::current_path() : executable.parent_path();
}

fs::path SiblingExecutable(const fs::path& release_directory, std::string name)
{
#if defined(_WIN32)
  name += ".exe";
#endif
  const fs::path sibling = release_directory / name;
  return fs::is_regular_file(sibling) ? sibling : fs::path(name);
}

bool ApplyBundledDolPatch(const fs::path& game_root, const fs::path& release_directory,
                          bool* changed, std::string* error)
{
#ifdef MODERNGEKKO_DOL_PATCH_MANIFEST
  if (game_root.empty() || !fs::is_directory(game_root))
  {
    *changed = false;
    return true;
  }
  return moderngekko::frontend::ApplyDolPatchManifest(
      game_root / "sys" / "main.dol", release_directory / MODERNGEKKO_DOL_PATCH_MANIFEST,
      changed, error);
#else
  *changed = false;
  return true;
#endif
}

fs::path DefaultGameFile(const fs::path& user_directory, const fs::path& release_directory)
{
#ifdef MODERNGEKKO_PORTABLE_DEFAULT_GAME
  return release_directory / "default-game.txt";
#else
  return user_directory / "default-game.txt";
#endif
}

fs::path ReadDefaultGame(const fs::path& user_directory, const fs::path& release_directory)
{
  std::ifstream file(DefaultGameFile(user_directory, release_directory));
  std::string value;
  std::getline(file, value);
  if (!value.empty() && value.back() == '\r')
    value.pop_back();
  fs::path game(value);
  if (game.is_relative())
    game = release_directory / game;
  return game;
}

bool WriteDefaultGame(const fs::path& user_directory, const fs::path& release_directory,
                      const fs::path& game, std::string* error)
{
  const fs::path destination = DefaultGameFile(user_directory, release_directory);
  std::error_code ec;
  fs::create_directories(destination.parent_path(), ec);
  std::ofstream file(destination, std::ios::trunc);
  if (!file)
  {
    if (error)
      *error = "can't save default-game.txt";
    return false;
  }
  fs::path stored = game;
#ifdef MODERNGEKKO_PORTABLE_DEFAULT_GAME
  const fs::path relative = fs::relative(game, release_directory, ec);
  if (!ec && !relative.empty())
    stored = relative;
#endif
  file << stored.string() << '\n';
  return true;
}

std::vector<fs::path> FindDiscImages()
{
  std::vector<fs::path> images;
  std::error_code ec;
  const fs::path documents = DocumentsDirectory();
  if (!fs::is_directory(documents, ec))
    return images;
  fs::recursive_directory_iterator iterator(documents,
                                            fs::directory_options::skip_permission_denied, ec);
  const fs::recursive_directory_iterator end;
  while (iterator != end)
  {
    if (iterator.depth() > 4)
      iterator.disable_recursion_pending();
    if (iterator->is_regular_file(ec))
    {
      std::string extension = iterator->path().extension().string();
      std::ranges::transform(extension, extension.begin(),
                             [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      if (extension == ".wbfs" || extension == ".iso" || extension == ".rvz")
        images.push_back(iterator->path());
    }
    iterator.increment(ec);
    if (ec)
      ec.clear();
  }
  std::ranges::sort(images);
  return images;
}

std::optional<fs::path> PrepareDisc(const fs::path& image, const fs::path& user_directory,
                                    const fs::path& release_directory, ExtractionState* state,
                                    std::string* error)
{
  std::unique_ptr<DiscIO::Volume> source = DiscIO::CreateVolume(image.string());
  if (!source)
  {
    *error = "Dolphin rejected the selected disc image";
    return std::nullopt;
  }
  const std::string source_id = source->GetGameID(source->GetGamePartition());
#ifdef MODERNGEKKO_REQUIRED_DISC_ID
  if (source_id == MODERNGEKKO_REQUIRED_DISC_ID)
    return image;
#endif
#if defined(MODERNGEKKO_DISC_PREPARER_FILENAME) && \
    defined(MODERNGEKKO_ACCEPTED_SOURCE_DISC_ID) && defined(MODERNGEKKO_REQUIRED_DISC_ID)
  if (source_id != MODERNGEKKO_ACCEPTED_SOURCE_DISC_ID)
  {
    *error = "expected clean " MODERNGEKKO_ACCEPTED_SOURCE_DISC_ID " or patched "
             MODERNGEKKO_REQUIRED_DISC_ID "; selected " + source_id;
    return std::nullopt;
  }
  const fs::path output = user_directory / "Setup" / "SonicRidersTE-2.4.6.1.iso";
  std::error_code ec;
  fs::create_directories(output.parent_path(), ec);
  if (ec)
  {
    *error = "can't create setup directory: " + ec.message();
    return std::nullopt;
  }
  {
    std::lock_guard lock(state->mutex);
    state->status = "Applying Tournament Edition 2.4.6.1";
  }
  const fs::path preparer =
      SiblingExecutable(release_directory, MODERNGEKKO_DISC_PREPARER_FILENAME);
  const std::array<std::string, 5> storage = {
      preparer.string(), "--input", image.string(), "--output", output.string()};
  std::array<const char*, 6> arguments{};
  for (std::size_t i = 0; i < storage.size(); ++i)
    arguments[i] = storage[i].c_str();
  SDL_Process* process = SDL_CreateProcess(arguments.data(), false);
  if (!process)
  {
    *error = "can't start Tournament Edition patcher: " + std::string(SDL_GetError());
    return std::nullopt;
  }
  int exit_code = 1;
  const bool waited = SDL_WaitProcess(process, true, &exit_code);
  SDL_DestroyProcess(process);
  if (!waited || exit_code != 0)
  {
    *error = "Tournament Edition patching failed";
    return std::nullopt;
  }
  return output;
#else
  *error = "expected disc ID " MODERNGEKKO_REQUIRED_DISC_ID "; selected " + source_id;
  return std::nullopt;
#endif
}

bool ExtractDisc(const fs::path& image, const fs::path& user_directory,
                 const fs::path& release_directory, ExtractionState* state)
{
  const auto report = [state](double progress, const std::string& text)
  {
    if (!state->report_progress)
      return;
    std::cout << "progress " << std::fixed << std::setprecision(3) << progress
              << ' ' << text << '\n'
              << std::flush;
  };

  auto fail = [&](std::string message)
  {
    report(0.0, message);
    std::lock_guard lock(state->mutex);
    state->error = std::move(message);
    state->running = false;
    return false;
  };

  {
    std::lock_guard lock(state->mutex);
    state->status = "Opening " + image.filename().string();
  }
  report(0.0, "Opening " + image.filename().string());
  std::string preparation_error;
  const std::optional<fs::path> prepared =
      PrepareDisc(image, user_directory, release_directory, state, &preparation_error);
  if (!prepared)
    return fail(std::move(preparation_error));
  std::unique_ptr<DiscIO::Volume> volume = DiscIO::CreateVolume(prepared->string());
  if (!volume)
    return fail("Dolphin rejected the prepared disc image");

  const DiscIO::Partition partition = volume->GetGamePartition();
  const DiscIO::FileSystem* filesystem = volume->GetFileSystem(partition);
  if (!filesystem || !filesystem->IsValid())
    return fail("Dolphin could not read the game partition filesystem");

  std::string disc_id = volume->GetGameID(partition);
  if (disc_id.size() != 6)
    return fail("the selected image has an invalid disc ID");
#ifdef MODERNGEKKO_REQUIRED_DISC_ID
  if (disc_id != MODERNGEKKO_REQUIRED_DISC_ID)
    return fail("this frontend requires disc ID " MODERNGEKKO_REQUIRED_DISC_ID "; selected " +
                disc_id);
#endif
#ifdef MODERNGEKKO_PORTABLE_DEFAULT_GAME
  const fs::path games_directory = release_directory;
  const fs::path output = games_directory / "Game";
#else
  const fs::path games_directory = user_directory / "games";
  const fs::path output = games_directory / disc_id;
#endif
  std::error_code ec;
  if (fs::is_directory(output, ec))
  {
    bool changed = false;
    std::string patch_error;
    if (!ApplyBundledDolPatch(output, release_directory, &changed, &patch_error))
      return fail("existing extraction patching failed: " + patch_error);
  }
  const auto existing = moderngekko::InspectGame(output);
  if (existing && RequiredGameError(*existing.metadata).empty())
  {
    std::string error;
    if (!WriteDefaultGame(user_directory, release_directory, output, &error))
      return fail(error);
    std::lock_guard lock(state->mutex);
    state->finished_game = output;
    state->status = "Using existing extraction";
    state->running = false;
    report(1.0, "Using existing extraction");
    return true;
  }

  const fs::path staging = games_directory / (disc_id + ".extracting");
  fs::remove_all(staging, ec);
  fs::create_directories(staging / "files", ec);
  if (ec)
    return fail("can't create extraction directory: " + ec.message());

  {
    std::lock_guard lock(state->mutex);
    state->status = "Extracting system data";
  }
  if (!DiscIO::ExportSystemData(*volume, partition, staging.string()))
  {
    fs::remove_all(staging, ec);
    return fail("Dolphin failed while extracting the disc system data");
  }

  state->total = std::max(1u, filesystem->GetRoot().GetTotalChildren());
  {
    std::lock_guard lock(state->mutex);
    state->status = "Extracting game files";
  }
  report(0.05, "Extracting game files");
  DiscIO::ExportDirectory(*volume, partition, filesystem->GetRoot(), true, "",
                          (staging / "files").string(),
                          [state, &report](const std::string& path)
                          {
                            ++state->completed;
                            std::string status;
                            {
                              std::lock_guard lock(state->mutex);
                              state->status = "Extracting " + path;
                              status = state->status;
                            }
                            const double progress =
                                0.05 + 0.90 * static_cast<double>(state->completed.load()) /
                                           static_cast<double>(state->total.load());
                            report(progress, status);
                            return false;
                          });

  bool dol_changed = false;
  std::string dol_patch_error;
  if (!ApplyBundledDolPatch(staging, release_directory, &dol_changed, &dol_patch_error))
  {
    fs::remove_all(staging, ec);
    return fail("extracted DOL patching failed: " + dol_patch_error);
  }

  report(0.96, "Verifying extracted files");
  const auto inspected = moderngekko::InspectGame(staging);
  if (!inspected)
  {
    fs::remove_all(staging, ec);
    return fail("extracted game validation failed: " + inspected.error);
  }
  if (const std::string identity_error = RequiredGameError(*inspected.metadata);
      !identity_error.empty())
  {
    fs::remove_all(staging, ec);
    return fail("extracted game validation failed: " + identity_error);
  }

  fs::remove_all(output, ec);
  ec.clear();
  fs::rename(staging, output, ec);
  if (ec)
    return fail("can't publish extracted game: " + ec.message());
  std::string error;
  if (!WriteDefaultGame(user_directory, release_directory, output, &error))
    return fail(error);

  {
    std::lock_guard lock(state->mutex);
    state->finished_game = output;
    state->status = "Extraction complete";
    report(1.0, "Extraction complete");
  }
  state->running = false;
  return true;
}

void SDLCALL FileDialogCallback(void* userdata, const char* const* filelist, int)
{
  auto* state = static_cast<DialogState*>(userdata);
  std::lock_guard lock(state->mutex);
  if (!filelist)
    state->error = SDL_GetError();
  else if (filelist[0])
    state->selected = filelist[0];
}

void SDLCALL FolderDialogCallback(void* userdata, const char* const* filelist, int)
{
  auto* state = static_cast<DialogState*>(userdata);
  std::lock_guard lock(state->mutex);
  if (!filelist)
    state->error = SDL_GetError();
  else if (filelist[0])
    state->selected_folder = filelist[0];
}

fs::path SiblingRunner(const char* argv0)
{
  std::error_code ec;
  const fs::path self = fs::weakly_canonical(argv0, ec);
  fs::path runner = MODERNGEKKO_RUNNER_FILENAME;
#if defined(_WIN32)
  runner += ".exe";
#endif
  const fs::path sibling = self.parent_path() / runner;
  return fs::is_regular_file(sibling) ? sibling : runner;
}
} // namespace

int main(int argc, char** argv)
{
#if defined(_WIN32)
  if (HWND console = GetConsoleWindow())
    ShowWindow(console, SW_HIDE);
#endif
  bool use_wayland = false;
  bool auto_play = false;
  std::vector<std::string> config_overrides;
  std::optional<fs::path> extract_only;
  for (int i = 1; i < argc; ++i)
  {
    if (std::string_view(argv[i]) == "-X11" || std::string_view(argv[i]) == "--x11")
      use_wayland = false;
    else if (std::string_view(argv[i]) == "--wayland")
      use_wayland = true;
    else if (std::string_view(argv[i]) == "--play")
      auto_play = true;
    else if (std::string_view(argv[i]) == "--set" && i + 1 < argc)
      config_overrides.emplace_back(argv[++i]);
    else if (std::string_view(argv[i]) == "--extract" && i + 1 < argc)
      extract_only = argv[++i];
  }

  const fs::path user_directory = DefaultUserDirectory();
  const fs::path release_directory = ExecutableDirectory(argv[0]);
  // Slot A must be a raw memory card and slot B empty before the game boots;
  // Dolphin's default (GCI folder) needs GC/<region>/Card A and can hang the
  // slot-A probe. The runner enforces the same keys again before Runtime::Create.
  {
    bool memcard_changed = false;
    if (moderngekko::frontend::EnsureMemoryCardSlots(user_directory, &memcard_changed))
    {
      if (memcard_changed)
        std::cerr << "memory card: set SlotA=1 (raw card), SlotB=255 (None)\n";
    }
    else
    {
      std::cerr << "memory card: could not write Config/Dolphin.ini\n";
    }
  }
  if (extract_only)
  {
    ExtractionState extraction;
    extraction.running = true;
    extraction.report_progress = true;
    const bool success =
        ExtractDisc(*extract_only, user_directory, release_directory, &extraction);
    std::lock_guard lock(extraction.mutex);
    if (!success)
      std::cerr << "extraction failed: " << extraction.error << '\n';
    else
      std::cout << extraction.status << ": " << *extraction.finished_game << '\n';
    return success ? 0 : 1;
  }

  auto config = moderngekko::frontend::LoadConfig(user_directory, true);
  if (!config)
  {
    if (config_overrides.empty())
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                               "Invalid " MODERNGEKKO_FRONTEND_NAME " config.ini",
                               config.error.c_str(), nullptr);
    else
      std::cerr << "invalid config.ini: " << config.error << '\n';
    return 2;
  }

  if (!config_overrides.empty())
  {
    // Scriptable equivalent of the launcher settings panel: rewrite the keys
    // in config.ini, then reload through the same validating parser the GUI
    // uses, so a bad value is rejected exactly as it would be in the UI.
    const fs::path config_path = user_directory / "config.ini";
    std::vector<std::string> lines;
    {
      std::ifstream in(config_path);
      std::string line;
      while (std::getline(in, line))
      {
        if (!line.empty() && line.back() == '\r')
          line.pop_back();
        lines.push_back(line);
      }
    }
    for (const std::string& override_text : config_overrides)
    {
      const std::size_t eq = override_text.find('=');
      if (eq == std::string::npos || eq == 0)
      {
        std::cerr << "invalid --set; expected key=value\n";
        return 2;
      }
      const std::string key = override_text.substr(0, eq);
      const std::string value = override_text.substr(eq + 1);
      bool replaced = false;
      for (std::string& line : lines)
      {
        const std::size_t sep = line.find('=');
        if (sep != std::string::npos && line.substr(0, sep) == key)
        {
          line = key + "=" + value;
          replaced = true;
        }
      }
      if (!replaced)
      {
        const auto video = std::find(lines.begin(), lines.end(), "[Video]");
        if (video != lines.end())
          lines.insert(video + 1, key + "=" + value);
        else
          lines.push_back(key + "=" + value);
      }
    }
    std::string candidate;
    for (const std::string& line : lines)
      candidate += line + '\n';
    config = moderngekko::frontend::ParseConfig(candidate);
    if (!config)
    {
      std::cerr << "invalid config.ini: " << config.error << '\n';
      return 2;
    }
    {
      std::ofstream out(config_path, std::ios::trunc);
      for (const std::string& line : lines)
        out << line << '\n';
    }
    std::cout << "settings saved: " << config_path.string() << '\n';
    if (!auto_play)
      return 0;
  }

  const LauncherTheme theme = LoadLauncherTheme(release_directory, user_directory);
  const HostStatus host = PrepareHost(user_directory, release_directory);
  const fs::path launcher_log_path = user_directory / "Logs" / MODERNGEKKO_LOG_FILENAME;
  std::string log_video_line = FindVideoLine(ReadLogTail(launcher_log_path, 400));

#if defined(__linux__)
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, use_wayland ? "wayland" : "x11");
#endif
  SDL_setenv_unsafe("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD", "1", 0);
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    return 1;

  const float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  SDL_Window* window = SDL_CreateWindow(MODERNGEKKO_FRONTEND_NAME, static_cast<int>(1180 * scale),
                                        static_cast<int>(780 * scale),
                                        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if (!window)
  {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_Renderer* renderer = SDL_CreateRenderer(window, "software");
  if (!renderer)
    renderer = SDL_CreateRenderer(window, nullptr);
  if (!renderer)
  {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_SetRenderVSync(renderer, 1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.IniFilename = nullptr;
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowPadding = ImVec2(0, 0);
  style.FramePadding = ImVec2(14, 9);
  style.ItemSpacing = ImVec2(10, 10);
  style.ItemInnerSpacing = ImVec2(8, 6);
  style.FrameRounding = 8.0f;
  style.GrabRounding = 8.0f;
  style.ChildRounding = 14.0f;
  style.PopupRounding = 10.0f;
  style.ScrollbarRounding = 8.0f;
  style.WindowRounding = 0.0f;
  style.Colors[ImGuiCol_Text] = ImVec4(0.93f, 0.96f, 0.98f, 1.00f);
  style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.62f, 0.62f, 0.66f, 1.00f);
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  style.Colors[ImGuiCol_ChildBg] = ImVec4(0.035f, 0.040f, 0.055f, 0.88f);
  style.Colors[ImGuiCol_PopupBg] = ImVec4(0.04f, 0.045f, 0.06f, 0.98f);
  style.Colors[ImGuiCol_Border] = ImVec4(0.95f, 0.82f, 0.20f, 0.18f);
  style.Colors[ImGuiCol_FrameBg] = ImVec4(0.09f, 0.08f, 0.11f, 0.92f);
  style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.10f, 0.12f, 0.95f);
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.12f, 0.14f, 0.98f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.12f, 0.09f, 0.12f, 0.95f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.12f, 0.14f, 1.00f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.15f, 0.16f, 1.00f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.24f, 0.10f, 0.12f, 1.00f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.12f, 0.14f, 1.00f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.15f, 0.16f, 1.00f);
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.97f, 0.84f, 0.18f, 1.00f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.90f, 0.30f, 1.00f);
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.98f, 0.85f, 0.15f, 1.00f);
  style.Colors[ImGuiCol_Separator] = ImVec4(0.95f, 0.82f, 0.20f, 0.22f);
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.20f, 0.20f, 0.80f);
  style.ScaleAllSizes(scale);
  style.FontScaleDpi = scale;

  ImFont* font_ui = nullptr;
  ImFont* font_bold = nullptr;
#if defined(_WIN32)
  const char* regular_fonts[] = {"C:\\Windows\\Fonts\\segoeui.ttf",
                                 "C:\\Windows\\Fonts\\tahoma.ttf"};
  const char* bold_fonts[] = {"C:\\Windows\\Fonts\\segoeuib.ttf",
                              "C:\\Windows\\Fonts\\tahomabd.ttf"};
#elif defined(__APPLE__)
  const char* regular_fonts[] = {"/System/Library/Fonts/SFNS.ttf"};
  const char* bold_fonts[] = {"/System/Library/Fonts/SFNS.ttf"};
#else
  const char* regular_fonts[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"};
  const char* bold_fonts[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"};
#endif
  for (const char* path : regular_fonts)
  {
    if (std::filesystem::is_regular_file(path))
    {
      font_ui = io.Fonts->AddFontFromFileTTF(path, 18.0f);
      if (font_ui)
        break;
    }
  }
  for (const char* path : bold_fonts)
  {
    if (std::filesystem::is_regular_file(path))
    {
      font_bold = io.Fonts->AddFontFromFileTTF(path, 18.0f);
      if (font_bold)
        break;
    }
  }
  if (!font_ui)
    font_ui = io.Fonts->AddFontDefault();
  if (!font_bold)
    font_bold = font_ui;
  io.FontDefault = font_ui;
  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  LauncherArtwork artwork;
  if (!theme.background.empty())
  {
    fs::path background_path = release_directory / theme.background;
    if (!fs::is_regular_file(background_path))
      background_path = user_directory / theme.background;
    artwork.background = LoadPngTexture(renderer, background_path, &artwork.background_width,
                                        &artwork.background_height);
    if (!artwork.background)
      SDL_Log("Theme background not loaded: %s", background_path.string().c_str());
  }

  std::vector<fs::path> images = FindDiscImages();
  std::optional<fs::path> selected_image = images.empty() ? std::nullopt : std::optional(images[0]);
  fs::path current_game = ReadDefaultGame(user_directory, release_directory);
  bool current_dol_changed = false;
  std::string current_dol_error;
  auto current_metadata =
      ApplyBundledDolPatch(current_game, release_directory, &current_dol_changed,
                           &current_dol_error) ?
          moderngekko::InspectGame(current_game) :
          moderngekko::GameInspectResult{{}, current_dol_error};
  if (current_metadata && !RequiredGameError(*current_metadata.metadata).empty())
    current_metadata = {{}, RequiredGameError(*current_metadata.metadata)};
  const auto& resolutions = moderngekko::frontend::SupportedResolutions();
  const auto& graphics_backends =
      moderngekko::frontend::SupportedGraphicsBackends();
  bool show_fps_in_title = config.show_fps_in_title;
  bool fullscreen = config.fullscreen;
  static constexpr int kMsaaValues[] = {1, 2, 4, 8};
  static constexpr const char* kMsaaLabels[] = {"Off (1x)", "2x", "4x", "8x"};
  int msaa_index = 0;
  for (int i = 0; i < 4; ++i)
  {
    if (config.msaa == kMsaaValues[i])
      msaa_index = i;
  }
  bool ssaa = config.ssaa;
  int anisotropy_index = config.anisotropy;
  bool vsync = config.vsync;
  int aspect_index = config.aspect;
  bool widescreen_hack = config.widescreen_hack;
  bool efb_access = config.efb_access;
  std::string cpu_core = config.cpu_core;
  std::array<char, 31> netplay_nickname{};
  std::array<char, 256> netplay_address{};
  std::snprintf(netplay_nickname.data(), netplay_nickname.size(), "%s",
                config.netplay_nickname.c_str());
  std::snprintf(netplay_address.data(), netplay_address.size(), "%s",
                config.netplay_address.c_str());
  int netplay_port = config.netplay_port;
  bool automatic_buffer = config.netplay_buffer == "auto";
  int manual_buffer = automatic_buffer ? 5 : std::stoi(config.netplay_buffer);
  int resolution_index = 0;
  for (std::size_t i = 0; i < resolutions.size(); ++i)
  {
    if (config.resolution == resolutions[i].text)
      resolution_index = static_cast<int>(i);
  }
  int graphics_backend_index = 0;
  for (std::size_t i = 0; i < graphics_backends.size(); ++i)
  {
    if (config.graphics_backend == graphics_backends[i].value)
      graphics_backend_index = static_cast<int>(i);
  }

  // StateSaves is Dolphin's own state directory, so states written in-game land
  // here without any extra configuration and show up on the next launch.
  //
  // Listed once at startup rather than every frame, so the dropdown does not
  // hit the filesystem on each redraw. A state written while the launcher is
  // open therefore appears on the next launch, which is the case that matters:
  // states are written by the running game, not by the launcher.
  std::vector<fs::path> launcher_savestates =
      moderngekko::frontend::ListLauncherSavestates(user_directory / "StateSaves");
  // Loading is always explicit. StateSaves is shared by games, so selecting the
  // newest file automatically could try to resume a different title.
  int selected_savestate = -1;
  std::string card_status;
  DialogState gci_dialog;

  enum class LauncherPage
  {
    Play,
    Graphics,
    Performance,
    Audio,
    Controller,
  };
  LauncherPage page = LauncherPage::Play;
  bool audio_mute = config.mute;
  int audio_volume = config.volume;

  DialogState dialog;
  const auto save_config = [&]() -> bool
  {
    std::string error;
    if (!moderngekko::frontend::SaveConfig(user_directory, config, &error))
    {
      std::lock_guard lock(dialog.mutex);
      dialog.error = std::move(error);
      return false;
    }
    return true;
  };
  std::vector<ControllerOption> controllers = EnumerateControllers();
  bool controller_profile_exists = moderngekko::frontend::ControllerConfigExists(user_directory);
  std::vector<std::string> configured_controllers =
      moderngekko::frontend::ReadConfiguredControllers(user_directory);
  std::string selected_controller =
      configured_controllers.empty() ? config.controller : configured_controllers.front();
  int controller_index = FindController(controllers, selected_controller);
  std::string controller_status;
  int layout_index = 0;
  for (int i = 0; i < kControllerLayoutCount; ++i)
  {
    if (config.controller_class ==
        moderngekko::frontend::ControllerClassName(kControllerLayouts[i]))
      layout_index = i;
  }
  const auto select_controller = [&](int index)
  {
    using moderngekko::frontend::ControllerClass;
    const ControllerClass selected_class = kControllerLayouts[layout_index];
    std::string device = controllers[index].device;
    if (device == KEYBOARD_DEVICE)
      device = std::string(KEYBOARD_DEVICE);

    std::string message;
    // Rewriting is an explicit user action here; the runner never regenerates.
    const std::filesystem::path profile_path =
        user_directory / "Config" / std::string(CONTROLLER_PROFILE_NAME);
    std::error_code remove_error;
    std::filesystem::remove(profile_path, remove_error);
    if (!moderngekko::frontend::EnsureCanonicalPadProfile(user_directory, device,
                                                         &message))
    {
      std::lock_guard lock(dialog.mutex);
      dialog.error = std::move(message);
      return false;
    }
    selected_controller = device;
    controller_index = FindController(controllers, device);
    configured_controllers = {selected_controller};
    config.controller = selected_controller;
    config.controllers = configured_controllers;
    config.resolution = resolutions[resolution_index].text;
    config.show_fps_in_title = show_fps_in_title;
    config.controller_class =
        moderngekko::frontend::ControllerClassName(selected_class);
    if (!save_config())
      return false;
    controller_profile_exists = true;
    controller_status = std::move(message);
    return true;
  };
  const auto ensure_controller = [&]
  {
    if (moderngekko::frontend::ControllerConfigExists(user_directory))
    {
      controller_profile_exists = true;
      configured_controllers = moderngekko::frontend::ReadConfiguredControllers(user_directory);
      if (configured_controllers.empty())
      {
        std::lock_guard lock(dialog.mutex);
        dialog.error = std::string(CONTROLLER_PROFILE_NAME) +
                       " has no configured controller device";
        return false;
      }
      selected_controller = configured_controllers.front();
      controller_index = FindController(controllers, selected_controller);
      controller_status = "Using existing " + std::string(CONTROLLER_PROFILE_NAME);
      return true;
    }
    if (controller_index < 0)
    {
      std::lock_guard lock(dialog.mutex);
      dialog.error = "Select a controller or keyboard before playing";
      return false;
    }
    return select_controller(controller_index);
  };
  const auto refresh_controllers = [&]
  {
    controllers = EnumerateControllers();
    controller_index = FindController(controllers, selected_controller);
    if (!controller_profile_exists)
    {
      if (controller_index < 0 && !controllers.empty())
      {
        controller_index = 0;
        selected_controller = controllers.front().device;
      }
      if (controller_index >= 0)
        select_controller(controller_index);
      else
        controller_status = "No SDL gamepad detected";
    }
    else
    {
      controller_status = "Using existing " + std::string(CONTROLLER_PROFILE_NAME);
    }
  };
  refresh_controllers();

  const auto persist_common = [&]
  {
    config.resolution = resolutions[resolution_index].text;
    config.show_fps_in_title = show_fps_in_title;
    config.controller = selected_controller;
    if (!configured_controllers.empty())
      config.controllers = configured_controllers;
    else if (!selected_controller.empty())
      config.controllers = {selected_controller};
    config.controller_class =
        moderngekko::frontend::ControllerClassName(kControllerLayouts[layout_index]);
  };
  const auto save_netplay = [&]() -> bool
  {
    config.netplay_nickname = netplay_nickname.data();
    config.netplay_address = netplay_address.data();
    config.netplay_port = static_cast<std::uint16_t>(netplay_port);
    config.netplay_buffer = automatic_buffer ? "auto" : std::to_string(manual_buffer);
    persist_common();
    return save_config();
  };
  ExtractionState extraction;
  std::jthread extraction_thread;
  enum class LaunchMode
  {
    None,
    Solo,
    Host,
    Join,
  };
  bool done = false;
  LaunchMode launch_mode = LaunchMode::None;
  if (auto_play && current_metadata &&
      RequiredGameError(*current_metadata.metadata).empty() && ensure_controller())
  {
    launch_mode = LaunchMode::Solo;
    done = true;
  }
  while (!done)
  {
    bool controllers_changed = false;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        done = true;
      if (event.type == SDL_EVENT_JOYSTICK_ADDED || event.type == SDL_EVENT_JOYSTICK_REMOVED ||
          event.type == SDL_EVENT_GAMEPAD_REMAPPED)
      {
        controllers_changed = true;
      }
    }
    if (controllers_changed)
      refresh_controllers();

    {
      std::lock_guard lock(dialog.mutex);
      if (dialog.selected)
      {
        selected_image = std::move(dialog.selected);
        dialog.selected.reset();
      }
    }
    {
      std::lock_guard lock(gci_dialog.mutex);
      if (gci_dialog.selected)
      {
        const fs::path source = *gci_dialog.selected;
        const fs::path destination = FolderCardPath(user_directory) / source.filename();
        std::error_code copy_error;
        fs::create_directories(destination.parent_path(), copy_error);
        fs::copy_file(source, destination, fs::copy_options::overwrite_existing, copy_error);
        card_status = copy_error ? "Import failed: " + copy_error.message()
                                 : "Imported " + source.filename().string();
        gci_dialog.selected.reset();
      }
    }
    {
      std::optional<fs::path> selected_folder;
      {
        std::lock_guard lock(dialog.mutex);
        selected_folder = std::move(dialog.selected_folder);
        dialog.selected_folder.reset();
      }
      if (selected_folder)
      {
        current_game = *selected_folder;
        current_metadata = moderngekko::InspectGame(current_game);
        if (current_metadata && !RequiredGameError(*current_metadata.metadata).empty())
          current_metadata = {{}, RequiredGameError(*current_metadata.metadata)};
        std::string error;
        if (!WriteDefaultGame(user_directory, release_directory, current_game, &error))
        {
          std::lock_guard lock(dialog.mutex);
          dialog.error = std::move(error);
        }
      }
    }
    {
      std::lock_guard lock(extraction.mutex);
      if (extraction.finished_game)
      {
        current_game = *extraction.finished_game;
        current_metadata = moderngekko::InspectGame(current_game);
        if (current_metadata && !RequiredGameError(*current_metadata.metadata).empty())
          current_metadata = {{}, RequiredGameError(*current_metadata.metadata)};
        extraction.finished_game.reset();
        if (ensure_controller())
        {
          launch_mode = LaunchMode::Solo;
          done = true;
        }
      }
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float S = scale;
    const double now = ImGui::GetTime();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##launcher_root", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoNavFocus);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = viewport->Pos;
    const ImVec2 size = viewport->Size;

    // ------------------------------------------------------------------
    // Theme backdrop: the SteamGridDB hero when theme.ini provides one, with
    // a procedural arena as the fallback. No game data is involved.
    // ------------------------------------------------------------------
    if (artwork.background)
    {
      const float texture_w = static_cast<float>(artwork.background_width);
      const float texture_h = static_cast<float>(artwork.background_height);
      const float cover = std::max(size.x / texture_w, size.y / texture_h);
      const float draw_w = texture_w * cover;
      const float draw_h = texture_h * cover;
      const float x0 = origin.x + (size.x - draw_w) * 0.5f;
      const float y0 = origin.y + (size.y - draw_h) * 0.5f;
      draw->AddImage((ImTextureID)(intptr_t)artwork.background, ImVec2(x0, y0),
                     ImVec2(x0 + draw_w, y0 + draw_h));
      draw->AddRectFilledMultiColor(
          origin, ImVec2(origin.x + size.x, origin.y + size.y),
          IM_COL32(4, 5, 10, 144), IM_COL32(4, 5, 10, 44),
          IM_COL32(4, 5, 10, 104), IM_COL32(4, 5, 10, 184));
    }
    else
    {
      draw->AddRectFilledMultiColor(
          origin, ImVec2(origin.x + size.x, origin.y + size.y),
          IM_COL32(5, 6, 11, 255), IM_COL32(10, 10, 18, 255),
          IM_COL32(26, 16, 18, 255), IM_COL32(5, 6, 11, 255));
  
      // Gold spotlight beams from the arena rig.
      for (int beam = 0; beam < 5; ++beam)
      {
        const float x = origin.x + size.x * (0.14f + 0.18f * beam);
        const float sway = std::sin(static_cast<float>(now) * 0.22f + beam * 0.9f) * 12 * S;
        draw->AddTriangleFilled(
            ImVec2(x + sway, origin.y - 30 * S),
            ImVec2(x - 150 * S, origin.y + size.y * 0.88f),
            ImVec2(x + 190 * S, origin.y + size.y * 0.88f),
            IM_COL32(255, 214, 140, 9));
        draw->AddTriangleFilled(
            ImVec2(x + sway, origin.y - 30 * S),
            ImVec2(x - 60 * S, origin.y + size.y * 0.82f),
            ImVec2(x + 86 * S, origin.y + size.y * 0.82f),
            IM_COL32(255, 236, 195, 11));
      }
  
      // Crowd bokeh, slowly twinkling.
      for (int i = 0; i < 90; ++i)
      {
        const float fx = std::fmod(i * 0.6180339f, 1.0f);
        const float fy = std::fmod(i * 0.381966f + 0.06f, 0.40f);
        const float twinkle = 0.5f + 0.5f * std::sin(static_cast<float>(now) * 0.8f + i * 1.7f);
        const int alpha = static_cast<int>(10 + 26 * twinkle);
        const ImU32 color = (i % 3 == 0) ? IM_COL32(245, 205, 130, alpha)
                            : (i % 3 == 1) ? IM_COL32(190, 200, 220, alpha)
                                           : IM_COL32(150, 60, 70, alpha);
        draw->AddCircleFilled(
            ImVec2(origin.x + size.x * fx, origin.y + size.y * fy),
            (1.2f + (i % 5) * 0.8f) * S, color);
      }
  
      // The ring: canvas, apron, three red ropes and corner posts.
      const float ring_cx = origin.x + size.x * 0.52f;
      const float ring_cy = origin.y + size.y * 0.73f;
      const float ring_rx = size.x * 0.33f;
      const float ring_ry = size.y * 0.11f;
      draw->AddEllipseFilled(ImVec2(ring_cx, ring_cy + ring_ry * 0.45f),
                             ImVec2(ring_rx, ring_ry * 0.85f),
                             IM_COL32(12, 13, 18, 150), 0, 64);
      draw->AddEllipse(ImVec2(ring_cx, ring_cy), ImVec2(ring_rx, ring_ry),
                       IM_COL32(78, 80, 92, 110), 0, 64, 4 * S);
      draw->AddEllipse(ImVec2(ring_cx, ring_cy), ImVec2(ring_rx, ring_ry),
                       IM_COL32(150, 28, 40, 120), 0, 64, 2 * S);
      for (int rope = 0; rope < 3; ++rope)
      {
        ImVec2 points[33];
        for (int seg = 0; seg <= 32; ++seg)
        {
          const float t = seg / 32.0f;
          const float px = ring_cx - ring_rx * 1.12f + ring_rx * 2.24f * t;
          const float py = ring_cy - ring_ry * (0.85f - rope * 0.42f) +
                           std::sin(t * 3.14159265f) * ring_ry * 0.32f;
          points[seg] = ImVec2(px, py);
        }
        draw->AddPolyline(points, 33, IM_COL32(185, 28, 42, 165), 0, 3.0f * S);
      }
      for (const float side : {-1.0f, 1.0f})
      {
        const float px = ring_cx + side * ring_rx * 1.12f;
        draw->AddRectFilled(ImVec2(px - 6 * S, ring_cy - ring_ry * 1.5f),
                            ImVec2(px + 6 * S, ring_cy + ring_ry * 0.6f),
                            IM_COL32(110, 114, 128, 90), 4 * S);
      }
    }

    // ------------------------------------------------------------------
    // Header
    // ------------------------------------------------------------------
    const float header_h = 108 * S;
    const float pad = 30 * S;
    draw->AddLine(ImVec2(origin.x, origin.y + header_h),
                  ImVec2(origin.x + size.x, origin.y + header_h),
                  ImGui::ColorConvertFloat4ToU32(ImVec4(theme.accent.x, theme.accent.y,
                                                       theme.accent.z, 0.24f)),
                  1.5f * S);
    ImGui::SetCursorPos(ImVec2(pad, 20 * S));
    ImGui::PushFont(font_bold, 15 * S);
    ImGui::TextColored(theme.accent, "%s", theme.subtitle.c_str());
    ImGui::PopFont();
    ImGui::SetCursorPos(ImVec2(pad, 38 * S));
    ImGui::PushFont(font_bold, 32 * S);
    ImGui::TextUnformatted(theme.product.c_str());
    ImGui::PopFont();

    const bool game_ready = current_metadata &&
                            RequiredGameError(*current_metadata.metadata).empty();
    const std::string ready_chip = theme.disc_id + "  |  READY";
    const char* chip_text = game_ready ? ready_chip.c_str() : "NO GAME IMPORTED";
    const ImVec2 chip_text_size =
        ImGui::CalcTextSize(chip_text);
    const float chip_w = chip_text_size.x + 28 * S;
    const ImVec2 chip_min(origin.x + size.x - pad - chip_w, origin.y + 30 * S);
    const ImVec2 chip_max(chip_min.x + chip_w, chip_min.y + 32 * S);
    draw->AddRectFilled(chip_min, chip_max,
                        game_ready ? IM_COL32(34, 120, 92, 190) : IM_COL32(120, 44, 52, 190),
                        16 * S);
    draw->AddRect(chip_min, chip_max,
                  game_ready ? IM_COL32(120, 230, 180, 120) : IM_COL32(240, 120, 120, 120),
                  16 * S, 0, 1.0f * S);
    draw->AddText(ImVec2(chip_min.x + 14 * S, chip_min.y + 8 * S),
                  IM_COL32(235, 250, 245, 255), chip_text);
    ImGui::SetCursorPos(ImVec2(origin.x + size.x - pad - 260 * S, origin.y + 70 * S));
    ImGui::TextDisabled("Static recompilation  |  PC");

    // ------------------------------------------------------------------
    // Sidebar
    // ------------------------------------------------------------------
    const float sidebar_w = 232 * S;
    const float body_y = header_h + 20 * S;
    const float body_h = size.y - body_y - 20 * S;
    ImGui::SetCursorPos(ImVec2(0, body_y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24 * S, 0));
    ImGui::BeginChild("##sidebar", ImVec2(sidebar_w, body_h), false,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8 * S));
    const auto nav_button = [&](const char* label, LauncherPage target)
    {
      const bool active = page == target;
      const ImVec2 pos = ImGui::GetCursorScreenPos();
      const float line_h = 42 * S;
      if (active)
      {
        draw->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 4 * S, pos.y + line_h),
                            ImGui::ColorConvertFloat4ToU32(theme.accent), 2 * S);
      }
      ImGui::PushStyleColor(ImGuiCol_Button,
                            active ? ImVec4(0.34f, 0.11f, 0.13f, 0.95f)
                                   : ImVec4(0.06f, 0.07f, 0.10f, 0.60f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.10f, 0.12f, 0.95f));
      ImGui::PushStyleColor(ImGuiCol_Text,
                            active ? theme.accent
                                   : ImVec4(0.86f, 0.87f, 0.90f, 1.0f));
      ImGui::PushFont(font_bold, 16 * S);
      if (ImGui::Button(label, ImVec2(-1, line_h)))
        page = target;
      ImGui::PopFont();
      ImGui::PopStyleColor(3);
    };
    nav_button("PLAY", LauncherPage::Play);
    nav_button("GRAPHICS", LauncherPage::Graphics);
    nav_button("PERFORMANCE", LauncherPage::Performance);
    nav_button("AUDIO", LauncherPage::Audio);
    nav_button("CONTROLLER", LauncherPage::Controller);
    ImGui::PopStyleVar();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 18 * S);
    ImGui::Separator();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 14 * S);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.62f, 0.66f, 1.0f));
    ImGui::TextWrapped("Your disc, your files.\nNo game data ships with this app.");
    ImGui::PopStyleColor();
    ImGui::SetCursorPosY(body_h - 58 * S);
    ImGui::TextDisabled("v%s", theme.version.c_str());
    ImGui::SetCursorPosY(body_h - 38 * S);
    ImGui::TextDisabled("%s", game_ready ? "Game files verified" : "Import a disc to begin");
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // ------------------------------------------------------------------
    // Content
    // ------------------------------------------------------------------
    const float content_x = sidebar_w + 18 * S;
    const float content_w = size.x - content_x - pad;
    ImGui::SetCursorPos(ImVec2(content_x, body_y));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("##content", ImVec2(content_w, body_h), false,
                      ImGuiWindowFlags_NoBackground);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20 * S, 18 * S));
    const auto begin_card = [&](const char* id, const char* title, float height)
    {
      ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.035f, 0.040f, 0.055f, 0.84f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.95f, 0.82f, 0.20f, 0.16f));
      ImGui::BeginChild(id, ImVec2(0, height * S), ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoScrollbar);
      ImGui::PushFont(font_bold, 15 * S);
      ImGui::TextColored(theme.accent, "%s", title);
      ImGui::PopFont();
      ImGui::Separator();
      ImGui::Spacing();
    };
    const auto end_card = [&]
    {
      ImGui::EndChild();
      ImGui::PopStyleColor(2);
    };
    const auto row_label = [&](const char* text)
    {
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(text);
    };
    const auto save_or_report = [&]() -> bool
    {
      persist_common();
      return save_config();
    };

    if (page == LauncherPage::Play)
    {
      if (ImGui::BeginTable("##play_row", 2, ImGuiTableFlags_SizingStretchSame))
      {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        begin_card("##game_card", "GAME", 500);
        if (current_metadata)
        {
          ImGui::PushFont(font_bold, 20 * S);
          ImGui::TextWrapped("%s", current_metadata.metadata->game_name.c_str());
          ImGui::PopFont();
          ImGui::TextDisabled("Disc ID %s  |  retail DOL verified",
                              current_metadata.metadata->disc_id.c_str());
        }
        else
        {
          ImGui::PushFont(font_bold, 20 * S);
          ImGui::TextUnformatted("No extracted game yet");
          ImGui::PopFont();
          ImGui::TextDisabled("Import your own GameCube disc below.");
        }
        ImGui::Spacing();
        ImGui::TextDisabled("Extracted game folder");
        if (!current_game.empty())
          ImGui::TextWrapped("%s", current_game.string().c_str());
        else
          ImGui::TextDisabled("(none)");
        if (ImGui::Button("Browse for extracted game folder", ImVec2(-1, 36 * S)))
          SDL_ShowOpenFolderDialog(FolderDialogCallback, &dialog, window, nullptr, false);
        ImGui::Spacing();
        ImGui::TextDisabled("Disc image");
        if (selected_image)
          ImGui::TextWrapped("%s", selected_image->filename().string().c_str());
        else
          ImGui::TextDisabled("No ISO, WBFS or RVZ selected");
        if (!extraction.running)
        {
          if (ImGui::Button("Browse for ISO / WBFS / RVZ", ImVec2(-1, 36 * S)))
          {
            static constexpr SDL_DialogFileFilter filters[] = {
                {"Disc images", "iso;wbfs;rvz"},
                {"ISO", "iso"},
                {"WBFS", "wbfs"},
                {"RVZ", "rvz"}};
            const std::string documents = DocumentsDirectory().string();
            SDL_ShowOpenFileDialog(FileDialogCallback, &dialog, window, filters,
                                   static_cast<int>(std::size(filters)), documents.c_str(),
                                   false);
          }
          if (selected_image)
          {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.98f, 0.82f, 0.12f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.90f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.06f, 0.08f, 0.10f, 1.0f));
            if (ImGui::Button("Extract and play", ImVec2(-1, 36 * S)))
            {
              extraction.completed = 0;
              extraction.total = 1;
              extraction.running = true;
              {
                std::lock_guard lock(extraction.mutex);
                extraction.error.clear();
                extraction.finished_game.reset();
              }
              const fs::path image = *selected_image;
              extraction_thread =
                  std::jthread([image, user_directory, release_directory, &extraction]
                               {
                                 ExtractDisc(image, user_directory, release_directory,
                                             &extraction);
                               });
            }
            ImGui::PopStyleColor(3);
          }
        }
        else
        {
          const float progress = std::min(
              1.0f, static_cast<float>(extraction.completed.load()) / extraction.total.load());
          ImGui::ProgressBar(progress, ImVec2(-1, 20 * S));
        }
        end_card();

        ImGui::TableNextColumn();
        begin_card("##launch_card", "LAUNCH", 500);
        const ImVec4 accent = theme.accent;
        const ImVec4 accent_hover(std::min(1.0f, accent.x + 0.08f),
                                  std::min(1.0f, accent.y + 0.08f),
                                  std::min(1.0f, accent.z + 0.08f), 1.0f);
        const ImVec4 accent_active(std::max(0.0f, accent.x - 0.10f),
                                   std::max(0.0f, accent.y - 0.10f),
                                   std::max(0.0f, accent.z - 0.10f), 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, accent);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, accent_hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, accent_active);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.06f, 0.08f, 0.10f, 1.0f));
        ImGui::PushFont(font_bold, 28 * S);
        if (ImGui::Button("PLAY", ImVec2(-1, 70 * S)))
        {
          if (ensure_controller())
          {
            launch_mode = LaunchMode::Solo;
            done = true;
          }
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(4);
        ImGui::Spacing();
        if (ImGui::BeginTable("##netplay_row", 2, ImGuiTableFlags_SizingStretchSame))
        {
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          if (ImGui::Button("Host netplay", ImVec2(-1, 32 * S)))
          {
            if (ensure_controller() && save_netplay())
            {
              launch_mode = LaunchMode::Host;
              done = true;
            }
          }
          ImGui::TableNextColumn();
          if (ImGui::Button("Join netplay", ImVec2(-1, 32 * S)))
          {
            if (ensure_controller() && save_netplay())
            {
              launch_mode = LaunchMode::Join;
              done = true;
            }
          }
          ImGui::EndTable();
        }
        ImGui::Spacing();
        if (!launcher_savestates.empty())
        {
          ImGui::TextDisabled("Start from savestate");
          const std::string savestate_preview =
              selected_savestate >= 0
                  ? moderngekko::frontend::LauncherSavestateLabel(
                        launcher_savestates[static_cast<std::size_t>(selected_savestate)],
                        selected_savestate == 0)
                  : "Normal boot";
          ImGui::SetNextItemWidth(-1);
          if (ImGui::BeginCombo("##launch_savestate", savestate_preview.c_str()))
          {
            const bool normal_boot_selected = selected_savestate < 0;
            if (ImGui::Selectable("Normal boot", normal_boot_selected))
              selected_savestate = -1;
            if (normal_boot_selected)
              ImGui::SetItemDefaultFocus();
            for (std::size_t i = 0; i < launcher_savestates.size(); ++i)
            {
              const std::string label = moderngekko::frontend::LauncherSavestateLabel(
                  launcher_savestates[i], i == 0);
              const bool selected = selected_savestate == static_cast<int>(i);
              if (ImGui::Selectable(label.c_str(), selected))
                selected_savestate = static_cast<int>(i);
              if (selected)
                ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }
          ImGui::TextDisabled("Solo play only.");
        }
        else
        {
          ImGui::TextDisabled("Savestates appear here after the game writes one.");
        }
        ImGui::Spacing();
        if (selected_savestate >= 0 &&
            static_cast<std::size_t>(selected_savestate) < launcher_savestates.size())
        {
          if (ImGui::Button("Delete selected state", ImVec2(-1, 28 * S)))
          {
            std::error_code state_error;
            fs::remove(launcher_savestates[static_cast<std::size_t>(selected_savestate)],
                       state_error);
            launcher_savestates =
                moderngekko::frontend::ListLauncherSavestates(user_directory / "StateSaves");
            selected_savestate = -1;
          }
        }
        if (ImGui::Button("Open state folder", ImVec2(-1, 28 * S)))
          OpenFolder(user_directory / "StateSaves");
        ImGui::TextDisabled("Press F1 in game to save a state.");
        end_card();
        ImGui::EndTable();
      }
      ImGui::Spacing();
      begin_card("##status_card", "STATUS", 150);
      ImGui::TextWrapped("Controller: %s", controller_status.c_str());
      {
        std::lock_guard lock(extraction.mutex);
        if (!extraction.status.empty())
          ImGui::TextWrapped("%s", extraction.status.c_str());
        if (!extraction.error.empty())
          ImGui::TextColored(ImVec4(1.0f, 0.40f, 0.34f, 1.0f), "%s",
                             extraction.error.c_str());
      }
      {
        std::lock_guard lock(dialog.mutex);
        if (!dialog.error.empty())
          ImGui::TextColored(ImVec4(1.0f, 0.40f, 0.34f, 1.0f), "%s", dialog.error.c_str());
      }
      ImGui::TextDisabled("Settings save automatically. The extracted game folder is reused "
                          "between sessions.");
      end_card();

      ImGui::Spacing();
      begin_card("##card_card", "MEMORY CARD", 300);
      const fs::path card_path = RawCardPath(user_directory);
      std::error_code card_size_error;
      const auto card_size = fs::file_size(card_path, card_size_error);
      ImGui::TextDisabled("Slot A raw card");
      ImGui::TextWrapped("%s", card_path.string().c_str());
      ImGui::TextDisabled("%s",
                          card_size_error ? "not created yet (the game formats it on first boot)"
                                          : (std::to_string(card_size / (1024 * 1024)) +
                                             " MB, created by the game on first boot")
                                                .c_str());
      if (ImGui::Button("Back up card", ImVec2(-1, 30 * S)))
      {
        const fs::path backup = BackupFolderCard(user_directory);
        card_status = backup.empty() ? "Backup failed (no card yet)"
                                     : "Backup: " + backup.string();
      }
      ImGui::BeginDisabled(true);
      if (ImGui::Button("Import .gci", ImVec2(-1, 30 * S)))
      {
      }
      ImGui::EndDisabled();
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Slot A is a raw card now; importing a single .gci needs a "
                          "folder card. Copy MemoryCardA.USA.raw instead.");
      if (ImGui::Button("Open card folder", ImVec2(-1, 30 * S)))
        OpenFolder(card_path.parent_path());
      ImGui::TextDisabled("%s", card_status.c_str());
      end_card();

      ImGui::Spacing();
      begin_card("##about_card", "ABOUT", 150);
      ImGui::TextDisabled("%s v%s", theme.product.c_str(), theme.version.c_str());
      ImGui::TextWrapped("Static recompilation build. Updates are handled by "
                         "RecompLauncher.");
      end_card();
    }
    else if (page == LauncherPage::Graphics)
    {
      if (ImGui::BeginTable("##gfx_row1", 2, ImGuiTableFlags_SizingStretchSame))
      {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        begin_card("##display_card", "DISPLAY", 356);
        ImGui::TextDisabled("Graphics backend");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##graphics_backend",
                              graphics_backends[graphics_backend_index].text))
        {
          for (std::size_t i = 0; i < graphics_backends.size(); ++i)
          {
            const bool selected = graphics_backend_index == static_cast<int>(i);
            if (ImGui::Selectable(graphics_backends[i].text, selected))
            {
              const std::string previous = config.graphics_backend;
              config.graphics_backend = graphics_backends[i].value;
              if (save_or_report())
                graphics_backend_index = static_cast<int>(i);
              else
                config.graphics_backend = previous;
            }
            if (selected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        ImGui::Spacing();
        ImGui::TextDisabled("Internal resolution (EFB upscale)");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##resolution", resolutions[resolution_index].text))
        {
          for (std::size_t i = 0; i < resolutions.size(); ++i)
          {
            const bool selected = resolution_index == static_cast<int>(i);
            if (ImGui::Selectable(resolutions[i].text, selected))
            {
              const std::string previous = config.resolution;
              config.resolution = resolutions[i].text;
              if (save_or_report())
                resolution_index = static_cast<int>(i);
              else
                config.resolution = previous;
            }
          }
          ImGui::EndCombo();
        }
        ImGui::Spacing();
        if (ImGui::Checkbox("Start in fullscreen", &fullscreen))
        {
          const bool previous = config.fullscreen;
          config.fullscreen = fullscreen;
          if (!save_or_report())
          {
            fullscreen = previous;
            config.fullscreen = previous;
          }
        }
        if (ImGui::Checkbox("VSync", &vsync))
        {
          const bool previous = config.vsync;
          config.vsync = vsync;
          if (!save_or_report())
          {
            vsync = previous;
            config.vsync = previous;
          }
        }
        end_card();

        ImGui::TableNextColumn();
        begin_card("##quality_card", "IMAGE QUALITY", 356);
        ImGui::TextDisabled("Anti-aliasing (MSAA)");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##msaa", kMsaaLabels[msaa_index]))
        {
          for (int i = 0; i < 4; ++i)
          {
            const bool selected = msaa_index == i;
            if (ImGui::Selectable(kMsaaLabels[i], selected))
            {
              const int previous = config.msaa;
              config.msaa = kMsaaValues[i];
              if (save_or_report())
                msaa_index = i;
              else
                config.msaa = previous;
            }
            if (selected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        ImGui::Spacing();
        if (ImGui::Checkbox("SSAA (supersampling)", &ssaa))
        {
          const bool previous = config.ssaa;
          config.ssaa = ssaa;
          if (!save_or_report())
          {
            ssaa = previous;
            config.ssaa = previous;
          }
        }
        ImGui::Spacing();
        ImGui::TextDisabled("Anisotropic filtering");
        static constexpr const char* kAnisoLabels[] = {"1x", "2x", "4x", "8x", "16x"};
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##anisotropy", kAnisoLabels[anisotropy_index]))
        {
          for (int i = 0; i < 5; ++i)
          {
            const bool selected = anisotropy_index == i;
            if (ImGui::Selectable(kAnisoLabels[i], selected))
            {
              const int previous = config.anisotropy;
              config.anisotropy = i;
              if (save_or_report())
                anisotropy_index = i;
              else
                config.anisotropy = previous;
            }
            if (selected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        end_card();
        ImGui::EndTable();
      }
      ImGui::Spacing();
      if (ImGui::BeginTable("##gfx_row2", 2, ImGuiTableFlags_SizingStretchSame))
      {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        begin_card("##aspect_card", "FRAMING", 214);
        ImGui::TextDisabled("Aspect ratio");
        static constexpr const char* kAspectLabels[] = {"Auto (game native)", "16:9 widescreen",
                                                        "4:3 standard", "Stretch"};
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##aspect", kAspectLabels[aspect_index]))
        {
          for (int i = 0; i < 4; ++i)
          {
            const bool selected = aspect_index == i;
            if (ImGui::Selectable(kAspectLabels[i], selected))
            {
              const int previous = config.aspect;
              config.aspect = i;
              if (save_or_report())
                aspect_index = i;
              else
                config.aspect = previous;
            }
            if (selected)
              ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        if (ImGui::Checkbox("Widescreen hack (stretch 3D)", &widescreen_hack))
        {
          const bool previous = config.widescreen_hack;
          config.widescreen_hack = widescreen_hack;
          if (!save_or_report())
          {
            widescreen_hack = previous;
            config.widescreen_hack = previous;
          }
        }
        end_card();
        ImGui::TableNextColumn();
        begin_card("##compat_card", "COMPATIBILITY", 214);
        if (ImGui::Checkbox("2D compatibility (EFB access)", &efb_access))
        {
          const bool previous = config.efb_access;
          config.efb_access = efb_access;
          if (!save_or_report())
          {
            efb_access = previous;
            config.efb_access = previous;
          }
        }
        ImGui::TextWrapped("Required for the legal, title and menu screens. Accurate EFB "
                           "reads cost nothing in 3D.");
        end_card();
        ImGui::EndTable();
      }
    }
    else if (page == LauncherPage::Performance)
    {
      begin_card("##fps_card", "FRAME RATE", 132);
      ImGui::TextDisabled("59.94 FPS");
      ImGui::TextWrapped("The game runs at the console's native 59.94 Hz. Frame-rate uncapping "
                         "is disabled so timing and physics stay at the original speed.");
      end_card();
      ImGui::Spacing();
      begin_card("##core_card", "CPU CORE", 190);
      ImGui::TextDisabled("Game core");
      const char* core_preview =
          cpu_core == "static" ? "Static recomp (recommended)" : "JIT (fallback)";
      ImGui::SetNextItemWidth(-1);
      if (ImGui::BeginCombo("##core", core_preview))
      {
        if (ImGui::Selectable("Static recomp (recommended)", cpu_core == "static"))
        {
          const std::string previous = config.cpu_core;
          config.cpu_core = "static";
          cpu_core = "static";
          if (!save_or_report())
          {
            config.cpu_core = previous;
            cpu_core = previous;
          }
        }
        if (ImGui::Selectable("JIT (fallback)", cpu_core != "static"))
        {
          const std::string previous = config.cpu_core;
          config.cpu_core = "jit";
          cpu_core = "jit";
          if (!save_or_report())
          {
            config.cpu_core = previous;
            cpu_core = previous;
          }
        }
        ImGui::EndCombo();
      }
      ImGui::TextWrapped("Static runs the shipped Clang module and holds 59.94 fps in "
                         "the tested match. JIT is a fallback that ignores the module.");
      end_card();
      ImGui::Spacing();
      begin_card("##diag_card", "DIAGNOSTICS", 280);
      if (ImGui::Checkbox("Show FPS in the window title", &show_fps_in_title))
      {
        const bool previous = config.show_fps_in_title;
        config.show_fps_in_title = show_fps_in_title;
        if (!save_or_report())
        {
          show_fps_in_title = previous;
          config.show_fps_in_title = previous;
        }
      }
      if (ImGui::Button("Open logs", ImVec2(-1, 28 * S)))
        OpenFolder(user_directory / "Logs");
      if (ImGui::Button("Copy log tail", ImVec2(-1, 28 * S)))
      {
        const std::string tail = ReadLogTail(launcher_log_path, 40);
        SDL_SetClipboardText(tail.c_str());
      }
      ImGui::TextDisabled("Runner: %s", MODERNGEKKO_RUNNER_FILENAME);
      const char* module_id =
          (current_metadata && current_metadata.metadata)
              ? current_metadata.metadata->disc_id.c_str()
              : theme.disc_id.c_str();
      ImGui::TextDisabled("Module: g%s_recomp.dll", module_id);
      if (!log_video_line.empty())
        ImGui::TextWrapped("%s", log_video_line.c_str());
      else
        ImGui::TextDisabled("No [video] line yet.");
      end_card();
      ImGui::Spacing();
      begin_card("##host_card", "HOST", 220);
      ImGui::TextDisabled("Shader cache: %s",
                          host.shader_cache ? "Cache\\Shaders ready" : "missing");
      ImGui::TextDisabled("Priority: %s", host.high_priority ? "High" : "normal");
      ImGui::TextDisabled("Affinity: %s", host.affinity_text.c_str());
      ImGui::TextDisabled("DSP ROM: %s", host.dsp_rom ? "OK" : "missing");
      ImGui::TextDisabled("DSP coef: %s", host.dsp_coef ? "OK" : "missing");
      ImGui::TextDisabled("Audio: %s", audio_mute ? "muted" : "on");
      end_card();
    }
    else if (page == LauncherPage::Audio)
    {
      begin_card("##audio_card", "AUDIO", 272);
      if (ImGui::Checkbox("Mute all audio", &audio_mute))
      {
        const bool previous = config.mute;
        config.mute = audio_mute;
        if (!save_or_report())
        {
          audio_mute = previous;
          config.mute = previous;
        }
      }
      ImGui::TextWrapped("Silences the game, movies and menu sounds. Takes effect the next "
                         "time you press Play.");
      ImGui::Spacing();
      ImGui::BeginDisabled(audio_mute);
      ImGui::TextDisabled("Master volume");
      ImGui::SetNextItemWidth(-1);
      if (ImGui::SliderInt("##volume", &audio_volume, 0, 100, "%d%%"))
      {
        const int previous = config.volume;
        config.volume = audio_volume;
        if (!save_or_report())
        {
          audio_volume = previous;
          config.volume = previous;
        }
      }
      ImGui::EndDisabled();
      ImGui::TextDisabled("100% is the game's original mix level.");
      end_card();
    }
    else if (page == LauncherPage::Controller)
    {
      begin_card("##pad_card", "CONTROLLER", 430);
      const char* controller_preview =
          controller_index >= 0
              ? controllers[controller_index].label.c_str()
              : selected_controller.empty() ? "No controller detected"
                                            : selected_controller.c_str();
      ImGui::TextDisabled("Active controller");
      ImGui::SetNextItemWidth(-1);
      if (ImGui::BeginCombo("##controller", controller_preview))
      {
        for (std::size_t i = 0; i < controllers.size(); ++i)
        {
          const bool selected = controller_index == static_cast<int>(i);
          if (ImGui::Selectable(controllers[i].label.c_str(), selected))
          {
            controller_index = static_cast<int>(i);
            selected_controller = controllers[i].device;
            controller_status = controller_profile_exists
                                    ? "Existing " + std::string(CONTROLLER_PROFILE_NAME) +
                                          " unchanged"
                                    : "Ready to write the canonical profile";
          }
          if (selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      ImGui::Spacing();
      ImGui::TextDisabled("Layout");
      ImGui::SetNextItemWidth(-1);
      if (ImGui::BeginCombo("##layout", kControllerLayoutLabels[layout_index]))
      {
        for (int i = 0; i < kControllerLayoutCount; ++i)
        {
          const bool selected = layout_index == i;
          if (ImGui::Selectable(kControllerLayoutLabels[i], selected))
          {
            const int previous = layout_index;
            layout_index = i;
            config.controller_class =
                moderngekko::frontend::ControllerClassName(kControllerLayouts[i]);
            if (!save_config())
            {
              layout_index = previous;
              config.controller_class =
                  moderngekko::frontend::ControllerClassName(kControllerLayouts[previous]);
            }
            else
            {
              controller_status =
                  controller_profile_exists
                      ? "Layout set. Press Use this controller to rewrite " +
                            std::string(CONTROLLER_PROFILE_NAME)
                      : "Layout set. It will be used for the generated profile.";
            }
          }
          if (selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      const moderngekko::frontend::ControllerClass preview_class =
          kControllerLayouts[layout_index];
      if (preview_class == moderngekko::frontend::ControllerClass::Auto)
        ImGui::TextDisabled("Class: Auto - each pad decides from its SDL type");
      else
        ImGui::TextDisabled("Class: %s (forced)",
                            kControllerLayoutShortLabels[layout_index]);

      ImGui::BeginDisabled(controller_index < 0);
      if (ImGui::Button("Use this controller", ImVec2(-1, 34 * S)))
        select_controller(controller_index);
      ImGui::EndDisabled();
      ImGui::TextDisabled("%s", controller_status.c_str());
      ImGui::Separator();
      ImGui::TextDisabled("Mapping");
      ImGui::TextWrapped("One static profile holds the positional bindings. A/B and "
                         "X/Y are swapped in the input layer for Nintendo-class pads, "
                         "so nothing is regenerated at launch. D-pad, sticks and "
                         "triggers are never swapped.");
      if (selected_controller == KEYBOARD_DEVICE)
        ImGui::TextWrapped("Keyboard: Enter = A, Backspace = B, X/Y = X/Y, Left Shift "
                           "= Z, Space = Start, WASD = main stick, IJKL = C-stick, "
                           "Q/E = L/R, arrows = D-pad.");
      ImGui::TextDisabled("Profile: %s",
                          (user_directory / "Config" /
                           std::string(CONTROLLER_PROFILE_NAME)).string().c_str());
      end_card();
      ImGui::Spacing();
      begin_card("##netplay_card", "NETPLAY", 372);
      ImGui::TextDisabled("Nickname");
      ImGui::SetNextItemWidth(-1);
      ImGui::InputText("##nickname", netplay_nickname.data(), netplay_nickname.size());
      ImGui::TextDisabled("Host / IP");
      ImGui::SetNextItemWidth(-1);
      ImGui::InputText("##host", netplay_address.data(), netplay_address.size());
      ImGui::TextDisabled("UDP port");
      ImGui::SetNextItemWidth(160 * S);
      ImGui::InputInt("##port", &netplay_port);
      netplay_port = std::clamp(netplay_port, 1, 65535);
      ImGui::Checkbox("Automatic input buffer", &automatic_buffer);
      if (!automatic_buffer)
      {
        ImGui::SetNextItemWidth(180 * S);
        ImGui::SliderInt("##buffer", &manual_buffer, 1, 20);
      }
      ImGui::TextDisabled("Both players need the same game revision and release.");
      end_card();
    }

    {
      std::lock_guard lock(dialog.mutex);
      if (!dialog.error.empty())
      {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.40f, 0.34f, 1.0f), "%s", dialog.error.c_str());
      }
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::Render();
    SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(renderer, 18, 20, 28, 255);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  }

  if (extraction_thread.joinable())
    extraction_thread.join();

  int result = 0;
  if (launch_mode != LaunchMode::None)
  {
    std::string launch_error;
    if (!WriteDefaultGame(user_directory, release_directory, current_game,
                          &launch_error))
    {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Launch failed", launch_error.c_str(), window);
      result = 1;
    }
    else
    {
      std::string missing;
      if (!fs::is_regular_file(SiblingRunner(argv[0])))
      {
        missing = "The game runner (" MODERNGEKKO_RUNNER_FILENAME ") is missing from this "
                  "folder. Reinstall the app files next to the launcher.";
      }
      else
      {
        const fs::path module =
            release_directory /
            ("g" + current_metadata.metadata->disc_id + "_recomp"
#if defined(_WIN32)
             ".dll"
#else
             ".so"
#endif
             );
        if (!fs::is_regular_file(module) &&
            !fs::is_regular_file(user_directory / "StaticRecompModules" / module.filename()))
        {
          missing = "The recompiled game module (" + module.filename().string() +
                    ") is missing from this folder. Reinstall the app files next to "
                    "the launcher.";
        }
      }
      if (!missing.empty())
      {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Launch failed", missing.c_str(),
                                 window);
        result = 1;
      }
      else
      {
        std::vector<std::string> argument_storage = {
            SiblingRunner(argv[0]).string(), "--game", current_game.string(),
            "--user-dir", user_directory.string(), "--core", config.cpu_core};
        // Solo only: see the note by the dropdown.
        if (launch_mode == LaunchMode::Solo && selected_savestate >= 0 &&
            static_cast<std::size_t>(selected_savestate) < launcher_savestates.size())
        {
          argument_storage.emplace_back("--load-state");
          argument_storage.emplace_back(
              moderngekko::frontend::PathText(
                  launcher_savestates[static_cast<std::size_t>(selected_savestate)]));
        }
        if (launch_mode == LaunchMode::Host)
          argument_storage.emplace_back("--netplay-host");
        else if (launch_mode == LaunchMode::Join)
        {
          argument_storage.emplace_back("--netplay-join");
          argument_storage.emplace_back(config.netplay_address);
        }
        if (launch_mode == LaunchMode::Host || launch_mode == LaunchMode::Join)
        {
          argument_storage.emplace_back("--netplay-port");
          argument_storage.emplace_back(std::to_string(config.netplay_port));
          argument_storage.emplace_back("--nickname");
          argument_storage.emplace_back(config.netplay_nickname);
          argument_storage.emplace_back("--buffer");
          argument_storage.emplace_back(config.netplay_buffer);
          for (const std::string& controller : config.controllers)
          {
            argument_storage.emplace_back("--controller");
            argument_storage.emplace_back(controller);
          }
        }
        if (use_wayland)
          argument_storage.emplace_back("--wayland");
  #if defined(__linux__)
        else
          argument_storage.emplace_back("-X11");
  #endif
        std::vector<const char*> arguments;
        arguments.reserve(argument_storage.size() + 1);
        for (const std::string& argument : argument_storage)
          arguments.push_back(argument.c_str());
        arguments.push_back(nullptr);
        const std::filesystem::path log_path =
            user_directory / "Logs" / MODERNGEKKO_LOG_FILENAME;
        std::error_code log_error;
        std::filesystem::create_directories(log_path.parent_path(), log_error);
        SDL_IOStream* log_stream =
            log_error ? nullptr : SDL_IOFromFile(log_path.string().c_str(), "w");
        SDL_Process* process = nullptr;
        std::string process_error;
        if (log_stream)
        {
          const SDL_PropertiesID properties = SDL_CreateProperties();
          if (properties)
          {
            SDL_SetPointerProperty(properties, SDL_PROP_PROCESS_CREATE_ARGS_POINTER,
                                   arguments.data());
            SDL_SetNumberProperty(properties, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER,
                                  SDL_PROCESS_STDIO_REDIRECT);
            SDL_SetPointerProperty(properties, SDL_PROP_PROCESS_CREATE_STDOUT_POINTER, log_stream);
            SDL_SetBooleanProperty(properties, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN,
                                   true);
            process = SDL_CreateProcessWithProperties(properties);
            SDL_DestroyProperties(properties);
          }
          if (!process)
            process_error = SDL_GetError();
          SDL_CloseIO(log_stream);
        }
        else
        {
          process = SDL_CreateProcess(arguments.data(), false);
          if (!process)
            process_error = SDL_GetError();
        }
        if (!process)
        {
          launch_error = "Could not start " + SiblingRunner(argv[0]).string() + ": " + process_error;
          SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Launch failed", launch_error.c_str(),
                                   window);
          result = 1;
        }
        else
        {
          SDL_HideWindow(window);
          int exit_code = 1;
          bool waited = false;
          const Uint64 deadline = SDL_GetTicks() + 15000;
          while (SDL_GetTicks() < deadline)
          {
            if (SDL_WaitProcess(process, false, &exit_code))
            {
              waited = true;
              break;
            }
            SDL_Delay(50);
          }
          if (!waited)
          {
            // The runner has its own 3 s power-button grace. If it still has
            // not exited after 15 s, kill the whole tree rather than leaving a
            // hidden process or opening a modal dialog.
            SDL_KillProcess(process, true);
            SDL_WaitProcess(process, true, &exit_code);
            waited = true;
            exit_code = 0;
          }
          SDL_DestroyProcess(process);
          if (!waited || exit_code != 0)
          {
            SDL_ShowWindow(window);
            if (!waited)
            {
              launch_error =
                  "The game process could not be monitored: " + std::string(SDL_GetError());
            }
            else if (launch_mode == LaunchMode::Join)
            {
              switch (static_cast<moderngekko::frontend::NetplayExitCode>(exit_code))
              {
              case moderngekko::frontend::NetplayExitCode::VersionMismatch:
                launch_error = "The host is running an incompatible netplay build. Both "
                               "players must use the same release.";
                break;
              case moderngekko::frontend::NetplayExitCode::CompatibilityMismatch:
                launch_error = "The extracted game or recomp module does not match the "
                               "host. Both players need the same game revision and "
                               "release.";
                break;
              case moderngekko::frontend::NetplayExitCode::RoomFull:
                launch_error = "All four controller slots are already occupied.";
                break;
              case moderngekko::frontend::NetplayExitCode::GameRunning:
                launch_error = "The host has already started the game.";
                break;
              case moderngekko::frontend::NetplayExitCode::ServerFull:
                launch_error = "The netplay server is full.";
                break;
              case moderngekko::frontend::NetplayExitCode::NicknameRejected:
                launch_error = "The nickname was rejected by the host.";
                break;
              default:
                launch_error = "Could not reach the netplay host. Check the host name, UDP "
                               "port, and firewall.";
                break;
              }
            }
            else if (launch_mode == LaunchMode::Host)
            {
              launch_error = "Could not create the netplay session. Check the UDP port "
                             "and firewall settings.";
            }
            else
            {
              launch_error = "The game process exited with code " + std::to_string(exit_code) + ".";
            }
            if (!log_error)
              launch_error += "\n\nDetails were written to:\n" + log_path.string();
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Session failed", launch_error.c_str(),
                                     window);
            result = 1;
          }
        }
      }
    }
  }

  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  if (artwork.background)
  {
    SDL_DestroyTexture(artwork.background);
    artwork.background = nullptr;
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return result;
}

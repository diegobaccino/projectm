// Keep it as the first line, as SDL2 will otherwise redefine
// ProjectMSDLApplication::main to ProjectMSDLApplication::SDL_main
#define SDL_MAIN_HANDLED

#include "ProjectMSDLApplication.h"

#include "AudioCapture.h"
#include "ProjectMWrapper.h"
#include "RenderLoop.h"
#include "SDLRenderingWindow.h"
#include "gui/ProjectMGUI.h"

#include <Poco/Environment.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>
#include <Poco/String.h>

#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/PropertyFileConfiguration.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <set>

#ifdef _WIN32
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

namespace
{
std::vector<std::string> CollectLeafKeys(Poco::Util::AbstractConfiguration& config, const std::string& basePath = "")
{
    std::vector<std::string> keys;
    Poco::Util::AbstractConfiguration::Keys children;
    config.keys(basePath, children);

    for (const auto& child : children)
    {
        std::string key = basePath.empty() ? child : (basePath + "." + child);
        Poco::Util::AbstractConfiguration::Keys subChildren;
        config.keys(key, subChildren);
        if (subChildren.empty())
        {
            keys.push_back(key);
        }
        else
        {
            auto nested = CollectLeafKeys(config, key);
            keys.insert(keys.end(), nested.begin(), nested.end());
        }
    }

    return keys;
}

void ApplyConfiguration(Poco::Util::PropertyFileConfiguration& target,
                        Poco::Util::PropertyFileConfiguration& source)
{
    auto currentKeys = CollectLeafKeys(target);
    auto nextKeys = CollectLeafKeys(source);

    std::set<std::string> nextKeySet(nextKeys.begin(), nextKeys.end());

    for (const auto& key : currentKeys)
    {
        if (nextKeySet.find(key) == nextKeySet.end())
        {
            target.remove(key);
        }
    }

    for (const auto& key : nextKeys)
    {
        target.setString(key, source.getString(key));
    }
}
}

ProjectMSDLApplication::ProjectMSDLApplication()
    : Poco::Util::Application()
{
    // Note: order here is important, as subsystems are initialized in the same order.
    addSubsystem(new SDLRenderingWindow);
    addSubsystem(new ProjectMWrapper);
    addSubsystem(new AudioCapture);
    addSubsystem(new ProjectMGUI);
}

const char* ProjectMSDLApplication::name() const
{
    return "projectMSDL";
}

ProjectMSDLApplication& ProjectMSDLApplication::instance()
{
    return dynamic_cast<ProjectMSDLApplication&>(Poco::Util::Application::instance());
}

Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> ProjectMSDLApplication::UserConfiguration()
{
    return _userConfiguration;
}

Poco::AutoPtr<Poco::Util::MapConfiguration> ProjectMSDLApplication::CommandLineConfiguration()
{
    return _commandLineOverrides;
}

const std::vector<ProjectMSDLApplication::Profile>& ProjectMSDLApplication::Profiles() const
{
    return _profiles;
}

size_t ProjectMSDLApplication::ActiveProfileIndex() const
{
    return _activeProfileIndex;
}

const std::string& ProjectMSDLApplication::ActiveProfileName() const
{
    return _profiles.at(_activeProfileIndex).name;
}

bool ProjectMSDLApplication::SwitchProfileByIndex(size_t index, std::string& message)
{
    if (index >= _profiles.size())
    {
        message = "Profile slot out of range.";
        return false;
    }

    if (index == _activeProfileIndex)
    {
        message = "Profile already active: " + _profiles[index].name;
        return true;
    }

    try
    {
        _userConfiguration->save(ActiveProfilePath());

        auto targetPath = ProfilePath(_profiles[index]);
        if (!Poco::File(targetPath).exists())
        {
            Poco::File(targetPath).createFile();
        }

        Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> targetConfig = new Poco::Util::PropertyFileConfiguration();
        targetConfig->load(targetPath);

        ApplyConfiguration(*_userConfiguration, *targetConfig);

        _activeProfileIndex = index;
        _commandLineOverrides->setString("app.UserConfigurationFile", targetPath);
        _commandLineOverrides->setString("app.ActiveProfileName", _profiles[index].name);

        std::string saveError;
        if (!SaveProfilesIndex(saveError))
        {
            poco_warning_f1(logger(), "Profile switch succeeded but index file was not updated: %s", saveError);
        }

        _userConfiguration->save(targetPath);
        message = "Switched to profile " + _profiles[index].name;
        return true;
    }
    catch (const Poco::Exception& ex)
    {
        message = "Failed to switch profile: " + ex.displayText();
        return false;
    }
}

bool ProjectMSDLApplication::CreateProfile(const std::string& name, std::string& message)
{
    const auto trimmedName = Trim(name);
    if (!IsProfileNameValid(trimmedName))
    {
        message = "Profile name must be 1-32 chars and use letters, numbers, space, _, or -.";
        return false;
    }

    if (_profiles.size() >= MaxProfiles())
    {
        message = "Maximum of 9 profiles reached.";
        return false;
    }

    const auto wantedName = ToLowerAscii(trimmedName);
    for (const auto& profile : _profiles)
    {
        if (ToLowerAscii(profile.name) == wantedName)
        {
            message = "A profile with that name already exists.";
            return false;
        }
    }

    try
    {
        size_t nextId = 1;
        std::set<std::string> existingFiles;
        for (const auto& profile : _profiles)
        {
            existingFiles.insert(profile.fileName);
        }

        std::string fileName;
        do
        {
            fileName = "profile-" + std::to_string(nextId++) + ".properties";
        }
        while (existingFiles.find(fileName) != existingFiles.end());

        auto profileFilePath = ProfilePath({trimmedName, fileName});
        Poco::File(profileFilePath).createFile();

        _profiles.push_back({trimmedName, fileName});
        std::string saveError;
        if (!SaveProfilesIndex(saveError))
        {
            message = "Created profile but failed to update profile list: " + saveError;
            return false;
        }

        message = "Created profile " + trimmedName;
        return true;
    }
    catch (const Poco::Exception& ex)
    {
        message = "Failed to create profile: " + ex.displayText();
        return false;
    }
}

bool ProjectMSDLApplication::DeleteProfileByIndex(size_t index, std::string& message)
{
    if (index >= _profiles.size())
    {
        message = "Profile slot out of range.";
        return false;
    }

    if (index == 0)
    {
        message = "The Default profile cannot be deleted.";
        return false;
    }

    auto deletingActive = index == _activeProfileIndex;
    auto profileToDelete = _profiles[index];

    try
    {
        _userConfiguration->save(ActiveProfilePath());

        auto fileToDelete = ProfilePath(profileToDelete);
        if (Poco::File(fileToDelete).exists())
        {
            Poco::File(fileToDelete).remove();
        }

        _profiles.erase(_profiles.begin() + static_cast<std::ptrdiff_t>(index));

        if (_profiles.empty())
        {
            _profiles.push_back({"Default", "default.properties"});
        }

        if (deletingActive)
        {
            _activeProfileIndex = 0;

            std::string loadError;
            if (!LoadActiveProfileConfiguration(loadError))
            {
                message = "Profile deleted, but failed to load Default profile: " + loadError;
                return false;
            }
        }
        else if (_activeProfileIndex > index)
        {
            --_activeProfileIndex;
        }

        std::string saveError;
        if (!SaveProfilesIndex(saveError))
        {
            message = "Profile deleted, but failed to update profile list: " + saveError;
            return false;
        }

        message = "Deleted profile " + profileToDelete.name;
        return true;
    }
    catch (const Poco::Exception& ex)
    {
        message = "Failed to delete profile: " + ex.displayText();
        return false;
    }
}

void ProjectMSDLApplication::initialize(Poco::Util::Application& self)
{
    // Application settings are PRIO_APPLICATION, higher values have lower precedence.
    // So we put command-line overrides just below settings changed in the UI.
    config().add(_commandLineOverrides, PRIO_APPLICATION + 10);

    std::string configFileName = config().getString("application.baseName") + ".properties";
    Poco::Path userConfigurationDir = Poco::Path::configHome();
    userConfigurationDir.makeDirectory().append("projectM/");
    Poco::Path legacyUserConfigurationFile = userConfigurationDir;
    legacyUserConfigurationFile.setFileName(configFileName);

    try
    {
        if (loadConfiguration(PRIO_DEFAULT) == 0)
        {
            // The file may be located in the ../Resources bundle dir on macOS, elsewhere relative
            // to the executable or within an absolute path.
            // By setting and retrieving the compiled-in default, we can make use of POCO's variable replacement.
            // This allows using ${application.dir} etc. in the path.
            config().setString("application.defaultConfigurationFile", PROJECTMSDL_CONFIG_LOCATION);
            std::string configPath = config().getString("application.defaultConfigurationFile", "");
            if (!configPath.empty())
            {
                Poco::Path configFilePath(configPath);
                configFilePath.makeDirectory().setFileName(configFileName);
                poco_information_f1(logger(), "Trying to load configuration from %s.", configFilePath.toString());
                if (Poco::File(configFilePath).exists())
                {
                    loadConfiguration(configFilePath.toString(), PRIO_DEFAULT);
                }
            }
        }
    }
    catch (Poco::Exception& ex)
    {
        poco_error_f1(logger(), "Failed to load default configuration file: %s", ex.displayText());
    }

    try
    {
        InitializeProfiles(configFileName, legacyUserConfigurationFile.toString());

        std::string promptError;
        if (!PromptForProfileSelection(promptError))
        {
            poco_warning_f1(logger(), "Profile selection prompt warning: %s", promptError);
        }

        std::string loadError;
        if (!LoadActiveProfileConfiguration(loadError))
        {
            throw Poco::Exception(loadError);
        }

        config().add(_userConfiguration, PRIO_DEFAULT - 10);
    }
    catch (Poco::Exception& ex)
    {
        poco_error_f1(logger(), "Failed to initialize profile system: %s", ex.displayText());
    }

    Application::initialize(self);
}

void ProjectMSDLApplication::InitializeProfiles(const std::string& configFileName, const std::string& legacyConfigFile)
{
    POCO_UNUSED(configFileName);
    POCO_UNUSED(legacyConfigFile);

    Poco::Path profilesDirectory = Poco::Path::configHome();
    profilesDirectory.makeDirectory().append("projectM/").append("profiles/");
    Poco::File(profilesDirectory).createDirectories();

    _profilesDirectory = profilesDirectory.toString();
    Poco::Path profilesIndexFile = profilesDirectory;
    profilesIndexFile.setFileName("profiles.properties");
    _profilesIndexFile = profilesIndexFile.toString();

    LoadProfilesIndex();
    EnsureDefaultProfileExists();

    if (_profiles.empty())
    {
        _profiles.push_back({"Default", "default.properties"});
        _activeProfileIndex = 0;
    }

    if (_activeProfileIndex >= _profiles.size())
    {
        _activeProfileIndex = 0;
    }

    for (const auto& profile : _profiles)
    {
        auto profilePath = ProfilePath(profile);
        if (!Poco::File(profilePath).exists())
        {
            Poco::File(profilePath).createFile();
        }
    }

    std::string saveError;
    SaveProfilesIndex(saveError);
}

void ProjectMSDLApplication::EnsureDefaultProfileExists()
{
    bool hasDefault = false;
    for (const auto& profile : _profiles)
    {
        if (ToLowerAscii(profile.name) == "default")
        {
            hasDefault = true;
            break;
        }
    }

    if (!hasDefault)
    {
        _profiles.insert(_profiles.begin(), {"Default", "default.properties"});
        _activeProfileIndex = 0;
    }
}

void ProjectMSDLApplication::LoadProfilesIndex()
{
    _profiles.clear();
    _activeProfileIndex = 0;

    if (_profilesIndexFile.empty() || !Poco::File(_profilesIndexFile).exists())
    {
        _profiles.push_back({"Default", "default.properties"});
        _activeProfileIndex = 0;
        return;
    }

    Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> indexConfig = new Poco::Util::PropertyFileConfiguration();
    indexConfig->load(_profilesIndexFile);

    size_t index = 0;
    while (_profiles.size() < MaxProfiles())
    {
        auto baseKey = std::string("profile.") + std::to_string(index + 1);
        auto name = Trim(indexConfig->getString(baseKey + ".name", ""));
        auto fileName = Trim(indexConfig->getString(baseKey + ".file", ""));
        if (name.empty() || fileName.empty())
        {
            break;
        }

        _profiles.push_back({name, fileName});
        ++index;
    }

    if (_profiles.empty())
    {
        _profiles.push_back({"Default", "default.properties"});
    }

    auto activeName = ToLowerAscii(indexConfig->getString("active", "Default"));
    for (size_t i = 0; i < _profiles.size(); ++i)
    {
        if (ToLowerAscii(_profiles[i].name) == activeName)
        {
            _activeProfileIndex = i;
            break;
        }
    }
}

bool ProjectMSDLApplication::SaveProfilesIndex(std::string& errorMessage) const
{
    try
    {
        Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> indexConfig = new Poco::Util::PropertyFileConfiguration();
        for (size_t i = 0; i < _profiles.size() && i < MaxProfiles(); ++i)
        {
            auto baseKey = std::string("profile.") + std::to_string(i + 1);
            indexConfig->setString(baseKey + ".name", _profiles[i].name);
            indexConfig->setString(baseKey + ".file", _profiles[i].fileName);
        }
        indexConfig->setString("active", _profiles.at(_activeProfileIndex).name);
        indexConfig->save(_profilesIndexFile);
        return true;
    }
    catch (const Poco::Exception& ex)
    {
        errorMessage = ex.displayText();
        return false;
    }
}

bool ProjectMSDLApplication::PromptForProfileSelection(std::string& errorMessage)
{
    if (ISATTY(FILENO(stdin)) == 0)
    {
        return true;
    }

    while (true)
    {
        std::cout << "\nSelect projectM profile:\n";
        for (size_t i = 0; i < _profiles.size(); ++i)
        {
            std::cout << "  " << (i + 1) << ") " << _profiles[i].name;
            if (i == _activeProfileIndex)
            {
                std::cout << " [active]";
            }
            std::cout << "\n";
        }
        std::cout << "  n) New profile\n";
        std::cout << "  d) Delete profile\n";
        std::cout << "Enter choice (1-" << _profiles.size() << ", n, d, or Enter for active): ";

        std::string input;
        if (!std::getline(std::cin, input))
        {
            errorMessage = "Input stream unavailable; using active profile " + _profiles[_activeProfileIndex].name;
            return false;
        }

        input = Trim(input);
        if (input.empty())
        {
            return true;
        }

        auto lower = ToLowerAscii(input);
        if (lower == "n")
        {
            if (_profiles.size() >= MaxProfiles())
            {
                std::cout << "Maximum of 9 profiles reached.\n";
                continue;
            }

            std::cout << "New profile name: ";
            std::string profileName;
            if (!std::getline(std::cin, profileName))
            {
                errorMessage = "Failed to read profile name.";
                return false;
            }

            std::string message;
            if (!CreateProfile(profileName, message))
            {
                std::cout << message << "\n";
                continue;
            }

            std::cout << message << "\n";
            continue;
        }

        if (lower == "d")
        {
            std::cout << "Delete which profile number? ";
            std::string numberInput;
            if (!std::getline(std::cin, numberInput))
            {
                errorMessage = "Failed to read delete profile number.";
                return false;
            }

            const auto deleteIndexRaw = Trim(numberInput);
            if (deleteIndexRaw.empty())
            {
                continue;
            }

            int deleteIndex = 0;
            try
            {
                deleteIndex = std::stoi(deleteIndexRaw);
            }
            catch (...)
            {
                std::cout << "Invalid profile number.\n";
                continue;
            }

            std::string message;
            if (!DeleteProfileByIndex(static_cast<size_t>(deleteIndex - 1), message))
            {
                std::cout << message << "\n";
                continue;
            }

            std::cout << message << "\n";
            continue;
        }

        try
        {
            int selectedIndex = std::stoi(input);
            if (selectedIndex < 1 || selectedIndex > static_cast<int>(_profiles.size()))
            {
                std::cout << "Invalid profile number.\n";
                continue;
            }

            _activeProfileIndex = static_cast<size_t>(selectedIndex - 1);
            std::string saveError;
            SaveProfilesIndex(saveError);
            return true;
        }
        catch (...)
        {
            std::cout << "Please enter a number, n, d, or Enter.\n";
        }
    }
}

bool ProjectMSDLApplication::LoadActiveProfileConfiguration(std::string& errorMessage)
{
    try
    {
        auto configFilePath = ActiveProfilePath();
        if (!Poco::File(configFilePath).exists())
        {
            Poco::File(configFilePath).createFile();
        }

        _userConfiguration->load(configFilePath);
        _commandLineOverrides->setString("app.UserConfigurationFile", configFilePath);
        _commandLineOverrides->setString("app.ActiveProfileName", ActiveProfileName());
        return true;
    }
    catch (const Poco::Exception& ex)
    {
        errorMessage = ex.displayText();
        return false;
    }
}

std::string ProjectMSDLApplication::ActiveProfilePath() const
{
    return ProfilePath(_profiles.at(_activeProfileIndex));
}

std::string ProjectMSDLApplication::ProfilePath(const Profile& profile) const
{
    Poco::Path profilePath(_profilesDirectory);
    profilePath.setFileName(profile.fileName);
    return profilePath.toString();
}

bool ProjectMSDLApplication::IsProfileNameValid(const std::string& name)
{
    if (name.empty() || name.size() > 32)
    {
        return false;
    }

    return std::all_of(name.begin(), name.end(), [](unsigned char ch) {
        return std::isalnum(ch) || ch == ' ' || ch == '_' || ch == '-';
    });
}

std::string ProjectMSDLApplication::Trim(const std::string& value)
{
    return Poco::trim(value);
}

std::string ProjectMSDLApplication::ToLowerAscii(const std::string& value)
{
    return Poco::toLower(value);
}

void ProjectMSDLApplication::uninitialize()
{
    Application::uninitialize();
}

void ProjectMSDLApplication::defineOptions(Poco::Util::OptionSet& options)
{
    using Poco::Util::Option;
    using Poco::Util::OptionCallback;

    options.addOption(Option("help", "h", "Display this help text and exit.")
                          .callback(
                              OptionCallback<ProjectMSDLApplication>(this, &ProjectMSDLApplication::DisplayHelp)));

    options.addOption(Option("listAudioDevices", "l",
                             "Output a list of available audio recording devices on startup.")
                          .callback(
                              OptionCallback<ProjectMSDLApplication>(this, &ProjectMSDLApplication::ListAudioDevices)));

    options.addOption(Option("audioDevice", "d",
                             "Select an audio device to record from initially. Can be the numerical ID or the full device name. "
                             "If the device is not found, the default device will be used instead.",
                             false, "<id or name>", true)
                          .binding("audio.device", _commandLineOverrides));

    options.addOption(Option("presetPath", "p", "Base directory to search for presets.",
                             false, "<path>", true)
                          .binding("projectM.presetPath", _commandLineOverrides));

    options.addOption(Option("texturePath", "", "Additional path with textures/images.",
                             false, "<path>", true)
                          .binding("projectM.texturePath", _commandLineOverrides));

    options.addOption(Option("enableSplash", "s", "If true, initially displays the built-in projectM logo preset.",
                             false, "<0/1>", true)
                          .binding("projectM.enableSplash", _commandLineOverrides));

    options.addOption(Option("fullscreen", "f", "Start in fullscreen mode.",
                             false, "<0/1>", true)
                          .binding("window.fullscreen", _commandLineOverrides));

    options.addOption(Option("exclusive", "e",
                             "Use exclusive fullscreen mode. If true, this will change display resolution on most platforms to best match the window resolution.",
                             false, "<0/1>", true)
                          .binding("window.fullscreen.exclusiveMode", _commandLineOverrides));

    options.addOption(Option("monitor", "",
                             "Displays the window on the given monitor. 0 uses OS default window position, 1 is the primary display and so on.",
                             false, "<number>", true)
                          .binding("window.monitor", _commandLineOverrides));

    options.addOption(Option("vsync", "",
                             "If true, waits for vertical sync to avoid tearing, but limits max FPS to the vsync interval.",
                             false, "<0/1>", true)
                          .binding("window.waitForVerticalSync", _commandLineOverrides));

    options.addOption(Option("vsyncAdaptive", "",
                             "If true and vsync is enabled, tries to use adaptive vsync. Set FPS to 0 for best results.",
                             false, "<0/1>", true)
                          .binding("window.adaptiveVerticalSync", _commandLineOverrides));

    options.addOption(Option("width", "", "Initial window width.",
                             false, "<number>", true)
                          .binding("window.width", _commandLineOverrides));

    options.addOption(Option("height", "", "Initial window height.",
                             false, "<number>", true)
                          .binding("window.height", _commandLineOverrides));

    options.addOption(Option("fullscreenWidth", "", "Fullscreen horizontal resolution.",
                             false, "<number>", true)
                          .binding("window.fullscreen.width", _commandLineOverrides));

    options.addOption(Option("fullscreenHeight", "", "Fullscreen vertical resolution.",
                             false, "<number>", true)
                          .binding("window.fullscreen.height", _commandLineOverrides));

    options.addOption(Option("left", "", "Initial window X position.",
                             false, "<number>", true)
                          .binding("window.left", _commandLineOverrides));

    options.addOption(Option("top", "", "Initial window Y position.",
                             false, "<number>", true)
                          .binding("window.top", _commandLineOverrides));

    options.addOption(Option("fps", "", "Target frames per second rate.",
                             false, "<number>", true)
                          .binding("projectM.fps", _commandLineOverrides));

    options.addOption(Option("shuffleEnabled", "", "Shuffle enabled.",
                             false, "<0/1>", true)
                          .binding("projectM.shuffleEnabled", _commandLineOverrides));

    options.addOption(Option("skipToDropped", "", "Skip to drag & dropped presets",
                             false, "<0/1>", true)
                          .binding("projectM.skipToDropped", _commandLineOverrides));

    options.addOption(Option("droppedFolderOverride", "", "When dropping a folder, clear the playlist and add all presets from the folder.",
                             false, "<0/1>", true)
                          .binding("projectM.droppedFolderOverride", _commandLineOverrides));

    options.addOption(Option("presetDuration", "", "Preset duration. Any number > 1, default 30.",
                             false, "<number>", true)
                          .binding("projectM.displayDuration", _commandLineOverrides));

    options.addOption(Option("transitionDuration", "", "Transition duration. Any number >= 0, default 3.",
                             false, "<number>", true)
                          .binding("projectM.transitionDuration", _commandLineOverrides));

    options.addOption(Option("hardCutsEnabled", "", "Hard cuts enabled.",
                             false, "<0/1>", true)
                          .binding("projectM.hardCutsEnabled", _commandLineOverrides));

    options.addOption(Option("hardCutDuration", "", "Hard cut duration. Any number > 1, default 20.",
                             false, "<number>", true)
                          .binding("projectM.hardCutDuration", _commandLineOverrides));

    options.addOption(Option("hardCutSensitivity", "", "Hard cut sensitivity. Between 0.0 and 5.0. Default 1.0.",
                             false, "<number>", true)
                          .binding("projectM.hardCutSensitivity", _commandLineOverrides));

    options.addOption(Option("beatSensitivity", "", "Beat sensitivity. Between 0.0 and 2.0. Default 1.0.",
                             false, "<number>", true)
                          .binding("projectM.beatSensitivity", _commandLineOverrides));
}

int ProjectMSDLApplication::main(POCO_UNUSED const std::vector<std::string>& args)
{
    RenderLoop renderLoop;
    renderLoop.Run();

    return EXIT_SUCCESS;
}

void ProjectMSDLApplication::DisplayHelp(POCO_UNUSED const std::string& name, POCO_UNUSED const std::string& value)
{
    Poco::Util::HelpFormatter formatter(options());

    SDL_version sdlBuild;
    SDL_version sdlLoaded;

    SDL_VERSION(&sdlBuild);
    SDL_GetVersion(&sdlLoaded);

    auto* projectMVersion = projectm_get_version_string();
    std::string projectMRuntimeVersion(projectMVersion);
    projectm_free_string(projectMVersion);

    formatter.setUsage(config().getString("application.name") + " [options]");
    formatter.setHeader(Poco::format(R"(
projectM SDL Standalone Visualizer

Licensed under the GNU General Public License 3.0

Application version: %s
Built/running with projectM4 version: %s / %s
Built against SDL version: %?d.%?d.%?d (running with %?d.%?d.%?d))",
                                     std::string(PROJECTMSDL_VERSION),
                                     std::string(PROJECTM_VERSION_STRING),
                                     projectMRuntimeVersion,
                                     sdlBuild.major, sdlBuild.minor, sdlBuild.patch,
                                     sdlLoaded.major, sdlLoaded.minor, sdlLoaded.patch));

    formatter.format(std::cerr);

    exit(EXIT_SUCCESS);
}

void ProjectMSDLApplication::ListAudioDevices(POCO_UNUSED const std::string& name, POCO_UNUSED const std::string& value)
{
    _commandLineOverrides->setBool("audio.listDevices", true);
}

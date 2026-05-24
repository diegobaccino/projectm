#pragma once

#include <Poco/Util/Application.h>
#include <Poco/Util/MapConfiguration.h>
#include <Poco/Util/PropertyFileConfiguration.h>

#include <vector>

class ProjectMSDLApplication : public Poco::Util::Application
{
public:
    ProjectMSDLApplication();

    struct Profile
    {
        std::string name;
        std::string fileName;
    };

    const char* name() const override;

    /**
     * @brief Returns the instance of the projectMSDL application.
     * @return The instance of the projectMSDL application.
     */
    static ProjectMSDLApplication& instance();

    /**
     * @brief Returns the user configuration layer.
     * @return The configuration instance which stores the settings for the current user.
     */
    Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> UserConfiguration();

    /**
     * @brief Returns the command line override map.
     * @return The properties file instance which stores the UI settings.
     */
    Poco::AutoPtr<Poco::Util::MapConfiguration> CommandLineConfiguration();

    /**
     * @brief Returns all available profiles in keyboard-slot order.
     */
    const std::vector<Profile>& Profiles() const;

    /**
     * @brief Returns index of the currently active profile.
     */
    size_t ActiveProfileIndex() const;

    /**
     * @brief Returns the currently active profile name.
     */
    const std::string& ActiveProfileName() const;

    /**
     * @brief Tries to switch to profile by slot index.
     */
    bool SwitchProfileByIndex(size_t index, std::string& message);

    /**
     * @brief Creates a new empty profile.
     */
    bool CreateProfile(const std::string& name, std::string& message);

    /**
     * @brief Deletes a profile by slot index. The default profile cannot be deleted.
     */
    bool DeleteProfileByIndex(size_t index, std::string& message);

    /**
     * @brief Maximum number of supported profiles.
     */
    static constexpr size_t MaxProfiles()
    {
        return 9;
    }

protected:
    void initialize(Application& self) override;

    void uninitialize() override;

    void defineOptions(Poco::Util::OptionSet& options) override;

    int main(const std::vector<std::string>& args) override;

    /**
     * @brief Display help and exit.
     * @param name Unused.
     * @param value Unused.
     */
    void DisplayHelp(const std::string& name, const std::string& value);

    void ListAudioDevices(const std::string& name, const std::string& value);

    Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> _userConfiguration{
        new Poco::Util::PropertyFileConfiguration()}; //!< The current user's configuration, used to store/reset changes made in the UI's settings dialog.
    Poco::AutoPtr<Poco::Util::MapConfiguration> _commandLineOverrides{
        new Poco::Util::MapConfiguration()}; //!< Map configuration with overrides set by command line arguments.

    std::vector<Profile> _profiles;
    size_t _activeProfileIndex{0};
    std::string _profilesDirectory;
    std::string _profilesIndexFile;

    void InitializeProfiles(const std::string& configFileName, const std::string& legacyConfigFile);
    void EnsureDefaultProfileExists();
    void LoadProfilesIndex();
    bool SaveProfilesIndex(std::string& errorMessage) const;
    bool PromptForProfileSelection(std::string& errorMessage);
    bool LoadActiveProfileConfiguration(std::string& errorMessage);
    std::string ActiveProfilePath() const;
    std::string ProfilePath(const Profile& profile) const;
    static bool IsProfileNameValid(const std::string& name);
    static std::string Trim(const std::string& value);
    static std::string ToLowerAscii(const std::string& value);
};

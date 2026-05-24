#pragma once

#include <string>

#include <array>

class ProjectMGUI;
class ProjectMWrapper;
class AudioCapture;

namespace Poco {
class NotificationCenter;
}

class MainMenu
{
public:
    MainMenu() = delete;

    explicit MainMenu(ProjectMGUI& gui);

    /**
     * @brief Draws the main menu bar.
     */
    void Draw();

private:
    void DrawCreateProfilePopup();
    void DrawDeleteProfilePopup();

    Poco::NotificationCenter& _notificationCenter; //!< Notification center instance.
    ProjectMGUI& _gui; //!< Reference to the GUI subsystem.
    ProjectMWrapper& _projectMWrapper; //!< Reference to the projectM wrapper subsystem.
    AudioCapture& _audioCapture; //!< Reference to the audio capture subsystem.

    std::array<char, 64> _newProfileName{};
    int _deleteProfileSelection{1};
    bool _openCreateProfilePopup{false};
    bool _openDeleteProfilePopup{false};
};

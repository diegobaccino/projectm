#include "gui/MainMenu.h"

#include "AudioCapture.h"
#include "ProjectMSDLApplication.h"
#include "ProjectMWrapper.h"

#include "gui/ProjectMGUI.h"
#include "gui/SystemBrowser.h"

#include "notifications/PlaybackControlNotification.h"
#include "notifications/QuitNotification.h"
#include "notifications/UpdateWindowTitleNotification.h"
#include "notifications/DisplayToastNotification.h"

#include "imgui.h"

#include <Poco/NotificationCenter.h>

#include <algorithm>


MainMenu::MainMenu(ProjectMGUI& gui)
    : _notificationCenter(Poco::NotificationCenter::defaultCenter())
    , _gui(gui)
    , _projectMWrapper(Poco::Util::Application::instance().getSubsystem<ProjectMWrapper>())
    , _audioCapture(Poco::Util::Application::instance().getSubsystem<AudioCapture>())
{
}

void MainMenu::Draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            auto& app = ProjectMSDLApplication::instance();

            if (ImGui::BeginMenu("Switch Profile"))
            {
                const auto& profiles = app.Profiles();
                const auto activeIndex = app.ActiveProfileIndex();

                for (size_t i = 0; i < profiles.size(); ++i)
                {
                    const auto shortcut = (i < 9) ? ("Ctrl+" + std::to_string(i + 1)) : "";
                    const auto label = std::to_string(i + 1) + ". " + profiles[i].name;

                    if (ImGui::MenuItem(label.c_str(), shortcut.c_str(), i == activeIndex))
                    {
                        std::string message;
                        if (app.SwitchProfileByIndex(i, message))
                        {
                            _projectMWrapper.LoadLastPresetForActiveProfile();
                            _notificationCenter.postNotification(new DisplayToastNotification(message));
                        }
                        else
                        {
                            _notificationCenter.postNotification(new DisplayToastNotification("Profile switch failed: " + message));
                        }
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Create Profile..."))
            {
                std::fill(_newProfileName.begin(), _newProfileName.end(), '\0');
                _openCreateProfilePopup = true;
            }

            if (ImGui::MenuItem("Delete Profile..."))
            {
                _deleteProfileSelection = static_cast<int>(app.ActiveProfileIndex());
                _openDeleteProfilePopup = true;
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Settings...", "Ctrl+s"))
            {
                _gui.ShowSettingsWindow();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Quit projectM", "Ctrl+q"))
            {
                _notificationCenter.postNotification(new QuitNotification);
            }

            ImGui::EndMenu();
        }

        if (_openCreateProfilePopup)
        {
            ImGui::OpenPopup("Create Profile");
            _openCreateProfilePopup = false;
        }

        if (_openDeleteProfilePopup)
        {
            ImGui::OpenPopup("Delete Profile");
            _openDeleteProfilePopup = false;
        }

        DrawCreateProfilePopup();
        DrawDeleteProfilePopup();

        if (ImGui::BeginMenu("Playback"))
        {
            auto& app = ProjectMSDLApplication::instance();

            if (ImGui::MenuItem("Play Next Preset", "n"))
            {
                _notificationCenter.postNotification(new PlaybackControlNotification(PlaybackControlNotification::Action::LastPreset));
            }
            if (ImGui::MenuItem("Play Previous Preset", "p"))
            {
                _notificationCenter.postNotification(new PlaybackControlNotification(PlaybackControlNotification::Action::PreviousPreset));
            }
            if (ImGui::MenuItem("Go Back One Preset", "Backspace"))
            {
                _notificationCenter.postNotification(new PlaybackControlNotification(PlaybackControlNotification::Action::LastPreset));
            }
            if (ImGui::MenuItem("Random Preset", "r"))
            {
                _notificationCenter.postNotification(new PlaybackControlNotification(PlaybackControlNotification::Action::RandomPreset));
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Lock Preset", "Spacebar", app.config().getBool("projectM.presetLocked", false)))
            {
                _notificationCenter.postNotification(new PlaybackControlNotification(PlaybackControlNotification::Action::TogglePresetLocked));
            }
            if (ImGui::MenuItem("Enable Shuffle", "y", app.config().getBool("projectM.shuffleEnabled", true)))
            {
                _notificationCenter.postNotification(new PlaybackControlNotification(PlaybackControlNotification::Action::ToggleShuffle));
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Copy Current Preset Filename", "Ctrl+c"))
            {
                _projectMWrapper.PresetFileNameToClipboard();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options"))
        {
            auto& app = ProjectMSDLApplication::instance();

            if (ImGui::BeginMenu("Audio Capture Device"))
            {
                auto devices = _audioCapture.AudioDeviceList();
                auto currentIndex = _audioCapture.AudioDeviceIndex();

                for (const auto& device : devices)
                {
                    if (ImGui::MenuItem(device.second.c_str(), "", device.first == currentIndex))
                    {
                        _audioCapture.AudioDeviceIndex(device.first);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Display Toast Messages", "", app.config().getBool("projectM.displayToasts", true)))
            {
                app.UserConfiguration()->setBool("projectM.displayToasts", !app.config().getBool("projectM.displayToasts", true));
            }
            if (ImGui::MenuItem("Display Preset Name in Window Title", "", app.config().getBool("window.displayPresetNameInTitle", true)))
            {
                app.UserConfiguration()->setBool("window.displayPresetNameInTitle", !app.config().getBool("window.displayPresetNameInTitle", true));
                _notificationCenter.postNotification(new UpdateWindowTitleNotification);
            }

            ImGui::Separator();

            float beatSensitivity = projectm_get_beat_sensitivity(_projectMWrapper.ProjectM());
            if (ImGui::SliderFloat("Beat Sensitivity", &beatSensitivity, 0.0f, 2.0f))
            {
                projectm_set_beat_sensitivity(_projectMWrapper.ProjectM(), beatSensitivity);
                app.UserConfiguration()->setDouble("projectM.beatSensitivity", beatSensitivity);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Quick Help..."))
            {
                _gui.ShowHelpWindow();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("About projectM..."))
            {
                _gui.ShowAboutWindow();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Visit the projectM Wiki on GitHub"))
            {
                SystemBrowser::OpenURL("https://github.com/projectM-visualizer/projectm/wiki");
            }
            if (ImGui::MenuItem("Report a Bug or Request a Feature"))
            {
                SystemBrowser::OpenURL("https://github.com/projectM-visualizer/projectm/issues/new/choose");
            }
            if (ImGui::MenuItem("Sponsor projectM on OpenCollective"))
            {
                SystemBrowser::OpenURL("https://opencollective.com/projectm");
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void MainMenu::DrawCreateProfilePopup()
{
    if (!ImGui::BeginPopupModal("Create Profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    ImGui::Text("Create a new profile (max 9).");
    ImGui::InputText("Name", _newProfileName.data(), _newProfileName.size());

    if (ImGui::Button("Create"))
    {
        std::string message;
        auto& app = ProjectMSDLApplication::instance();
        if (app.CreateProfile(_newProfileName.data(), message))
        {
            _notificationCenter.postNotification(new DisplayToastNotification(message));
            ImGui::CloseCurrentPopup();
        }
        else
        {
            _notificationCenter.postNotification(new DisplayToastNotification("Create profile failed: " + message));
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void MainMenu::DrawDeleteProfilePopup()
{
    if (!ImGui::BeginPopupModal("Delete Profile", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    auto& app = ProjectMSDLApplication::instance();
    const auto& profiles = app.Profiles();

    if (profiles.empty())
    {
        ImGui::Text("No profiles available.");
    }
    else
    {
        if (_deleteProfileSelection < 0 || _deleteProfileSelection >= static_cast<int>(profiles.size()))
        {
            _deleteProfileSelection = static_cast<int>(app.ActiveProfileIndex());
        }

        const auto selectedName = profiles[static_cast<size_t>(_deleteProfileSelection)].name;
        ImGui::Text("Delete profile:");
        if (ImGui::BeginCombo("##DeleteProfileCombo", selectedName.c_str()))
        {
            for (size_t i = 0; i < profiles.size(); ++i)
            {
                bool selected = _deleteProfileSelection == static_cast<int>(i);
                if (ImGui::Selectable(profiles[i].name.c_str(), selected))
                {
                    _deleteProfileSelection = static_cast<int>(i);
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    if (ImGui::Button("Delete"))
    {
        std::string message;
        if (app.DeleteProfileByIndex(static_cast<size_t>(_deleteProfileSelection), message))
        {
            _projectMWrapper.LoadLastPresetForActiveProfile();
            _notificationCenter.postNotification(new DisplayToastNotification(message));
            ImGui::CloseCurrentPopup();
        }
        else
        {
            _notificationCenter.postNotification(new DisplayToastNotification("Delete profile failed: " + message));
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

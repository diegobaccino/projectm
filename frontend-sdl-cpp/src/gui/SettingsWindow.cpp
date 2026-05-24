#include "SettingsWindow.h"

#include "AudioCapture.h"
#include "ProjectMSDLApplication.h"
#include "ProjectMWrapper.h"
#include "SDLRenderingWindow.h"

#include "ProjectMGUI.h"

#include "notifications/DisplayToastNotification.h"

#include <imgui.h>

#include <Poco/NotificationCenter.h>

#include <Poco/Util/Application.h>

#include <projectM-4/logo_overlay.h>

SettingsWindow::SettingsWindow(ProjectMGUI& gui)
    : _gui(gui)
    , _audioCapture(ProjectMSDLApplication::instance().getSubsystem<AudioCapture>())
    , _userConfiguration(ProjectMSDLApplication::instance().UserConfiguration())
    , _commandLineConfiguration(ProjectMSDLApplication::instance().CommandLineConfiguration())
{
    ReloadLogoStateFromConfig();
}

void SettingsWindow::Show()
{
    ReloadLogoStateFromConfig();
    _userScale = static_cast<float>(_userConfiguration->getDouble("window.uiScale", 1.0));
    _visible = true;
}

void SettingsWindow::ReloadLogoStateFromConfig()
{
    _logoPath = _userConfiguration->getString("logo.path", "");
    _logoLoaded = !_logoPath.empty();
    _logoEnabled = _userConfiguration->getBool("logo.enabled", true);
    _logoOpacity = static_cast<float>(_userConfiguration->getDouble("logo.opacity", 0.8));
    _logoScale = static_cast<float>(_userConfiguration->getDouble("logo.scale", 0.15));
    _logoRotation = static_cast<float>(_userConfiguration->getDouble("logo.rotation", 0.0));
    _logoAnchor = _userConfiguration->getInt("logo.anchor", 4);
    _logoOffsetX = static_cast<float>(_userConfiguration->getDouble("logo.offsetX", 0.0));
    _logoOffsetY = static_cast<float>(_userConfiguration->getDouble("logo.offsetY", 0.0));
    _logoReactiveEffect = _userConfiguration->getInt("logo.reactiveEffect", 0);
    _logoReactiveSensitivity = static_cast<float>(_userConfiguration->getDouble("logo.reactiveSensitivity", 0.5));
    _logoMotionEffect = _userConfiguration->getInt("logo.motionEffect", 0);
    _logoMotionSpeed = static_cast<float>(_userConfiguration->getDouble("logo.motionSpeed", 4.0));
    _logoRandomReactive = _userConfiguration->getBool("logo.randomReactive", false);
    _logoRandomMotion = _userConfiguration->getBool("logo.randomMotion", false);
    _logoRandomInterval = static_cast<float>(_userConfiguration->getDouble("logo.randomInterval", 15.0));
}

void SettingsWindow::Draw()
{
    if (!_visible)
    {
        return;
    }

    const bool wasVisible = _visible;

    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
    constexpr ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;

    std::string windowId = "Settings";
    if (_changed)
    {
        windowId.append(" [CHANGED - NOT SAVED]");
    }
    windowId.append("###Settings");

    ImGui::SetNextWindowSize(ImVec2(1050, 550), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(windowId.c_str(), &_visible, windowFlags))
    {
        if (ImGui::BeginTabBar("projectM Settings", tabBarFlags))
        {
            DrawProjectMSettingsTab();
            DrawWindowSettingsTab();
            DrawAudioSettingsTab();
            DrawLogoOverlayTab();
            DrawHelpTab();

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        SaveButton();
    }
    ImGui::End();

    if (_pathChooser.Draw())
    {
        auto& selectedDirectory = _pathChooser.SelectedFiles();
        if (!selectedDirectory.empty())
        {
            _userConfiguration->setString(_pathChooser.Context(),
                                          Poco::Path(selectedDirectory.at(0).path()).makeDirectory().toString());
            _changed = true;
        }
    }

    if (_logoFileChooser.Draw())
    {
        auto& selectedFiles = _logoFileChooser.SelectedFiles();
        if (!selectedFiles.empty())
        {
            _logoPath = selectedFiles.at(0).path();
            auto& projectMWrapper = Poco::Util::Application::instance().getSubsystem<ProjectMWrapper>();
            auto handle = projectMWrapper.ProjectM();
            if (projectm_logo_overlay_load_image(handle, _logoPath.c_str()))
            {
                _logoLoaded = true;
                _logoEnabled = true;
                projectm_logo_overlay_set_enabled(handle, true);
                projectm_logo_overlay_set_opacity(handle, _logoOpacity);
                projectm_logo_overlay_set_size(handle, _logoScale);
                projectm_logo_overlay_set_rotation(handle, _logoRotation);
                projectm_logo_overlay_set_position(handle, static_cast<projectm_logo_anchor>(_logoAnchor),
                                                   _logoOffsetX, _logoOffsetY);
                projectm_logo_overlay_set_reactive_effect(handle,
                    static_cast<projectm_logo_reactive_effect>(_logoReactiveEffect));
                projectm_logo_overlay_set_reactive_sensitivity(handle, _logoReactiveSensitivity);
                projectm_logo_overlay_set_motion_effect(handle,
                    static_cast<projectm_logo_motion_effect>(_logoMotionEffect));
                projectm_logo_overlay_set_motion_speed(handle, _logoMotionSpeed);
                projectm_logo_overlay_set_random_reactive(handle, _logoRandomReactive);
                projectm_logo_overlay_set_random_motion(handle, _logoRandomMotion);
                projectm_logo_overlay_set_random_interval(handle, _logoRandomInterval);
                _changed = true;
                Poco::NotificationCenter::defaultCenter().postNotification(
                    new DisplayToastNotification("Logo loaded: " + Poco::Path(_logoPath).getFileName()));
            }
            else
            {
                _logoLoaded = false;
                Poco::NotificationCenter::defaultCenter().postNotification(
                    new DisplayToastNotification("Failed to load logo image!"));
            }
        }
    }

    if (wasVisible && !_visible && _changed)
    {
        PersistSettings(false);
    }
}

void SettingsWindow::DrawProjectMSettingsTab()
{
    if (ImGui::BeginTabItem("projectM"))
    {
        if (ImGui::BeginTable("projectM", 5, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("##desc", ImGuiTableColumnFlags_WidthFixed, .0f);
            ImGui::TableSetupColumn("##setting", ImGuiTableColumnFlags_WidthStretch, .0f);
            ImGui::TableSetupColumn("##choose", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("##reset", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("##override", ImGuiTableColumnFlags_WidthFixed, .0f);

            ImGui::TableNextRow();
            LabelWithTooltip("Preset Path", "Path to search for preset files if no playlist is loaded.");
            PathSetting("projectM.presetPath");

            ImGui::TableNextRow();
            LabelWithTooltip("Texture Path", "Path to search for texture/image files requested by presets.");
            PathSetting("projectM.texturePath");

            ImGui::TableNextRow();
            LabelWithTooltip("Display projectM Logo Preset",
                             "If enabled, the projectM logo preset is shown on startup.");
            BooleanSetting("projectM.enableSplash", false);

            ImGui::TableNextRow();
            LabelWithTooltip("Lock Preset", "If enabled, presets will not be switched automatically.");
            BooleanSetting("projectM.presetLocked", false);

            ImGui::TableNextRow();
            LabelWithTooltip("Shuffle Presets", "Selects presets randomly from the current playlist.");
            BooleanSetting("projectM.shuffleEnabled", true);

            ImGui::TableNextRow();
            LabelWithTooltip("Skip To Dropped Presets",
                             "If enabled, will skip to the new presets when preset(s) are dropped onto the window and added to the playlist");
            BooleanSetting("projectM.skipToDropped", true);

            ImGui::TableNextRow();
            LabelWithTooltip("Dropped Folder Overrides Playlist",
                             "When dropping a folder, clear the playlist and add all presets from the folder.");
            BooleanSetting("projectM.droppedFolderOverride", false);

            ImGui::TableNextRow();
            LabelWithTooltip("Preset Display Duration",
                             "Time in seconds a preset will be displayed before it's switched.");
            DoubleSetting("projectM.displayDuration", 30.0, 1.0, 240.0);

            ImGui::TableNextRow();
            LabelWithTooltip("Preset Transition Duration",
                             "Time in seconds it takes to transition softly from one preset to another.");
            DoubleSetting("projectM.transitionDuration", 3.0, .0, 10.0);

            ImGui::TableNextRow();
            LabelWithTooltip("Enable Hard Cuts",
                             "Enables beat-driven, fast preset changes.\nSensitivity and earliest switch time can be configured separately.");
            BooleanSetting("projectM.hardCutsEnabled", false);

            ImGui::TableNextRow();
            LabelWithTooltip("  Hard Cut Duration",
                             "Time in seconds before a preset will be switched at\nthe earliest on hard cuts. If larger than display duration,\nhard cuts won't happen at all.");
            DoubleSetting("projectM.hardCutDuration", 20.0, 1.0, 240.0);

            ImGui::TableNextRow();
            LabelWithTooltip("  Hard Cut Threshold",
                             "Volume difference between measurements required to trigger a hard cut.\nHigher values mean fewer hard cuts.");
            DoubleSetting("projectM.hardCutSensitivity", 1.0, 0.0, 10.0);

            ImGui::TableNextRow();
            LabelWithTooltip("Aspect Correction",
                             "Enables aspect ration correction in presets.\nOnly affects presets using the aspect ratio variables.");
            BooleanSetting("projectM.aspectCorrectionEnabled", true);

            ImGui::TableNextRow();
            LabelWithTooltip("Per-Point Mesh Size X/Y",
                             "Size of the per-point transformation grid.\nHigher values produce better quality, but require exponentially more CPU time to calculate.\nMilkdrop's default is 48x32.");
            IntegerSettingVec("projectM.meshX", "projectM.meshY", 64, 48, 8, 300);

            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
}

void SettingsWindow::DrawWindowSettingsTab()
{
    if (ImGui::BeginTabItem("Window / Rendering"))
    {
        if (ImGui::BeginTable("projectM", 5, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("##desc", ImGuiTableColumnFlags_WidthFixed, .0f);
            ImGui::TableSetupColumn("##setting", ImGuiTableColumnFlags_WidthStretch, .0f);
            ImGui::TableSetupColumn("##choose", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("##reset", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("##override", ImGuiTableColumnFlags_WidthFixed, .0f);

            ImGui::TableNextRow();
            LabelWithTooltip("Startup Window Placement",
                             "Initial window placement options when starting projectM.\nClick the button to copy the current window size, position and display to the settings.");
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button("Use Current Size and Position"))
            {
                auto& renderingWindow = Poco::Util::Application::instance().getSubsystem<SDLRenderingWindow>();
                int x;
                int y;

                renderingWindow.GetWindowSize(x, y);
                _userConfiguration->setInt("window.width", x);
                _userConfiguration->setInt("window.height", y);

                renderingWindow.GetWindowPosition(x, y, true);
                _userConfiguration->setInt("window.left", x);
                _userConfiguration->setInt("window.top", y);

                _userConfiguration->setBool("window.overridePosition", true);
                _userConfiguration->setInt("window.monitor", renderingWindow.GetCurrentDisplay() + 1);
            }

            ImGui::TableNextRow();
            LabelWithTooltip("  Window Size",
                             "Initial window size when starting projectM.\nThis might or might not include the window decoration, depending on the OS.");
            IntegerSettingVec("window.width", "window.height", 1920, 1080, 32, 8192);

            ImGui::TableNextRow();
            LabelWithTooltip("  Window Position",
                             "Initial window position when starting projectM.\nThe coordinates are relative to the current monitor.");
            WindowPositionSetting();

            ImGui::TableNextRow();
            LabelWithTooltip("  Monitor",
                             "Use 0 to let the OS select the monitor, or any positive number to select a specific monitor.\nIf the number is larger than the number of connected monitors, the last available one will be used.");
            IntegerSetting("window.monitor", 0, 0, 10);

            ImGui::TableNextRow();
            LabelWithTooltip("Borderless Window",
                             "Don't display the window border and title bar if the OS supports it.\nCan make the window immovable.");
            BooleanSetting("window.borderless", false);

            ImGui::TableNextRow();
            LabelWithTooltip("Start Fullscreen", "Start projectM in fullscreen mode");
            BooleanSetting("window.fullscreen", false);

            ImGui::TableNextRow();
            LabelWithTooltip("  Exclusive Fullscreen Mode",
                             "Use exclusive mode if fullscreen, e.g. not as a borderless window.\nThis can improve performance, but may switch the desktop resolution!");
            BooleanSetting("window.fullscreen.exclusiveMode", false);

            ImGui::TableNextRow();
            LabelWithTooltip("  Exclusive Mode Resolution",
                             "Resolution to change to in exclusive fullscreen mode.\nNot all graphics driver support arbitrary resolution and will use the next-best supported one.");
            IntegerSettingVec("fullscreen.width", "fullscreen.height", 1920, 1080, 240, 8192);

            ImGui::TableNextRow();
            LabelWithTooltip("Target FPS",
                             "Limit frames rendered per second to the given FPS value.\nNOTE: A value of 0 will NOT limit FPS and render at either VSync or unlimited pace, possibly using all CPU/GPU resources.");
            IntegerSetting("projectM.fps", 60, 0, 300);

            ImGui::TableNextRow();
            LabelWithTooltip("Wait for Vertical Sync",
                             "Wait for vertical sync interval before displaying the next frame.\nThis will limit max FPS to the vertical sync frequency but prevents tearing.");
            BooleanSetting("window.waitForVerticalSync", false);

            ImGui::TableNextRow();
            LabelWithTooltip("  Use Adaptive Sync",
                             "Tries to use adaptive vertical sync if vertical sync is enabled.\nWhen using a monitor capable of adaptive sync, setting FPS to 0 gives the best results.");
            BooleanSetting("window.adaptiveVerticalSync", false);

            ImGui::TableNextRow();
            LabelWithTooltip("Preset Name in Title",
                             "Controls displaying the current preset name after the application name in the window title.");
            BooleanSetting("window.displayPresetNameInTitle", true);

            ImGui::TableNextRow();
            LabelWithTooltip("Display Toast Messages",
                             "Controls displaying toast messages/notifications, e.g. when changing the audio device.");
            BooleanSetting("projectM.displayToasts", true);

            ImGui::TableNextRow();
            LabelWithTooltip("UI Scaling Factor", "Multiplies the default UI/font size with the given factor.");
            DoubleSettingWithApply("window.uiScale", 1.0, 0.1, 3.0, _userScale);

            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
}

void SettingsWindow::DrawAudioSettingsTab()
{
    if (ImGui::BeginTabItem("Audio"))
    {
        if (ImGui::BeginTable("projectM", 5, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("##desc", ImGuiTableColumnFlags_WidthFixed, .0f);
            ImGui::TableSetupColumn("##setting", ImGuiTableColumnFlags_WidthStretch, .0f);
            ImGui::TableSetupColumn("##choose", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("##reset", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("##override", ImGuiTableColumnFlags_WidthFixed, .0f);

            ImGui::TableNextRow();
            LabelWithTooltip("Audio Capturing Device", "The device to capture audio from.");
            AudioDeviceSetting();

            ImGui::TableNextRow();
            LabelWithTooltip("Beat Sensitivity", "Beat detection multiplier.");
            DoubleSetting("projectM.beatSensitivity", 1.0, 0.0, 2.0);

            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }
}

void SettingsWindow::DrawLogoOverlayTab()
{
    if (ImGui::BeginTabItem("Logo Overlay"))
    {
        auto& projectMWrapper = Poco::Util::Application::instance().getSubsystem<ProjectMWrapper>();
        auto handle = projectMWrapper.ProjectM();

        ImGui::TextWrapped("Overlay a custom logo (PNG, JPG, GIF, BMP, TGA) on top of the visualization.");
        ImGui::Separator();

        // Logo file selection
        ImGui::TextUnformatted("Logo Image:");
        ImGui::SameLine();
        if (_logoPath.empty())
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(none loaded)");
        }
        else
        {
            ImGui::TextWrapped("%s", Poco::Path(_logoPath).getFileName().c_str());
        }

        if (ImGui::Button("Browse for Logo..."))
        {
            _logoFileChooser.AllowedExtensions({"png", "jpg", "jpeg", "gif", "bmp", "tga"});
            _logoFileChooser.Title("Select Logo Image");
            _logoFileChooser.Show();
        }
        ImGui::SameLine();
        if (_logoLoaded && ImGui::Button("Remove Logo"))
        {
            projectm_logo_overlay_clear(handle);
            _logoLoaded = false;
            _logoEnabled = false;
            _logoPath.clear();
            _changed = true;
            Poco::NotificationCenter::defaultCenter().postNotification(
                new DisplayToastNotification("Logo removed"));
        }

        ImGui::Separator();

        if (!_logoLoaded)
        {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Load a logo image to configure overlay settings.");
            ImGui::EndTabItem();
            return;
        }

        // Enabled toggle
        if (ImGui::Checkbox("Show Logo", &_logoEnabled))
        {
            projectm_logo_overlay_set_enabled(handle, _logoEnabled);
            _changed = true;
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Position:");

        // Anchor selection
        const char* anchorNames[] = {
            "Top Left", "Top Center", "Top Right",
            "Center Left", "Center", "Center Right",
            "Bottom Left", "Bottom Center", "Bottom Right"
        };
        if (ImGui::Combo("Anchor", &_logoAnchor, anchorNames, IM_ARRAYSIZE(anchorNames)))
        {
            projectm_logo_overlay_set_position(handle, static_cast<projectm_logo_anchor>(_logoAnchor),
                                               _logoOffsetX, _logoOffsetY);
            _changed = true;
        }

        // Offset
        bool offsetChanged = false;
        offsetChanged |= ImGui::SliderFloat("Offset X (px)", &_logoOffsetX, -500.0f, 500.0f);
        offsetChanged |= ImGui::SliderFloat("Offset Y (px)", &_logoOffsetY, -500.0f, 500.0f);
        if (offsetChanged)
        {
            projectm_logo_overlay_set_position(handle, static_cast<projectm_logo_anchor>(_logoAnchor),
                                               _logoOffsetX, _logoOffsetY);
            _changed = true;
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Appearance:");

        // Size
        if (ImGui::SliderFloat("Size", &_logoScale, 0.01f, 1.0f, "%.2f"))
        {
            projectm_logo_overlay_set_size(handle, _logoScale);
            _changed = true;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Logo size as a fraction of the shorter viewport dimension.");
            ImGui::EndTooltip();
        }

        // Opacity
        if (ImGui::SliderFloat("Opacity", &_logoOpacity, 0.0f, 1.0f, "%.2f"))
        {
            projectm_logo_overlay_set_opacity(handle, _logoOpacity);
            _changed = true;
        }

        // Base Rotation
        if (ImGui::SliderFloat("Base Rotation", &_logoRotation, -180.0f, 180.0f, "%.1f deg"))
        {
            projectm_logo_overlay_set_rotation(handle, _logoRotation);
            _changed = true;
        }

        // ---- Reactive Effects (music-driven) ----
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Reactive Effects (Music-Driven)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("These effects respond to the music in real-time.");
            ImGui::EndTooltip();
        }

        const char* reactiveNames[] = {"None", "Pulse", "Glow", "Color Shift"};
        if (ImGui::Combo("Reactive Effect", &_logoReactiveEffect, reactiveNames, IM_ARRAYSIZE(reactiveNames)))
        {
            projectm_logo_overlay_set_reactive_effect(handle,
                static_cast<projectm_logo_reactive_effect>(_logoReactiveEffect));
            _changed = true;
        }

        if (_logoReactiveEffect > 0)
        {
            if (ImGui::SliderFloat("Sensitivity", &_logoReactiveSensitivity, 0.0f, 1.0f, "%.2f"))
            {
                projectm_logo_overlay_set_reactive_sensitivity(handle, _logoReactiveSensitivity);
                _changed = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("How strongly the effect responds to audio. 0 = subtle, 1 = maximum.");
                ImGui::EndTooltip();
            }
        }

        if (ImGui::Checkbox("Random Reactive", &_logoRandomReactive))
        {
            projectm_logo_overlay_set_random_reactive(handle, _logoRandomReactive);
            _changed = true;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Automatically cycle through reactive effects at the random interval.");
            ImGui::EndTooltip();
        }

        // ---- Motion Effects (time-driven) ----
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Motion Effects (Time-Driven)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("These effects animate independently of the music.");
            ImGui::EndTooltip();
        }

        const char* motionNames[] = {"None", "Spin", "Bounce", "Sway"};
        if (ImGui::Combo("Motion Effect", &_logoMotionEffect, motionNames, IM_ARRAYSIZE(motionNames)))
        {
            projectm_logo_overlay_set_motion_effect(handle,
                static_cast<projectm_logo_motion_effect>(_logoMotionEffect));
            _changed = true;
        }

        if (_logoMotionEffect > 0)
        {
            if (ImGui::SliderFloat("Cycle Speed (sec)", &_logoMotionSpeed, 0.5f, 30.0f, "%.1f"))
            {
                projectm_logo_overlay_set_motion_speed(handle, _logoMotionSpeed);
                _changed = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Seconds for one complete cycle of the motion effect.");
                ImGui::EndTooltip();
            }
        }

        if (ImGui::Checkbox("Random Motion", &_logoRandomMotion))
        {
            projectm_logo_overlay_set_random_motion(handle, _logoRandomMotion);
            _changed = true;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Automatically cycle through motion effects at the random interval.");
            ImGui::EndTooltip();
        }

        // Random interval (shared between both random modes)
        if (_logoRandomReactive || _logoRandomMotion)
        {
            ImGui::Separator();
            if (ImGui::SliderFloat("Random Interval (sec)", &_logoRandomInterval, 2.0f, 120.0f, "%.0f"))
            {
                projectm_logo_overlay_set_random_interval(handle, _logoRandomInterval);
                _changed = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("How often (in seconds) to switch to a new random effect.");
                ImGui::EndTooltip();
            }
        }

        ImGui::EndTabItem();
    }
}

void SettingsWindow::DrawHelpTab() const
{
    if (ImGui::BeginTabItem("Help"))
    {
        Poco::Path userConfigurationDir = Poco::Path::configHome();
        userConfigurationDir.makeDirectory().append("projectM/");

        ImGui::TextUnformatted("General:");
        ImGui::Bullet();
        ImGui::TextWrapped("%s", "Hover over the setting name to see a short description.");
        ImGui::Bullet();
        ImGui::TextWrapped("%s", "Settings overridden by command-line parameters cannot be changed.");
        ImGui::Bullet();
        ImGui::TextWrapped(
            "All values changed/set in this window are stored in the configuration file, projectMSDL.properties, in the user configuration directory \"%s\"",
            userConfigurationDir.toString().c_str());

        ImGui::Separator();

        ImGui::TextUnformatted("Changing values:");
        ImGui::Bullet();
        ImGui::TextWrapped(
            "%s",
            "Hold down control/command key when clicking on sliders to enter a custom value. The manually entered value can be outside the slider's value range.");
        ImGui::Bullet();
        ImGui::TextWrapped(
            "%s",
            "Click on \"Reset\" to unset a value and use the default value from the application's factory configuration file.");
        ImGui::EndTabItem();
    }
}

void SettingsWindow::SaveButton()
{
    if (ImGui::Button("Save Settings"))
    {
        PersistSettings(true);
    }
}

bool SettingsWindow::PersistSettings(bool notifySuccess)
{
    try
    {
        _userConfiguration->setString("logo.path", _logoPath);
        _userConfiguration->setBool("logo.enabled", _logoEnabled);
        _userConfiguration->setDouble("logo.opacity", _logoOpacity);
        _userConfiguration->setDouble("logo.scale", _logoScale);
        _userConfiguration->setDouble("logo.rotation", _logoRotation);
        _userConfiguration->setInt("logo.anchor", _logoAnchor);
        _userConfiguration->setDouble("logo.offsetX", _logoOffsetX);
        _userConfiguration->setDouble("logo.offsetY", _logoOffsetY);
        _userConfiguration->setInt("logo.reactiveEffect", _logoReactiveEffect);
        _userConfiguration->setDouble("logo.reactiveSensitivity", _logoReactiveSensitivity);
        _userConfiguration->setInt("logo.motionEffect", _logoMotionEffect);
        _userConfiguration->setDouble("logo.motionSpeed", _logoMotionSpeed);
        _userConfiguration->setBool("logo.randomReactive", _logoRandomReactive);
        _userConfiguration->setBool("logo.randomMotion", _logoRandomMotion);
        _userConfiguration->setDouble("logo.randomInterval", _logoRandomInterval);

        auto configFile = _commandLineConfiguration->getString("app.UserConfigurationFile", "");
        if (configFile.empty())
        {
            Poco::NotificationCenter::defaultCenter().postNotification(
                new DisplayToastNotification("Error saving settings"));
            return false;
        }

        _userConfiguration->save(configFile);
        if (notifySuccess)
        {
            Poco::NotificationCenter::defaultCenter().postNotification(
                new DisplayToastNotification("Settings saved!"));
        }
        _changed = false;
        return true;
    }
    catch (...)
    {
        Poco::NotificationCenter::defaultCenter().postNotification(
            new DisplayToastNotification("Error saving settings"));
        return false;
    }
}

void SettingsWindow::LabelWithTooltip(const std::string& label, const std::string& tooltipText)
{
    ImGui::TableSetColumnIndex(0);

    ImGui::TextUnformatted(label.c_str());
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltipText.c_str());
        ImGui::EndTooltip();
    }
}

void SettingsWindow::PathSetting(const std::string& property)
{
    ImGui::TableSetColumnIndex(1);

    auto path = _userConfiguration->getString(property, "");
    char pathBuffer[2048]{};
    strncpy(pathBuffer, path.c_str(), std::min<size_t>(2047, path.size()));

    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText(std::string("##path_" + property).c_str(), &pathBuffer[0], IM_ARRAYSIZE(pathBuffer),
                         ImGuiInputTextFlags_EnterReturnsTrue))
    {
        _userConfiguration->setString(property, std::string(pathBuffer));
        _changed = true;
    }

    ImGui::TableSetColumnIndex(2);

    ImGui::PushID(std::string(property + "_ChoosePathButton").c_str());
    if (ImGui::Button("..."))
    {
        _pathChooser.CurrentDirectory(path);
        _pathChooser.Title("Select directory");
        _pathChooser.Context(property);
        _pathChooser.Show();
    }
    ImGui::PopID();

    ResetButton(property);

    if (_commandLineConfiguration->has(property))
    {
        OverriddenSettingMarker();
    }
}


void SettingsWindow::BooleanSetting(const std::string& property, bool defaultValue)
{
    ImGui::TableSetColumnIndex(1);

    auto value = _userConfiguration->getBool(property, defaultValue);

    if (ImGui::Checkbox(std::string("##boolean_" + property).c_str(), &value))
    {
        _userConfiguration->setBool(property, value);
        _changed = true;
    }

    ResetButton(property);

    if (_commandLineConfiguration->has(property))
    {
        OverriddenSettingMarker();
    }
}

void SettingsWindow::IntegerSetting(const std::string& property, int defaultValue, int min, int max)
{
    ImGui::TableSetColumnIndex(1);

    auto value = _userConfiguration->getInt(property, defaultValue);

    if (ImGui::SliderInt(std::string("##integer_" + property).c_str(), &value, min, max))
    {
        _userConfiguration->setInt(property, value);
        _changed = true;
    }

    ResetButton(property);

    if (_commandLineConfiguration->has(property))
    {
        OverriddenSettingMarker();
    }
}

void SettingsWindow::IntegerSettingVec(const std::string& property1, const std::string& property2, int defaultValue1,
                                       int defaultValue2, int min, int max)
{
    ImGui::TableSetColumnIndex(1);

    int values[2] = {
        _userConfiguration->getInt(property1, defaultValue1),
        _userConfiguration->getInt(property2, defaultValue2)};

    if (ImGui::SliderInt2(std::string("##integer_" + property1 + property2).c_str(), values, min, max))
    {
        _userConfiguration->setInt(property1, values[0]);
        _userConfiguration->setInt(property2, values[1]);
        _changed = true;
    }

    ResetButton(property1, property2);

    if (_commandLineConfiguration->has(property1) || _commandLineConfiguration->has(property2))
    {
        OverriddenSettingMarker();
    }
}

void SettingsWindow::DoubleSetting(const std::string& property, double defaultValue, double min, double max)
{
    ImGui::TableSetColumnIndex(1);

    auto value = static_cast<float>(_userConfiguration->getDouble(property, defaultValue));

    if (ImGui::SliderFloat(std::string("##double_" + property).c_str(), &value, static_cast<float>(min),
                           static_cast<float>(max)))
    {
        _userConfiguration->setDouble(property, value);
        _changed = true;
    }

    ResetButton(property);

    if (_commandLineConfiguration->has(property))
    {
        OverriddenSettingMarker();
    }
}

void SettingsWindow::DoubleSettingWithApply(const std::string& property, double defaultValue, double min, double max,
                                            float& tempValue)
{
    ImGui::TableSetColumnIndex(1);

    ImGui::SliderFloat(std::string("##double_" + property).c_str(), &tempValue, static_cast<float>(min),
                       static_cast<float>(max));

    ImGui::SameLine();

    ImGui::PushID(std::string(property + "_ApplyButton").c_str());
    if (ImGui::Button("Apply"))
    {
        _userConfiguration->setDouble(property, tempValue);
        _changed = true;
    }
    ImGui::PopID();

    if (ResetButton(property))
    {
        tempValue = static_cast<float>(_userConfiguration->getDouble(property, defaultValue));
    }

    if (_commandLineConfiguration->has(property))
    {
        OverriddenSettingMarker();
    }
}

void SettingsWindow::WindowPositionSetting()
{
    ImGui::TableSetColumnIndex(1);

    bool positionIsOverridden = _userConfiguration->getBool("window.overridePosition", false);

    if (ImGui::Checkbox("##window_set_pos", &positionIsOverridden))
    {
        _userConfiguration->setBool("window.overridePosition", positionIsOverridden);
    }

    if (positionIsOverridden)
    {
        ImGui::SameLine();

        int values[2] = {
            _userConfiguration->getInt("window.left", 0),
            _userConfiguration->getInt("window.top", 0)};

        if (ImGui::SliderInt2("##window_pos", values, -8192, 8192))
        {
            _userConfiguration->setInt("window.left", values[0]);
            _userConfiguration->setInt("window.top", values[1]);
            _changed = true;
        }
    }

    ResetButton("window.overridePosition");

    if (_commandLineConfiguration->has("window.left") || _commandLineConfiguration->has("window.top"))
    {
        OverriddenSettingMarker();
    }
}

void SettingsWindow::AudioDeviceSetting()
{
    ImGui::TableSetColumnIndex(1);

    auto devices = _audioCapture.AudioDeviceList();
    auto currentIndex = _audioCapture.AudioDeviceIndex();

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##audiodevice", devices.at(currentIndex).c_str(), 0))
    {
        for (const auto& device : devices)
        {
            bool isSelected = device.first == currentIndex;

            if (ImGui::Selectable(device.second.c_str(), "", isSelected))
            {
                _audioCapture.AudioDeviceIndex(device.first);
                if (device.first == -1)
                {
                    _userConfiguration->setInt("audio.device", -1);
                }
                else
                {
                    _userConfiguration->setString("audio.device", device.second);
                }
                _changed = true;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ResetButton("audio.device");

    if (_commandLineConfiguration->has("audio.device"))
    {
        OverriddenSettingMarker();
    }
}

bool SettingsWindow::ResetButton(const std::string& property1, const std::string& property2)
{
    if (!_userConfiguration->has(property1) && (property2.empty() || !_userConfiguration->has(property2)))
    {
        return false;
    }

    ImGui::TableSetColumnIndex(3);

    bool pressed{false};

    ImGui::PushID(std::string(property1 + property2 + "_ResetButton").c_str());
    if (ImGui::Button("Reset"))
    {
        _userConfiguration->remove(property1);
        if (!property2.empty())
        {
            _userConfiguration->remove(property2);
        }
        _changed = true;
        pressed = true;
    }
    ImGui::PopID();

    return pressed;
}

void SettingsWindow::OverriddenSettingMarker()
{
    ImGui::TableSetColumnIndex(4);

    ImGui::TextColored(ImVec4(1, 0, 0, 1), "[!]");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Value passed as command line argument.");
        ImGui::TextUnformatted("The value configured here will only be used if NOT overridden via the command line.");
        ImGui::EndTooltip();
    }
}

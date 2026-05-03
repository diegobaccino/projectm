/**
 * @file LogoOverlay.hpp
 * @brief Renders a user-provided logo image as a persistent overlay on top of the visualization.
 * @since 4.2.0
 *
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2025 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 */
#pragma once

#include "Renderer/Mesh.hpp"
#include "Renderer/Sampler.hpp"
#include "Renderer/ShaderCache.hpp"
#include "Renderer/Texture.hpp"

#include <Audio/FrameAudioData.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace libprojectM {
namespace Renderer {

/**
 * @brief Anchor positions for logo placement on the viewport.
 */
enum class LogoAnchor : uint8_t
{
    TopLeft = 0,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

/**
 * @brief Music-reactive effects that respond to audio input.
 */
enum class LogoReactiveEffect : uint8_t
{
    None = 0, //!< No reactive effect.
    Pulse,    //!< Scale pulses with bass.
    Glow,     //!< Opacity oscillates with mid frequencies.
    ColorShift //!< Hue rotates with treble energy.
};

/**
 * @brief Time-driven motion effects independent of audio.
 */
enum class LogoMotionEffect : uint8_t
{
    None = 0, //!< No motion effect.
    Spin,     //!< Continuous rotation.
    Bounce,   //!< Gentle vertical floating.
    Sway      //!< Gentle horizontal pendulum.
};

/**
 * @brief Renders a user-provided logo image as a persistent overlay on top of the visualization.
 *
 * The logo is always rendered as the last element in the pipeline, on top of both
 * the preset visualization and any user sprites. It supports configurable positioning,
 * sizing, opacity, rotation, and optional effects in two categories:
 *
 * - **Reactive Effects**: Music-driven (pulse, glow, color shift) with a sensitivity parameter.
 * - **Motion Effects**: Time-driven (spin, bounce, sway) with a speed parameter (seconds per cycle).
 *
 * @note All methods that modify state must be called from the same thread that holds the
 *       active OpenGL context used by projectM. Image decoding (CPU) happens immediately,
 *       but GPU texture upload is deferred to the next Draw() call.
 *
 * @note GIF images are loaded as static images (first frame only). Animated GIF playback
 *       is not supported.
 */
class LogoOverlay
{
public:
    LogoOverlay();
    ~LogoOverlay();

    LogoOverlay(const LogoOverlay&) = delete;
    auto operator=(const LogoOverlay&) -> LogoOverlay& = delete;

    /**
     * @brief Loads a logo image from a file path.
     * @param filePath Path to the image file.
     * @return true if the image was decoded successfully, false on error.
     */
    bool LoadImage(const std::string& filePath);

    /**
     * @brief Loads a logo image from a memory buffer.
     * @param data Pointer to the image data in memory.
     * @param dataSize Size of the image data in bytes.
     * @return true if the image was decoded successfully, false on error.
     */
    bool LoadImageData(const unsigned char* data, size_t dataSize);

    /**
     * @brief Removes the current logo and frees associated resources.
     */
    void Clear();

    void SetEnabled(bool enabled);
    auto IsEnabled() const -> bool;

    void SetPosition(LogoAnchor anchor, float offsetX, float offsetY);
    void SetSize(float scale);
    void SetOpacity(float opacity);
    void SetRotation(float degrees);

    // --- Reactive Effects (music-driven) ---

    /**
     * @brief Sets the active reactive effect.
     * @param effect The reactive effect to use.
     */
    void SetReactiveEffect(LogoReactiveEffect effect);
    auto GetReactiveEffect() const -> LogoReactiveEffect;

    /**
     * @brief Sets the sensitivity of the active reactive effect.
     * @param sensitivity Value in [0.0, 1.0]. Higher = stronger response to audio.
     */
    void SetReactiveSensitivity(float sensitivity);

    // --- Motion Effects (time-driven) ---

    /**
     * @brief Sets the active motion effect.
     * @param effect The motion effect to use.
     */
    void SetMotionEffect(LogoMotionEffect effect);
    auto GetMotionEffect() const -> LogoMotionEffect;

    /**
     * @brief Sets the speed of the active motion effect.
     * @param secondsPerCycle Seconds for one full cycle (e.g., one full spin). Clamped to [0.5, 30.0].
     */
    void SetMotionSpeed(float secondsPerCycle);

    // --- Random mode ---

    /**
     * @brief Enables random cycling through reactive effects.
     * @param enabled true to randomly switch reactive effects.
     */
    void SetRandomReactive(bool enabled);

    /**
     * @brief Enables random cycling through motion effects.
     * @param enabled true to randomly switch motion effects.
     */
    void SetRandomMotion(bool enabled);

    /**
     * @brief Sets how often random mode switches effects.
     * @param seconds Interval in seconds between random switches.
     */
    void SetRandomInterval(float seconds);

    // Legacy API compatibility
    void SetBeatReactivity(float intensity);

    /**
     * @brief Draws the logo overlay.
     * @param audioData Current frame audio data.
     * @param shaderCache The global shader cache instance.
     * @param viewportWidth Current viewport width in pixels.
     * @param viewportHeight Current viewport height in pixels.
     * @param deltaTime Seconds since last frame (from TimeKeeper).
     */
    void Draw(const Audio::FrameAudioData& audioData,
              ShaderCache& shaderCache,
              int viewportWidth, int viewportHeight,
              float deltaTime);

private:
    bool UploadPendingTexture();
    void ComputeTransform(float effectiveScale, float extraRotation,
                          float offsetXAdjust, float offsetYAdjust,
                          int viewportWidth, int viewportHeight,
                          float* outVertices) const;

    // Configuration
    bool m_enabled{false};
    LogoAnchor m_anchor{LogoAnchor::BottomRight};
    float m_offsetX{-20.0f};
    float m_offsetY{-20.0f};
    float m_scale{0.15f};
    float m_opacity{0.8f};
    float m_rotationDegrees{0.0f};

    // Reactive effects
    LogoReactiveEffect m_reactiveEffect{LogoReactiveEffect::None};
    float m_reactiveSensitivity{0.5f};

    // Motion effects
    LogoMotionEffect m_motionEffect{LogoMotionEffect::None};
    float m_motionSecondsPerCycle{4.0f};

    // Random mode
    bool m_randomReactive{false};
    bool m_randomMotion{false};
    float m_randomInterval{10.0f};
    float m_randomReactiveTimer{0.0f};
    float m_randomMotionTimer{0.0f};

    // Animation accumulator
    float m_animTime{0.0f};

    // GPU resources
    std::shared_ptr<Texture> m_texture;
    Mesh m_mesh{VertexBufferUsage::DynamicDraw, false, true};
    Sampler m_sampler{GL_CLAMP_TO_EDGE, GL_LINEAR};
    std::weak_ptr<Shader> m_shader;

    // Pending image data
    std::vector<unsigned char> m_pendingImageData;
    int m_pendingWidth{0};
    int m_pendingHeight{0};
    bool m_hasPendingUpload{false};

    // Image dimensions (after upload)
    int m_imageWidth{0};
    int m_imageHeight{0};
};

} // namespace Renderer
} // namespace libprojectM

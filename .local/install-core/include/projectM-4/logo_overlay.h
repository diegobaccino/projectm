/**
 * @file logo_overlay.h
 * @copyright 2003-2025 projectM Team
 * @brief API for managing a persistent logo overlay on the visualization.
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
 *
 */

#pragma once

#include "projectM-4/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Anchor positions for logo placement on the viewport.
 * @since 4.2.0
 */
typedef enum
{
    PROJECTM_LOGO_ANCHOR_TOP_LEFT = 0,
    PROJECTM_LOGO_ANCHOR_TOP_CENTER = 1,
    PROJECTM_LOGO_ANCHOR_TOP_RIGHT = 2,
    PROJECTM_LOGO_ANCHOR_CENTER_LEFT = 3,
    PROJECTM_LOGO_ANCHOR_CENTER = 4,
    PROJECTM_LOGO_ANCHOR_CENTER_RIGHT = 5,
    PROJECTM_LOGO_ANCHOR_BOTTOM_LEFT = 6,
    PROJECTM_LOGO_ANCHOR_BOTTOM_CENTER = 7,
    PROJECTM_LOGO_ANCHOR_BOTTOM_RIGHT = 8
} projectm_logo_anchor;

/**
 * @brief Music-reactive logo effects.
 * @since 4.2.0
 */
typedef enum
{
    PROJECTM_LOGO_REACTIVE_NONE = 0,       //!< No reactive effect.
    PROJECTM_LOGO_REACTIVE_PULSE = 1,      //!< Scale pulses with bass.
    PROJECTM_LOGO_REACTIVE_GLOW = 2,       //!< Opacity oscillates with mid frequencies.
    PROJECTM_LOGO_REACTIVE_COLOR_SHIFT = 3 //!< Hue rotates with treble energy.
} projectm_logo_reactive_effect;

/**
 * @brief Time-driven logo motion effects.
 * @since 4.2.0
 */
typedef enum
{
    PROJECTM_LOGO_MOTION_NONE = 0,   //!< No motion effect.
    PROJECTM_LOGO_MOTION_SPIN = 1,   //!< Continuous rotation.
    PROJECTM_LOGO_MOTION_BOUNCE = 2, //!< Gentle vertical floating.
    PROJECTM_LOGO_MOTION_SWAY = 3    //!< Gentle horizontal pendulum.
} projectm_logo_motion_effect;

/**
 * @brief Loads a logo image from a file path for display as a visualization overlay.
 *
 * Supported formats: PNG, GIF (static first frame only), BMP, JPG, TGA.
 *
 * The image is decoded immediately but GPU texture upload is deferred to the next rendered frame.
 * This function must be called from the same thread that owns the projectM OpenGL context.
 *
 * @important Loading a new image replaces any previously loaded logo.
 * @param instance The projectM instance handle.
 * @param image_path Absolute or relative path to the image file.
 * @return true if the image was decoded successfully, false on error (file not found, unsupported format, etc.).
 * @since 4.2.0
 */
PROJECTM_EXPORT bool projectm_logo_overlay_load_image(projectm_handle instance,
                                                       const char* image_path);

/**
 * @brief Loads a logo image from a memory buffer for display as a visualization overlay.
 *
 * Supported formats: PNG, GIF (static first frame only), BMP, JPG, TGA.
 *
 * The image is decoded immediately but GPU texture upload is deferred to the next rendered frame.
 * This function must be called from the same thread that owns the projectM OpenGL context.
 *
 * @important Loading a new image replaces any previously loaded logo.
 * @param instance The projectM instance handle.
 * @param data Pointer to the image data in memory.
 * @param data_size Size of the image data in bytes.
 * @return true if the image was decoded successfully, false on error.
 * @since 4.2.0
 */
PROJECTM_EXPORT bool projectm_logo_overlay_load_image_data(projectm_handle instance,
                                                            const unsigned char* data,
                                                            size_t data_size);

/**
 * @brief Enables or disables the logo overlay.
 *
 * When disabled, the logo is not rendered but remains loaded. Re-enabling will show the
 * previously loaded logo without reloading it.
 *
 * @param instance The projectM instance handle.
 * @param enabled true to show the logo, false to hide it.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_enabled(projectm_handle instance,
                                                        bool enabled);

/**
 * @brief Returns whether the logo overlay is currently enabled.
 * @param instance The projectM instance handle.
 * @return true if the logo overlay is enabled, false if disabled.
 * @since 4.2.0
 */
PROJECTM_EXPORT bool projectm_logo_overlay_is_enabled(projectm_handle instance);

/**
 * @brief Sets the logo position using an anchor point and pixel offsets.
 *
 * The anchor determines which part of the viewport the logo is placed relative to.
 * Offsets are in pixels from that anchor. For corner anchors, the logo is inset from
 * the edge; for center anchors, it is centered on the axis.
 *
 * Default: Bottom-right corner with (-20, -20) offset (inset 20px from corner).
 *
 * @param instance The projectM instance handle.
 * @param anchor The anchor position on the viewport.
 * @param offset_x Horizontal offset in pixels. Positive = rightward.
 * @param offset_y Vertical offset in pixels. Positive = downward.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_position(projectm_handle instance,
                                                         projectm_logo_anchor anchor,
                                                         float offset_x,
                                                         float offset_y);

/**
 * @brief Sets the logo size as a fraction of the viewport's shorter dimension.
 *
 * A scale of 0.1 means the logo width will be 10% of min(viewport_width, viewport_height).
 * The logo's aspect ratio is always preserved.
 *
 * Default: 0.15 (15% of shorter viewport dimension).
 *
 * @param instance The projectM instance handle.
 * @param scale Size factor, clamped to [0.01, 1.0].
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_size(projectm_handle instance,
                                                     float scale);

/**
 * @brief Sets the logo opacity (transparency).
 *
 * Default: 0.8 (80% opaque).
 *
 * @param instance The projectM instance handle.
 * @param opacity Opacity value, clamped to [0.0, 1.0]. 0.0 = fully transparent, 1.0 = fully opaque.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_opacity(projectm_handle instance,
                                                        float opacity);

/**
 * @brief Sets the logo rotation angle.
 *
 * Default: 0.0 (no rotation).
 *
 * @param instance The projectM instance handle.
 * @param degrees Counter-clockwise rotation in degrees.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_rotation(projectm_handle instance,
                                                         float degrees);

/**
 * @brief Sets the intensity of beat-reactive logo pulsing.
 *
 * When non-zero, the logo scale gently pulses in response to the bass beat,
 * creating a dynamic, music-synchronized effect.
 *
 * Default: 0.0 (no pulsing).
 *
 * @param instance The projectM instance handle.
 * @param intensity Pulse intensity, clamped to [0.0, 1.0]. 0.0 = disabled.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_beat_reactivity(projectm_handle instance,
                                                                float intensity);

/**
 * @brief Removes the current logo and frees associated resources.
 *
 * After calling this function, no logo will be rendered until a new image is loaded.
 * The overlay enabled state is preserved.
 *
 * @param instance The projectM instance handle.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_clear(projectm_handle instance);

/**
 * @brief Sets the active reactive effect (music-driven).
 * @param instance The projectM instance handle.
 * @param effect The reactive effect to use.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_reactive_effect(projectm_handle instance,
                                                                projectm_logo_reactive_effect effect);

/**
 * @brief Sets the sensitivity of the active reactive effect.
 * @param instance The projectM instance handle.
 * @param sensitivity Value in [0.0, 1.0]. Higher = stronger response.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_reactive_sensitivity(projectm_handle instance,
                                                                     float sensitivity);

/**
 * @brief Sets the active motion effect (time-driven).
 * @param instance The projectM instance handle.
 * @param effect The motion effect to use.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_motion_effect(projectm_handle instance,
                                                              projectm_logo_motion_effect effect);

/**
 * @brief Sets the speed of the active motion effect.
 * @param instance The projectM instance handle.
 * @param seconds_per_cycle Seconds for one full cycle. Clamped to [0.5, 30.0].
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_motion_speed(projectm_handle instance,
                                                             float seconds_per_cycle);

/**
 * @brief Enables random cycling of reactive effects.
 * @param instance The projectM instance handle.
 * @param enabled true to randomly switch reactive effects at the configured interval.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_random_reactive(projectm_handle instance, bool enabled);

/**
 * @brief Enables random cycling of motion effects.
 * @param instance The projectM instance handle.
 * @param enabled true to randomly switch motion effects at the configured interval.
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_random_motion(projectm_handle instance, bool enabled);

/**
 * @brief Sets how often random mode switches effects.
 * @param instance The projectM instance handle.
 * @param seconds Interval in seconds between random switches. Clamped to [2.0, 120.0].
 * @since 4.2.0
 */
PROJECTM_EXPORT void projectm_logo_overlay_set_random_interval(projectm_handle instance, float seconds);

#ifdef __cplusplus
} // extern "C"
#endif

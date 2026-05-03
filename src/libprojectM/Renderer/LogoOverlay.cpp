/**
 * @file LogoOverlay.cpp
 * @brief Implementation of the logo overlay renderer.
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

#include "Renderer/LogoOverlay.hpp"

#include "Renderer/BlendMode.hpp"

#include <Logging.hpp>

#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace libprojectM {
namespace Renderer {

#ifdef USE_GLES
static constexpr char LogoShaderVersion[] = "#version 300 es\n\nprecision mediump float;\n";
#else
static constexpr char LogoShaderVersion[] = "#version 330\n\n";
#endif

static constexpr char LogoVertexShader[] = R"(
layout(location = 0) in vec2 position;
layout(location = 2) in vec2 tex_coord;

out vec2 frag_tex_coord;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    frag_tex_coord = tex_coord;
}
)";

static constexpr char LogoFragmentShader[] = R"(
in vec2 frag_tex_coord;

uniform sampler2D logo_texture;
uniform float logo_opacity;
uniform float logo_hue_shift;

out vec4 color;

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec4 texel = texture(logo_texture, frag_tex_coord);
    vec3 rgb = texel.rgb;
    if (logo_hue_shift != 0.0) {
        vec3 hsv = rgb2hsv(rgb);
        hsv.x = fract(hsv.x + logo_hue_shift);
        rgb = hsv2rgb(hsv);
    }
    color = vec4(rgb, texel.a * logo_opacity);
}
)";

static constexpr int MaxReasonableTextureSize = 8192;
static constexpr float Pi = 3.14159265358979f;
static constexpr float TwoPi = 6.28318530717959f;
static constexpr float MaxDeltaTime = 0.25f; // Cap dt to prevent jumps

LogoOverlay::LogoOverlay()
{
    m_mesh.SetRenderPrimitiveType(Mesh::PrimitiveType::TriangleStrip);
    m_mesh.SetVertexCount(4);
    m_mesh.Indices().Set({0, 1, 2, 3});

    m_mesh.UVs().Set({{0.0f, 0.0f},
                      {1.0f, 0.0f},
                      {0.0f, 1.0f},
                      {1.0f, 1.0f}});
}

LogoOverlay::~LogoOverlay() = default;

bool LogoOverlay::LoadImage(const std::string& filePath)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
    if (!pixels)
    {
        LOG_ERROR("LogoOverlay: Failed to load image from \"" + filePath + "\": " + stbi_failure_reason());
        return false;
    }

    if (width <= 0 || height <= 0 || width > MaxReasonableTextureSize || height > MaxReasonableTextureSize)
    {
        LOG_ERROR("LogoOverlay: Invalid image dimensions " + std::to_string(width) + "x" + std::to_string(height));
        stbi_image_free(pixels);
        return false;
    }

    size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    m_pendingImageData.assign(pixels, pixels + dataSize);
    m_pendingWidth = width;
    m_pendingHeight = height;
    m_hasPendingUpload = true;

    stbi_image_free(pixels);

    LOG_DEBUG("LogoOverlay: Decoded image " + std::to_string(width) + "x" + std::to_string(height) + " from \"" + filePath + "\"");
    return true;
}

bool LogoOverlay::LoadImageData(const unsigned char* data, size_t dataSize)
{
    if (!data || dataSize == 0)
    {
        LOG_ERROR("LogoOverlay: Null or empty image data");
        return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* pixels = stbi_load_from_memory(data, static_cast<int>(dataSize), &width, &height, &channels, 4);
    if (!pixels)
    {
        LOG_ERROR("LogoOverlay: Failed to decode image data: " + std::string(stbi_failure_reason()));
        return false;
    }

    if (width <= 0 || height <= 0 || width > MaxReasonableTextureSize || height > MaxReasonableTextureSize)
    {
        LOG_ERROR("LogoOverlay: Invalid image dimensions " + std::to_string(width) + "x" + std::to_string(height));
        stbi_image_free(pixels);
        return false;
    }

    size_t pixelDataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    m_pendingImageData.assign(pixels, pixels + pixelDataSize);
    m_pendingWidth = width;
    m_pendingHeight = height;
    m_hasPendingUpload = true;

    stbi_image_free(pixels);

    LOG_DEBUG("LogoOverlay: Decoded image " + std::to_string(width) + "x" + std::to_string(height) + " from memory");
    return true;
}

void LogoOverlay::Clear()
{
    m_texture.reset();
    m_pendingImageData.clear();
    m_pendingWidth = 0;
    m_pendingHeight = 0;
    m_hasPendingUpload = false;
    m_imageWidth = 0;
    m_imageHeight = 0;
}

void LogoOverlay::SetEnabled(bool enabled) { m_enabled = enabled; }
auto LogoOverlay::IsEnabled() const -> bool { return m_enabled; }

void LogoOverlay::SetPosition(LogoAnchor anchor, float offsetX, float offsetY)
{
    m_anchor = anchor;
    m_offsetX = offsetX;
    m_offsetY = offsetY;
}

void LogoOverlay::SetSize(float scale) { m_scale = std::max(0.01f, std::min(1.0f, scale)); }
void LogoOverlay::SetOpacity(float opacity) { m_opacity = std::max(0.0f, std::min(1.0f, opacity)); }
void LogoOverlay::SetRotation(float degrees) { m_rotationDegrees = degrees; }

void LogoOverlay::SetReactiveEffect(LogoReactiveEffect effect) { m_reactiveEffect = effect; }
auto LogoOverlay::GetReactiveEffect() const -> LogoReactiveEffect { return m_reactiveEffect; }
void LogoOverlay::SetReactiveSensitivity(float sensitivity) { m_reactiveSensitivity = std::max(0.0f, std::min(1.0f, sensitivity)); }

void LogoOverlay::SetMotionEffect(LogoMotionEffect effect)
{
    if (effect != m_motionEffect)
    {
        m_motionEffect = effect;
        m_animTime = 0.0f; // Reset animation phase on effect change
    }
}
auto LogoOverlay::GetMotionEffect() const -> LogoMotionEffect { return m_motionEffect; }
void LogoOverlay::SetMotionSpeed(float secondsPerCycle) { m_motionSecondsPerCycle = std::max(0.5f, std::min(30.0f, secondsPerCycle)); }

void LogoOverlay::SetRandomReactive(bool enabled) { m_randomReactive = enabled; }
void LogoOverlay::SetRandomMotion(bool enabled) { m_randomMotion = enabled; }
void LogoOverlay::SetRandomInterval(float seconds) { m_randomInterval = std::max(2.0f, std::min(120.0f, seconds)); }

// Legacy compatibility: maps to Pulse reactive effect
void LogoOverlay::SetBeatReactivity(float intensity)
{
    if (intensity > 0.0f)
    {
        m_reactiveEffect = LogoReactiveEffect::Pulse;
        m_reactiveSensitivity = intensity;
    }
    else
    {
        m_reactiveEffect = LogoReactiveEffect::None;
    }
}

bool LogoOverlay::UploadPendingTexture()
{
    if (!m_hasPendingUpload || m_pendingImageData.empty())
    {
        return false;
    }

    GLint maxTexSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    if (m_pendingWidth > maxTexSize || m_pendingHeight > maxTexSize)
    {
        LOG_ERROR("LogoOverlay: Image " + std::to_string(m_pendingWidth) + "x" + std::to_string(m_pendingHeight) +
                  " exceeds GL_MAX_TEXTURE_SIZE (" + std::to_string(maxTexSize) + ")");
        m_pendingImageData.clear();
        m_hasPendingUpload = false;
        return false;
    }

    m_texture = std::make_shared<Texture>(
        "logo_overlay",
        m_pendingImageData.data(),
        GL_TEXTURE_2D,
        m_pendingWidth, m_pendingHeight, 0,
        GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
        true);

    m_imageWidth = m_pendingWidth;
    m_imageHeight = m_pendingHeight;

    m_pendingImageData.clear();
    m_pendingImageData.shrink_to_fit();
    m_hasPendingUpload = false;

    LOG_DEBUG("LogoOverlay: Uploaded texture " + std::to_string(m_imageWidth) + "x" + std::to_string(m_imageHeight));
    return true;
}

void LogoOverlay::ComputeTransform(float effectiveScale, float effectiveRotation,
                                   float offsetXAdjust, float offsetYAdjust,
                                   int viewportWidth, int viewportHeight,
                                   float* outVertices) const
{
    if (viewportWidth == 0 || viewportHeight == 0 || m_imageWidth == 0 || m_imageHeight == 0)
    {
        return;
    }

    float shorterDim = static_cast<float>(std::min(viewportWidth, viewportHeight));
    float logoWidthPx = shorterDim * effectiveScale;
    float logoHeightPx = logoWidthPx * (static_cast<float>(m_imageHeight) / static_cast<float>(m_imageWidth));

    float anchorX = 0.0f;
    float anchorY = 0.0f;

    switch (m_anchor)
    {
        case LogoAnchor::TopLeft: case LogoAnchor::CenterLeft: case LogoAnchor::BottomLeft:
            anchorX = 0.0f; break;
        case LogoAnchor::TopCenter: case LogoAnchor::Center: case LogoAnchor::BottomCenter:
            anchorX = static_cast<float>(viewportWidth) * 0.5f; break;
        case LogoAnchor::TopRight: case LogoAnchor::CenterRight: case LogoAnchor::BottomRight:
            anchorX = static_cast<float>(viewportWidth); break;
    }

    switch (m_anchor)
    {
        case LogoAnchor::TopLeft: case LogoAnchor::TopCenter: case LogoAnchor::TopRight:
            anchorY = 0.0f; break;
        case LogoAnchor::CenterLeft: case LogoAnchor::Center: case LogoAnchor::CenterRight:
            anchorY = static_cast<float>(viewportHeight) * 0.5f; break;
        case LogoAnchor::BottomLeft: case LogoAnchor::BottomCenter: case LogoAnchor::BottomRight:
            anchorY = static_cast<float>(viewportHeight); break;
    }

    float centerX = anchorX + m_offsetX + offsetXAdjust;
    float centerY = anchorY + m_offsetY + offsetYAdjust;

    switch (m_anchor)
    {
        case LogoAnchor::TopLeft:     centerX += logoWidthPx * 0.5f; centerY += logoHeightPx * 0.5f; break;
        case LogoAnchor::TopCenter:   centerY += logoHeightPx * 0.5f; break;
        case LogoAnchor::TopRight:    centerX -= logoWidthPx * 0.5f; centerY += logoHeightPx * 0.5f; break;
        case LogoAnchor::CenterLeft:  centerX += logoWidthPx * 0.5f; break;
        case LogoAnchor::Center:      break;
        case LogoAnchor::CenterRight: centerX -= logoWidthPx * 0.5f; break;
        case LogoAnchor::BottomLeft:  centerX += logoWidthPx * 0.5f; centerY -= logoHeightPx * 0.5f; break;
        case LogoAnchor::BottomCenter: centerY -= logoHeightPx * 0.5f; break;
        case LogoAnchor::BottomRight: centerX -= logoWidthPx * 0.5f; centerY -= logoHeightPx * 0.5f; break;
    }

    float halfW = logoWidthPx * 0.5f;
    float halfH = logoHeightPx * 0.5f;

    float cosR = std::cos(effectiveRotation * Pi / 180.0f);
    float sinR = std::sin(effectiveRotation * Pi / 180.0f);

    float cornersX[4] = {-halfW, halfW, -halfW, halfW};
    float cornersY[4] = {-halfH, -halfH, halfH, halfH};

    float vpW = static_cast<float>(viewportWidth);
    float vpH = static_cast<float>(viewportHeight);

    for (int i = 0; i < 4; i++)
    {
        float rx = cornersX[i] * cosR - cornersY[i] * sinR;
        float ry = cornersX[i] * sinR + cornersY[i] * cosR;

        float px = centerX + rx;
        float py = centerY + ry;

        outVertices[i * 2 + 0] = (px / vpW) * 2.0f - 1.0f;
        outVertices[i * 2 + 1] = 1.0f - (py / vpH) * 2.0f;
    }
}

void LogoOverlay::Draw(const Audio::FrameAudioData& audioData,
                       ShaderCache& shaderCache,
                       int viewportWidth, int viewportHeight,
                       float deltaTime)
{
    if (!m_enabled)
    {
        return;
    }

    if (m_hasPendingUpload)
    {
        UploadPendingTexture();
    }

    if (!m_texture || m_texture->Empty())
    {
        return;
    }

    if (viewportWidth == 0 || viewportHeight == 0)
    {
        return;
    }

    // Clamp delta time to prevent jumps after pauses
    float dt = std::min(deltaTime, MaxDeltaTime);
    m_animTime += dt;

    // --- Random effect cycling ---
    if (m_randomReactive)
    {
        m_randomReactiveTimer += dt;
        if (m_randomReactiveTimer >= m_randomInterval)
        {
            m_randomReactiveTimer = 0.0f;
            int pick = 1 + (std::rand() % 3); // 1=Pulse, 2=Glow, 3=ColorShift
            m_reactiveEffect = static_cast<LogoReactiveEffect>(pick);
        }
    }
    if (m_randomMotion)
    {
        m_randomMotionTimer += dt;
        if (m_randomMotionTimer >= m_randomInterval)
        {
            m_randomMotionTimer = 0.0f;
            int pick = 1 + (std::rand() % 3); // 1=Spin, 2=Bounce, 3=Sway
            m_motionEffect = static_cast<LogoMotionEffect>(pick);
            m_animTime = 0.0f;
        }
    }

    // --- Compute reactive effect contributions ---
    float effectiveScale = m_scale;
    float effectiveOpacity = m_opacity;
    float hueShift = 0.0f;

    switch (m_reactiveEffect)
    {
        case LogoReactiveEffect::None:
            break;
        case LogoReactiveEffect::Pulse:
        {
            float pulse = static_cast<float>(audioData.bassAtt) * m_reactiveSensitivity * 0.15f;
            effectiveScale = m_scale * (1.0f + pulse);
            break;
        }
        case LogoReactiveEffect::Glow:
        {
            float midEnergy = static_cast<float>(audioData.midAtt) * m_reactiveSensitivity * 0.3f;
            effectiveOpacity = std::max(0.0f, std::min(1.0f, m_opacity * (1.0f + midEnergy * std::sin(m_animTime * 4.0f))));
            break;
        }
        case LogoReactiveEffect::ColorShift:
        {
            float trebleEnergy = static_cast<float>(audioData.trebAtt) * m_reactiveSensitivity;
            hueShift = trebleEnergy * 0.5f * std::sin(m_animTime * 2.0f);
            break;
        }
    }

    // --- Compute motion effect contributions ---
    float extraRotation = 0.0f;
    float offsetXAdjust = 0.0f;
    float offsetYAdjust = 0.0f;

    if (m_motionSecondsPerCycle > 0.0f)
    {
        float phase = m_animTime / m_motionSecondsPerCycle;

        switch (m_motionEffect)
        {
            case LogoMotionEffect::None:
                break;
            case LogoMotionEffect::Spin:
                extraRotation = phase * 360.0f;
                break;
            case LogoMotionEffect::Bounce:
            {
                float shorterDim = static_cast<float>(std::min(viewportWidth, viewportHeight));
                float amplitude = shorterDim * 0.02f; // 2% of viewport
                offsetYAdjust = amplitude * std::sin(phase * TwoPi);
                break;
            }
            case LogoMotionEffect::Sway:
            {
                float shorterDim = static_cast<float>(std::min(viewportWidth, viewportHeight));
                float amplitude = shorterDim * 0.02f;
                offsetXAdjust = amplitude * std::sin(phase * TwoPi);
                break;
            }
        }
    }

    float totalRotation = m_rotationDegrees + extraRotation;

    // Compute vertex positions
    float vertices[8];
    ComputeTransform(effectiveScale, totalRotation, offsetXAdjust, offsetYAdjust,
                     viewportWidth, viewportHeight, vertices);

    m_mesh.Vertices().Set({{vertices[0], vertices[1]},
                           {vertices[2], vertices[3]},
                           {vertices[4], vertices[5]},
                           {vertices[6], vertices[7]}});

    m_mesh.Update();

    // Get or compile shader
    auto shader = m_shader.lock();
    if (!shader)
    {
        shader = shaderCache.Get("logo_overlay");
    }
    if (!shader)
    {
        std::string vertSrc(LogoShaderVersion);
        std::string fragSrc(LogoShaderVersion);
        vertSrc.append(LogoVertexShader);
        fragSrc.append(LogoFragmentShader);

        shader = std::make_shared<Shader>();
        shader->CompileProgram(vertSrc, fragSrc);
        m_shader = shader;
        shaderCache.Insert("logo_overlay", shader);
    }

    // Render
    shader->Bind();
    shader->SetUniformInt("logo_texture", 0);
    shader->SetUniformFloat("logo_opacity", effectiveOpacity);
    shader->SetUniformFloat("logo_hue_shift", hueShift);

    m_texture->Bind(0);
    m_sampler.Bind(0);

    BlendMode::Set(true, BlendMode::Function::SourceAlpha, BlendMode::Function::OneMinusSourceAlpha);

    m_mesh.Draw();

    m_texture->Unbind(0);
    Sampler::Unbind(0);
    Mesh::Unbind();
    Shader::Unbind();
    BlendMode::SetBlendActive(false);
}

} // namespace Renderer
} // namespace libprojectM

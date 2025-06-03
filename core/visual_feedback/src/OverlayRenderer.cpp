#include "../include/visual_feedback/OverlayRenderer.h"
#include <sstream>
#include <iomanip>
#include <cstddef>

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
#endif

namespace VoxelEditor {
namespace VisualFeedback {

OverlayRenderer::OverlayRenderer()
    : m_frameActive(false) {
    
    initializeTextRenderer();
    initializeLineRenderer();
}

OverlayRenderer::~OverlayRenderer() {
    // Clean up GPU resources
    if (m_textRenderer.fontTexture) {
        glDeleteTextures(1, &m_textRenderer.fontTexture);
    }
    if (m_textRenderer.textShader) {
        glDeleteProgram(m_textRenderer.textShader);
    }
    if (m_textRenderer.vertexBuffer) {
        glDeleteBuffers(1, &m_textRenderer.vertexBuffer);
    }
    if (m_textRenderer.indexBuffer) {
        glDeleteBuffers(1, &m_textRenderer.indexBuffer);
    }
    if (m_lineRenderer.vertexBuffer) {
        glDeleteBuffers(1, &m_lineRenderer.vertexBuffer);
    }
    if (m_lineRenderer.lineShader) {
        glDeleteProgram(m_lineRenderer.lineShader);
    }
}

void OverlayRenderer::renderText(const std::string& text, const Math::Vector2f& position, 
                                const TextStyle& style) {
    if (!m_frameActive) return;
    
    renderTextQuad(text, position, style);
}

void OverlayRenderer::renderWorldText(const std::string& text, const Math::Vector3f& worldPosition, 
                                     const TextStyle& style, const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    Math::Vector2f screenPos = worldToScreen(worldPosition, camera);
    if (isOnScreen(screenPos)) {
        renderTextQuad(text, screenPos, style);
    }
}

void OverlayRenderer::renderPerformanceMetrics(const RenderStats& stats, const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    std::stringstream ss;
    ss << "Frame Time: " << formatTime(stats.frameTime) << "\n";
    ss << "CPU Time: " << formatTime(stats.cpuTime) << "\n";
    ss << "GPU Time: " << formatTime(stats.gpuTime) << "\n";
    ss << "Draw Calls: " << stats.drawCalls << "\n";
    ss << "Triangles: " << stats.triangleCount << "\n";
    ss << "Vertices: " << stats.vertexCount;
    
    renderText(ss.str(), position, TextStyle::Debug());
}

void OverlayRenderer::renderMemoryUsage(size_t used, size_t total, const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    std::stringstream ss;
    ss << "Memory: " << formatBytes(used) << " / " << formatBytes(total);
    ss << " (" << std::fixed << std::setprecision(1) << (100.0f * used / total) << "%)";
    
    renderText(ss.str(), position, TextStyle::Debug());
}

void OverlayRenderer::renderVoxelCount(const std::unordered_map<VoxelData::VoxelResolution, size_t>& counts, 
                                      const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    std::stringstream ss;
    ss << "Voxel Count:\n";
    for (const auto& pair : counts) {
        ss << formatResolution(pair.first) << ": " << pair.second << "\n";
    }
    
    renderText(ss.str(), position, TextStyle::Debug());
}

void OverlayRenderer::renderResolutionIndicator(VoxelData::VoxelResolution current, 
                                               const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    std::string text = "Resolution: " + formatResolution(current);
    renderText(text, position, TextStyle::Default());
}

void OverlayRenderer::renderWorkspaceIndicator(const Math::Vector3f& size, const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    std::stringstream ss;
    ss << "Workspace: " << std::fixed << std::setprecision(1);
    ss << size.x << "m x " << size.y << "m x " << size.z << "m";
    
    renderText(ss.str(), position, TextStyle::Default());
}

void OverlayRenderer::renderCameraInfo(const Camera::Camera& camera, const Math::Vector2f& position) {
    if (!m_frameActive) return;
    
    // Get camera position and rotation from the camera object
    Math::Vector3f cameraPos = camera.getPosition();
    // Get camera forward direction as rotation representation
    Math::Vector3f forward = camera.getForward();
    Math::Vector3f cameraRot(forward.x, forward.y, forward.z);
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Camera Position: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")\n";
    ss << "Camera Rotation: (" << cameraRot.x << "°, " << cameraRot.y << "°, " << cameraRot.z << "°)";
    
    renderText(ss.str(), position, TextStyle::Debug());
}

void OverlayRenderer::renderGrid(VoxelData::VoxelResolution resolution, const Math::Vector3f& center, 
                                float extent, const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    generateGridLines(resolution, center, extent);
    flushLineBatch(camera);
}

void OverlayRenderer::renderAxes(const Math::Vector3f& origin, float length, const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    generateAxisLines(origin, length);
    flushLineBatch(camera);
}

void OverlayRenderer::renderCompass(const Camera::Camera& camera, const Math::Vector2f& position, float size) {
    if (!m_frameActive) return;
    
    generateCompassGeometry(camera, position, size);
}

void OverlayRenderer::renderBoundingBoxes(const std::vector<Math::BoundingBox>& boxes, 
                                         const Rendering::Color& color, const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    for (const auto& box : boxes) {
        // Add box edges to line renderer
        Math::Vector3f corners[8] = {
            Math::Vector3f(box.min.x, box.min.y, box.min.z),
            Math::Vector3f(box.max.x, box.min.y, box.min.z),
            Math::Vector3f(box.max.x, box.max.y, box.min.z),
            Math::Vector3f(box.min.x, box.max.y, box.min.z),
            Math::Vector3f(box.min.x, box.min.y, box.max.z),
            Math::Vector3f(box.max.x, box.min.y, box.max.z),
            Math::Vector3f(box.max.x, box.max.y, box.max.z),
            Math::Vector3f(box.min.x, box.max.y, box.max.z)
        };
        
        // Bottom face
        addLine(corners[0], corners[1], color);
        addLine(corners[1], corners[2], color);
        addLine(corners[2], corners[3], color);
        addLine(corners[3], corners[0], color);
        
        // Top face
        addLine(corners[4], corners[5], color);
        addLine(corners[5], corners[6], color);
        addLine(corners[6], corners[7], color);
        addLine(corners[7], corners[4], color);
        
        // Vertical edges
        addLine(corners[0], corners[4], color);
        addLine(corners[1], corners[5], color);
        addLine(corners[2], corners[6], color);
        addLine(corners[3], corners[7], color);
    }
    
    flushLineBatch(camera);
}

void OverlayRenderer::renderRaycast(const Ray& ray, float length, const Rendering::Color& color, 
                                   const Camera::Camera& camera) {
    if (!m_frameActive) return;
    
    Math::Vector3f end = ray.origin + ray.direction * length;
    addLine(ray.origin, end, color);
    flushLineBatch(camera);
}

// void OverlayRenderer::renderFrustum(const Math::Frustum& frustum, const Rendering::Color& color, 
//                                    const Camera::Camera& camera) {
//     if (!m_frameActive) return;
//     
//     // TODO: Extract frustum corners and render lines
//     flushLineBatch(camera);
// }

void OverlayRenderer::beginFrame(int screenWidth, int screenHeight) {
    m_frameActive = true;
    m_textRenderer.screenWidth = screenWidth;
    m_textRenderer.screenHeight = screenHeight;
    
    // Clear batches
    m_textRenderer.vertices.clear();
    m_textRenderer.indices.clear();
    m_lineRenderer.vertices.clear();
    m_lineRenderer.colors.clear();
}

void OverlayRenderer::endFrame() {
    if (m_frameActive) {
        flushTextBatch();
        m_frameActive = false;
    }
}

void OverlayRenderer::initializeTextRenderer() {
    // Create a simple 8x8 bitmap font texture (ASCII characters 32-127)
    const int charWidth = 8;
    const int charHeight = 8;
    const int charsPerRow = 16;
    const int textureSize = 128; // 16x8 characters
    
    // Create a simple bitmap font data (1 = pixel on, 0 = pixel off)
    // For simplicity, we'll create a placeholder texture
    unsigned char* fontData = new unsigned char[textureSize * textureSize * 4];
    
    // Fill with a simple pattern for now (in production, load real font data)
    for (int i = 0; i < textureSize * textureSize * 4; i += 4) {
        fontData[i] = 255;     // R
        fontData[i+1] = 255;   // G
        fontData[i+2] = 255;   // B
        fontData[i+3] = (i % 8 < 4) ? 255 : 0; // A - simple pattern
    }
    
    // Create font texture
    glGenTextures(1, &m_textRenderer.fontTexture);
    glBindTexture(GL_TEXTURE_2D, m_textRenderer.fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSize, textureSize, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, fontData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    delete[] fontData;
    
    // Create text shader program
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 position;
        layout(location = 1) in vec2 texCoord;
        layout(location = 2) in vec4 color;
        
        uniform vec2 screenSize;
        
        out vec2 fragTexCoord;
        out vec4 fragColor;
        
        void main() {
            // Convert from screen coordinates to NDC
            vec2 ndc = (position / screenSize) * 2.0 - 1.0;
            ndc.y = -ndc.y; // Flip Y axis
            
            gl_Position = vec4(ndc, 0.0, 1.0);
            fragTexCoord = texCoord;
            fragColor = color;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 fragTexCoord;
        in vec4 fragColor;
        
        uniform sampler2D fontTexture;
        
        out vec4 color;
        
        void main() {
            float alpha = texture(fontTexture, fragTexCoord).a;
            color = vec4(fragColor.rgb, fragColor.a * alpha);
        }
    )";
    
    // Compile shaders
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    // Create and link program
    m_textRenderer.textShader = glCreateProgram();
    glAttachShader(m_textRenderer.textShader, vertexShader);
    glAttachShader(m_textRenderer.textShader, fragmentShader);
    glLinkProgram(m_textRenderer.textShader);
    
    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Create vertex and index buffers
    glGenBuffers(1, &m_textRenderer.vertexBuffer);
    glGenBuffers(1, &m_textRenderer.indexBuffer);
    
    // Allocate initial buffer sizes
    glBindBuffer(GL_ARRAY_BUFFER, m_textRenderer.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * 1024, nullptr, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_textRenderer.indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * 6 * 256, nullptr, GL_DYNAMIC_DRAW);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OverlayRenderer::initializeLineRenderer() {
    // Create vertex buffer for lines
    glGenBuffers(1, &m_lineRenderer.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineRenderer.vertexBuffer);
    // Allocate initial buffer size
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 7 * 1024, nullptr, GL_DYNAMIC_DRAW);
    
    // Create line shader program
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec4 color;
        
        uniform mat4 mvpMatrix;
        
        out vec4 fragColor;
        
        void main() {
            gl_Position = mvpMatrix * vec4(position, 1.0);
            fragColor = color;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec4 fragColor;
        
        out vec4 color;
        
        void main() {
            color = fragColor;
        }
    )";
    
    // Compile shaders
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    // Create and link program
    m_lineRenderer.lineShader = glCreateProgram();
    glAttachShader(m_lineRenderer.lineShader, vertexShader);
    glAttachShader(m_lineRenderer.lineShader, fragmentShader);
    glLinkProgram(m_lineRenderer.lineShader);
    
    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    m_lineRenderer.depthTest = true;
}

void OverlayRenderer::renderTextQuad(const std::string& text, const Math::Vector2f& position, 
                                    const TextStyle& style) {
    if (text.empty()) return;
    
    const float charWidth = 8.0f * style.size;
    const float charHeight = 8.0f * style.size;
    const float uvCharWidth = 1.0f / 16.0f; // 16 chars per row in texture
    const float uvCharHeight = 1.0f / 8.0f;  // 8 rows in texture
    
    Math::Vector2f currentPos = position;
    
    // Handle alignment
    if (style.alignment != TextAlignment::Left) {
        Math::Vector2f textSize = measureText(text, style.size);
        if (style.alignment == TextAlignment::Center) {
            currentPos.x -= textSize.x * 0.5f;
        } else if (style.alignment == TextAlignment::Right) {
            currentPos.x -= textSize.x;
        }
    }
    
    // Reserve space in vertex array (position + texcoord + color = 2 + 2 + 4 = 8 floats per vertex)
    size_t baseVertex = m_textRenderer.vertices.size() / 8;
    
    for (char c : text) {
        if (c == '\n') {
            currentPos.x = position.x;
            currentPos.y += charHeight * 1.2f;
            continue;
        }
        
        if (c < 32 || c > 127) continue; // Skip non-printable characters
        
        // Calculate texture coordinates for this character
        int charIndex = c - 32;
        int row = charIndex / 16;
        int col = charIndex % 16;
        
        float u0 = col * uvCharWidth;
        float v0 = row * uvCharHeight;
        float u1 = u0 + uvCharWidth;
        float v1 = v0 + uvCharHeight;
        
        // Add vertices for this character quad
        // Top-left
        m_textRenderer.vertices.push_back(currentPos.x);
        m_textRenderer.vertices.push_back(currentPos.y);
        m_textRenderer.vertices.push_back(u0);
        m_textRenderer.vertices.push_back(v0);
        m_textRenderer.vertices.push_back(style.color.r);
        m_textRenderer.vertices.push_back(style.color.g);
        m_textRenderer.vertices.push_back(style.color.b);
        m_textRenderer.vertices.push_back(style.color.a);
        
        // Top-right
        m_textRenderer.vertices.push_back(currentPos.x + charWidth);
        m_textRenderer.vertices.push_back(currentPos.y);
        m_textRenderer.vertices.push_back(u1);
        m_textRenderer.vertices.push_back(v0);
        m_textRenderer.vertices.push_back(style.color.r);
        m_textRenderer.vertices.push_back(style.color.g);
        m_textRenderer.vertices.push_back(style.color.b);
        m_textRenderer.vertices.push_back(style.color.a);
        
        // Bottom-right
        m_textRenderer.vertices.push_back(currentPos.x + charWidth);
        m_textRenderer.vertices.push_back(currentPos.y + charHeight);
        m_textRenderer.vertices.push_back(u1);
        m_textRenderer.vertices.push_back(v1);
        m_textRenderer.vertices.push_back(style.color.r);
        m_textRenderer.vertices.push_back(style.color.g);
        m_textRenderer.vertices.push_back(style.color.b);
        m_textRenderer.vertices.push_back(style.color.a);
        
        // Bottom-left
        m_textRenderer.vertices.push_back(currentPos.x);
        m_textRenderer.vertices.push_back(currentPos.y + charHeight);
        m_textRenderer.vertices.push_back(u0);
        m_textRenderer.vertices.push_back(v1);
        m_textRenderer.vertices.push_back(style.color.r);
        m_textRenderer.vertices.push_back(style.color.g);
        m_textRenderer.vertices.push_back(style.color.b);
        m_textRenderer.vertices.push_back(style.color.a);
        
        // Add indices for this quad
        uint32_t vertexOffset = static_cast<uint32_t>(baseVertex);
        m_textRenderer.indices.push_back(vertexOffset + 0);
        m_textRenderer.indices.push_back(vertexOffset + 1);
        m_textRenderer.indices.push_back(vertexOffset + 2);
        
        m_textRenderer.indices.push_back(vertexOffset + 0);
        m_textRenderer.indices.push_back(vertexOffset + 2);
        m_textRenderer.indices.push_back(vertexOffset + 3);
        
        // Move to next character position
        currentPos.x += charWidth;
        baseVertex += 4;
    }
}

Math::Vector2f OverlayRenderer::measureText(const std::string& text, float size) const {
    // TODO: Calculate text dimensions based on font metrics
    return Math::Vector2f(text.length() * size * 0.6f, size);
}

void OverlayRenderer::flushTextBatch() {
    if (m_textRenderer.vertices.empty()) return;
    
    // Enable blending for text
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    // Use text shader
    glUseProgram(m_textRenderer.textShader);
    
    // Set uniforms
    GLint screenSizeLoc = glGetUniformLocation(m_textRenderer.textShader, "screenSize");
    GLint fontTextureLoc = glGetUniformLocation(m_textRenderer.textShader, "fontTexture");
    
    glUniform2f(screenSizeLoc, static_cast<float>(m_textRenderer.screenWidth), 
                static_cast<float>(m_textRenderer.screenHeight));
    glUniform1i(fontTextureLoc, 0);
    
    // Bind font texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textRenderer.fontTexture);
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_textRenderer.vertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_textRenderer.vertices.size() * sizeof(float), 
                    m_textRenderer.vertices.data());
    
    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_textRenderer.indexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_textRenderer.indices.size() * sizeof(uint32_t),
                    m_textRenderer.indices.data());
    
    // Setup vertex attributes
    glEnableVertexAttribArray(0); // position
    glEnableVertexAttribArray(1); // texCoord
    glEnableVertexAttribArray(2); // color
    
    size_t stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
    
    // Draw text
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_textRenderer.indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    
    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    
    glEnable(GL_DEPTH_TEST);
    
    // Clear for next frame
    m_textRenderer.vertices.clear();
    m_textRenderer.indices.clear();
}

void OverlayRenderer::addLine(const Math::Vector3f& start, const Math::Vector3f& end, 
                             const Rendering::Color& color) {
    m_lineRenderer.vertices.push_back(start);
    m_lineRenderer.vertices.push_back(end);
    m_lineRenderer.colors.push_back(color);
    m_lineRenderer.colors.push_back(color);
}

void OverlayRenderer::flushLineBatch(const Camera::Camera& camera) {
    if (m_lineRenderer.vertices.empty()) return;
    
    // Use line shader
    glUseProgram(m_lineRenderer.lineShader);
    
    // Set MVP matrix
    GLint mvpLoc = glGetUniformLocation(m_lineRenderer.lineShader, "mvpMatrix");
    Math::Matrix4f mvpMatrix = camera.getProjectionMatrix() * camera.getViewMatrix();
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvpMatrix.data());
    
    // Create interleaved vertex data
    std::vector<float> interleavedData;
    interleavedData.reserve(m_lineRenderer.vertices.size() * 7);
    
    for (size_t i = 0; i < m_lineRenderer.vertices.size(); ++i) {
        interleavedData.push_back(m_lineRenderer.vertices[i].x);
        interleavedData.push_back(m_lineRenderer.vertices[i].y);
        interleavedData.push_back(m_lineRenderer.vertices[i].z);
        interleavedData.push_back(m_lineRenderer.colors[i].r);
        interleavedData.push_back(m_lineRenderer.colors[i].g);
        interleavedData.push_back(m_lineRenderer.colors[i].b);
        interleavedData.push_back(m_lineRenderer.colors[i].a);
    }
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_lineRenderer.vertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, interleavedData.size() * sizeof(float),
                    interleavedData.data());
    
    // Setup vertex attributes
    glEnableVertexAttribArray(0); // position
    glEnableVertexAttribArray(1); // color
    
    size_t stride = 7 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    
    // Enable/disable depth test based on settings
    if (m_lineRenderer.depthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    // Enable blending for lines
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw lines
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_lineRenderer.vertices.size()));
    
    // Cleanup
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    
    // Reset depth test
    glEnable(GL_DEPTH_TEST);
    
    // Clear for next batch
    m_lineRenderer.vertices.clear();
    m_lineRenderer.colors.clear();
}

void OverlayRenderer::generateGridLines(VoxelData::VoxelResolution resolution, 
                                       const Math::Vector3f& center, float extent) {
    float voxelSize = VoxelData::getVoxelSize(resolution);
    Rendering::Color gridColor(0.5f, 0.5f, 0.5f, 0.3f);
    
    int gridCount = static_cast<int>(extent / voxelSize);
    float halfExtent = gridCount * voxelSize * 0.5f;
    
    // Generate grid lines in XZ plane
    for (int i = -gridCount; i <= gridCount; ++i) {
        float offset = i * voxelSize;
        
        // Lines parallel to X axis
        addLine(
            Math::Vector3f(center.x - halfExtent, center.y, center.z + offset),
            Math::Vector3f(center.x + halfExtent, center.y, center.z + offset),
            gridColor
        );
        
        // Lines parallel to Z axis
        addLine(
            Math::Vector3f(center.x + offset, center.y, center.z - halfExtent),
            Math::Vector3f(center.x + offset, center.y, center.z + halfExtent),
            gridColor
        );
    }
}

void OverlayRenderer::generateAxisLines(const Math::Vector3f& origin, float length) {
    // X axis - Red
    addLine(origin, origin + Math::Vector3f(length, 0, 0), Rendering::Color::Red());
    
    // Y axis - Green
    addLine(origin, origin + Math::Vector3f(0, length, 0), Rendering::Color::Green());
    
    // Z axis - Blue
    addLine(origin, origin + Math::Vector3f(0, 0, length), Rendering::Color::Blue());
}

void OverlayRenderer::generateCompassGeometry(const Camera::Camera& camera, 
                                             const Math::Vector2f& position, float size) {
    // TODO: Generate compass rose geometry based on camera orientation
}

std::string OverlayRenderer::formatBytes(size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return ss.str();
}

std::string OverlayRenderer::formatTime(float milliseconds) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << milliseconds << " ms";
    return ss.str();
}

std::string OverlayRenderer::formatResolution(VoxelData::VoxelResolution res) const {
    switch (res) {
        case VoxelData::VoxelResolution::Size_1cm: return "1cm";
        case VoxelData::VoxelResolution::Size_2cm: return "2cm";
        case VoxelData::VoxelResolution::Size_4cm: return "4cm";
        case VoxelData::VoxelResolution::Size_8cm: return "8cm";
        case VoxelData::VoxelResolution::Size_16cm: return "16cm";
        case VoxelData::VoxelResolution::Size_32cm: return "32cm";
        case VoxelData::VoxelResolution::Size_64cm: return "64cm";
        case VoxelData::VoxelResolution::Size_128cm: return "128cm";
        case VoxelData::VoxelResolution::Size_256cm: return "256cm";
        case VoxelData::VoxelResolution::Size_512cm: return "512cm";
        case VoxelData::VoxelResolution::COUNT: return "Invalid";
    }
    return "Unknown";
}

Math::Vector2f OverlayRenderer::worldToScreen(const Math::Vector3f& worldPos, 
                                             const Camera::Camera& camera) const {
    // Transform world position to clip space
    Math::Vector4f worldPos4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
    Math::Matrix4f viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();
    Math::Vector4f clipPos = viewProj * worldPos4;
    
    // Perform perspective divide
    if (clipPos.w == 0.0f) {
        return Math::Vector2f(-1.0f, -1.0f); // Behind camera
    }
    
    Math::Vector3f ndcPos(clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w);
    
    // Convert from NDC (-1 to 1) to screen coordinates
    float screenX = (ndcPos.x + 1.0f) * 0.5f * m_textRenderer.screenWidth;
    float screenY = (1.0f - ndcPos.y) * 0.5f * m_textRenderer.screenHeight; // Flip Y
    
    return Math::Vector2f(screenX, screenY);
}

bool OverlayRenderer::isOnScreen(const Math::Vector2f& screenPos) const {
    return screenPos.x >= 0 && screenPos.x < m_textRenderer.screenWidth &&
           screenPos.y >= 0 && screenPos.y < m_textRenderer.screenHeight;
}

} // namespace VisualFeedback
} // namespace VoxelEditor
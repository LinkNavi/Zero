#include "Renderer.h"
#include <iostream>
#include <cstring>
#include "glad/glad.h"
Renderer::~Renderer() {
    if (mInitialized) {
        Shutdown();
    }
}

bool Renderer::Init() {
    // GLAD is already initialized by GLFW in main.cpp
    
    // Print OpenGL info
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable backface culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    
    // Create default shader
    mDefaultShader = CreateDefaultShader();
    if (mDefaultShader == 0) {
        std::cerr << "Failed to create default shader" << std::endl;
        return false;
    }
    
    mInitialized = true;
    return true;
}

void Renderer::Shutdown() {
    if (mDefaultShader != 0) {
        glDeleteProgram(mDefaultShader);
        mDefaultShader = 0;
    }
    mInitialized = false;
}

void Renderer::BeginFrame() {
    glClearColor(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame() {
    // Nothing needed here - GLFW handles buffer swapping
}

void Renderer::SetClearColor(float r, float g, float b, float a) {
    mClearColor[0] = r;
    mClearColor[1] = g;
    mClearColor[2] = b;
    mClearColor[3] = a;
}

Mesh Renderer::CreateMesh(const Vertex* vertices, uint32_t numVertices,
                          const uint16_t* indices, uint32_t numIndices) {
    Mesh mesh = {};
    mesh.numIndices = numIndices;
    
    // Create VAO
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);
    
    // Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    
    // Create EBO
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(uint16_t), indices, GL_STATIC_DRAW);
    
    // Set vertex attributes
    // Position (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    
    // Color (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    
    // TexCoord (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
    
    // Unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    return mesh;
}

Mesh Renderer::CreateCube() {
    Vertex vertices[] = {
        // Front face (red)
        {{-1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        
        // Back face (green)
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        
        // Top face (blue)
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        
        // Bottom face (yellow)
        {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f,  1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        
        // Right face (magenta)
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        
        // Left face (cyan)
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };
    
    uint16_t indices[] = {
        0, 1, 2,  0, 2, 3,      // Front
        4, 5, 6,  4, 6, 7,      // Back
        8, 9, 10, 8, 10, 11,    // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Right
        20, 21, 22, 20, 22, 23  // Left
    };
    
    return CreateMesh(vertices, 24, indices, 36);
}

Mesh Renderer::CreateQuad() {
    Vertex vertices[] = {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };
    
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
    
    return CreateMesh(vertices, 4, indices, 6);
}

void Renderer::DestroyMesh(Mesh& mesh) {
    if (mesh.vao != 0) {
        glDeleteVertexArrays(1, &mesh.vao);
        mesh.vao = 0;
    }
    if (mesh.vbo != 0) {
        glDeleteBuffers(1, &mesh.vbo);
        mesh.vbo = 0;
    }
    if (mesh.ebo != 0) {
        glDeleteBuffers(1, &mesh.ebo);
        mesh.ebo = 0;
    }
}

GLuint Renderer::CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    // Check compilation status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint Renderer::LinkProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    // Check linking status
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

GLuint Renderer::CreateShader(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);
    if (vertexShader == 0) return 0;
    
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }
    
    GLuint program = LinkProgram(vertexShader, fragmentShader);
    
    // Shaders can be deleted after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

GLuint Renderer::CreateDefaultShader() {
    const char* vertexSrc = R"(
        #version 330 core
        
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec4 aColor;
        layout(location = 2) in vec2 aTexCoord;
        
        uniform mat4 uMVP;
        
        out vec4 vColor;
        out vec2 vTexCoord;
        
        void main() {
            gl_Position = uMVP * vec4(aPosition, 1.0);
            vColor = aColor;
            vTexCoord = aTexCoord;
        }
    )";
    
    const char* fragmentSrc = R"(
        #version 330 core
        
        in vec4 vColor;
        in vec2 vTexCoord;
        
        out vec4 FragColor;
        
        void main() {
            FragColor = vColor;
        }
    )";
    
    return CreateShader(vertexSrc, fragmentSrc);
}

void Renderer::DestroyShader(GLuint program) {
    if (program != 0) {
        glDeleteProgram(program);
    }
}

void Renderer::DrawMesh(const Mesh& mesh, GLuint shader, const float* mvp) {
    glUseProgram(shader);
    
    // Set MVP uniform
    GLint mvpLoc = glGetUniformLocation(shader, "uMVP");
    if (mvpLoc != -1) {
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);
    }
    
    // Bind VAO and draw
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

void Renderer::SetViewport(int width, int height) {
    glViewport(0, 0, width, height);
}

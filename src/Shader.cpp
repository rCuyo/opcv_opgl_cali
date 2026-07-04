// ============================================================================
//  Shader.cpp
//  Implementación de la carga y compilación de shaders GLSL.
// ============================================================================
#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

Shader::~Shader()
{
    if (m_program != 0)
        glDeleteProgram(m_program);
}

Shader::Shader(Shader&& other) noexcept
    : m_program(other.m_program)
{
    other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        if (m_program != 0)
            glDeleteProgram(m_program);
        m_program       = other.m_program;
        other.m_program = 0;
    }
    return *this;
}

bool Shader::readFile(const std::string& path, std::string& out)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Shader] No se pudo abrir: " << path << std::endl;
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

GLuint Shader::compileStage(GLenum type, const std::string& source,
                            const std::string& label)
{
    GLuint shader   = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Comprobación de errores de compilación con log legible.
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(static_cast<size_t>(logLen) + 1, '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        std::cerr << "[Shader] Error compilando " << label << ":\n"
                  << log.data() << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::loadFromFiles(const std::string& vertexPath,
                           const std::string& fragmentPath)
{
    std::string vsSource, fsSource;
    if (!readFile(vertexPath, vsSource) || !readFile(fragmentPath, fsSource))
        return false;

    GLuint vs = compileStage(GL_VERTEX_SHADER,   vsSource, vertexPath);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, fsSource, fragmentPath);
    if (vs == 0 || fs == 0) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    // Enlace del programa.
    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    // Los shaders individuales ya no hacen falta tras el enlace.
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(static_cast<size_t>(logLen) + 1, '\0');
        glGetProgramInfoLog(m_program, logLen, nullptr, log.data());
        std::cerr << "[Shader] Error enlazando programa ("
                  << vertexPath << " + " << fragmentPath << "):\n"
                  << log.data() << std::endl;
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }
    return true;
}

void Shader::use() const
{
    glUseProgram(m_program);
}

GLint Shader::uniformLocation(const std::string& name) const
{
    return glGetUniformLocation(m_program, name.c_str());
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(uniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(uniformLocation(name), value);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE,
                       glm::value_ptr(value));
}

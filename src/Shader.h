// ============================================================================
//  Shader.h
//  Encapsula un programa de shaders de OpenGL (vertex + fragment).
//
//  Responsabilidades:
//    - Cargar el código GLSL desde archivos.
//    - Compilar, enlazar y reportar errores con mensajes claros.
//    - Ofrecer setters tipados para uniforms (RAII: el programa se destruye
//      automáticamente con el objeto).
// ============================================================================
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

/// Programa de shaders (vertex + fragment) con helpers de uniforms.
class Shader
{
public:
    Shader() = default;
    ~Shader();

    // No copiable (posee un recurso de GPU), pero sí movible.
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    /// Carga, compila y enlaza los shaders desde archivos GLSL.
    /// Imprime el log de compilación/enlace en caso de error.
    bool loadFromFiles(const std::string& vertexPath,
                       const std::string& fragmentPath);

    /// Activa el programa (glUseProgram).
    void use() const;

    // ---- Setters de uniforms (el programa debe estar activo) -------------
    void setInt (const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

    GLuint id() const { return m_program; }
    bool   isValid() const { return m_program != 0; }

private:
    /// Compila una etapa de shader y devuelve su id (0 si falla).
    static GLuint compileStage(GLenum type, const std::string& source,
                               const std::string& label);

    /// Lee un archivo de texto completo.
    static bool readFile(const std::string& path, std::string& out);

    GLint uniformLocation(const std::string& name) const;

    GLuint m_program = 0;
};

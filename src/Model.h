// ============================================================================
//  Model.h
//  Malla 3D almacenada en GPU con OpenGL moderno (VAO + VBO + EBO).
//
//  ¿Cómo se renderiza un modelo?
//  -----------------------------
//  1) Los vértices (posición + normal, o posición + color) se suben UNA vez
//     a un VBO (Vertex Buffer Object) en la GPU.
//  2) Los índices de los triángulos se suben a un EBO (Element Buffer
//     Object), evitando duplicar vértices compartidos.
//  3) El VAO (Vertex Array Object) recuerda el layout de atributos
//     (qué bytes del VBO corresponden a posición, normal, etc.).
//  4) En cada frame basta con activar el VAO y llamar a glDrawElements:
//     el vertex shader transforma cada vértice con las matrices
//     model/view/projection y el fragment shader calcula el color final.
//
//  Se incluyen fábricas estáticas para las tres geometrías del proyecto:
//  cubo (rojo), pirámide (verde) y ejes XYZ.
// ============================================================================
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

/// Malla en GPU: VAO + VBO (+ EBO opcional) con su primitiva de dibujo.
class Model
{
public:
    /// Layout de los atributos de vértice del VBO.
    enum class VertexLayout
    {
        PositionNormal,   ///< 6 floats: pos.xyz + normal.xyz  (modelos con luz)
        PositionColor     ///< 6 floats: pos.xyz + color.rgb   (ejes, sin luz)
    };

    Model() = default;
    ~Model();

    // No copiable (posee recursos de GPU), sí movible.
    Model(const Model&)            = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    /// Crea los buffers de GPU con geometría indexada.
    /// @param vertices  atributos intercalados según el layout (6 floats/vtx)
    /// @param indices   índices de primitivas (EBO)
    /// @param layout    interpretación de los atributos
    /// @param primitive GL_TRIANGLES, GL_LINES, ...
    void create(const std::vector<float>&  vertices,
                const std::vector<GLuint>& indices,
                VertexLayout layout,
                GLenum primitive = GL_TRIANGLES);

    /// Dibuja la malla (el shader y los uniforms deben estar preparados).
    void draw() const;

    // ---- Fábricas de las geometrías del proyecto --------------------------
    /// Cubo unitario centrado en XY, apoyado sobre el plano Z=0 y elevándose
    /// hacia Z+ del objeto (24 vértices con normales por cara).
    static Model createCube(float size);

    /// Pirámide de base cuadrada apoyada sobre Z=0 con ápice en el eje Z.
    static Model createPyramid(float baseSize, float height);

    /// Ejes XYZ como líneas de colores: X rojo, Y verde, Z azul.
    static Model createAxes(float length);

private:
    void destroy();

    GLuint  m_vao        = 0;
    GLuint  m_vbo        = 0;
    GLuint  m_ebo        = 0;
    GLsizei m_indexCount = 0;
    GLenum  m_primitive  = GL_TRIANGLES;
};

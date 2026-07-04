// ============================================================================
//  Model.cpp
//  Creación de buffers de GPU y geometrías del proyecto.
// ============================================================================
#include "Model.h"

#include <glm/glm.hpp>
#include <utility>

// ---------------------------------------------------------------------------
// Gestión de recursos de GPU
// ---------------------------------------------------------------------------
Model::~Model()
{
    destroy();
}

Model::Model(Model&& other) noexcept
    : m_vao(other.m_vao)
    , m_vbo(other.m_vbo)
    , m_ebo(other.m_ebo)
    , m_indexCount(other.m_indexCount)
    , m_primitive(other.m_primitive)
{
    other.m_vao = other.m_vbo = other.m_ebo = 0;
    other.m_indexCount = 0;
}

Model& Model::operator=(Model&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_vao        = other.m_vao;
        m_vbo        = other.m_vbo;
        m_ebo        = other.m_ebo;
        m_indexCount = other.m_indexCount;
        m_primitive  = other.m_primitive;
        other.m_vao = other.m_vbo = other.m_ebo = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

void Model::destroy()
{
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vao = m_vbo = m_ebo = 0;
    m_indexCount = 0;
}

void Model::create(const std::vector<float>&  vertices,
                   const std::vector<GLuint>& indices,
                   VertexLayout /*layout*/,
                   GLenum primitive)
{
    destroy();
    m_primitive  = primitive;
    m_indexCount = static_cast<GLsizei>(indices.size());

    // VAO: recuerda los enlaces de buffers y el layout de atributos.
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // VBO: atributos de vértice intercalados (6 floats por vértice).
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);

    // EBO: índices de las primitivas.
    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)),
                 indices.data(), GL_STATIC_DRAW);

    // Ambos layouts comparten estructura: 2 atributos vec3 intercalados.
    //   location 0: posición (xyz)
    //   location 1: normal o color (xyz / rgb)
    const GLsizei stride = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Model::draw() const
{
    if (m_vao == 0 || m_indexCount == 0)
        return;
    glBindVertexArray(m_vao);
    glDrawElements(m_primitive, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Geometrías
// ---------------------------------------------------------------------------
namespace {

/// Añade una cara cuadrada (4 vértices + 6 índices) con normal constante.
/// Los vértices se pasan en orden antihorario visto desde fuera.
void addQuad(std::vector<float>& vertices, std::vector<GLuint>& indices,
             const glm::vec3& a, const glm::vec3& b,
             const glm::vec3& c, const glm::vec3& d,
             const glm::vec3& n)
{
    const GLuint base = static_cast<GLuint>(vertices.size() / 6);
    for (const glm::vec3& p : {a, b, c, d}) {
        vertices.insert(vertices.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
    }
    indices.insert(indices.end(),
                   {base, base + 1, base + 2, base, base + 2, base + 3});
}

/// Añade un triángulo con la normal calculada del propio triángulo
/// (orden antihorario visto desde fuera => normal hacia fuera).
void addTriangle(std::vector<float>& vertices, std::vector<GLuint>& indices,
                 const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
    const glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
    const GLuint base = static_cast<GLuint>(vertices.size() / 6);
    for (const glm::vec3& p : {a, b, c}) {
        vertices.insert(vertices.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
    }
    indices.insert(indices.end(), {base, base + 1, base + 2});
}

} // namespace

Model Model::createCube(float size)
{
    // Cubo centrado en XY, apoyado en Z=0 y elevándose hacia +Z (en el
    // espacio del MODELO; la matriz de modelo lo orienta sobre el tablero).
    const float h = size * 0.5f;   // semilado en XY
    const float z0 = 0.0f, z1 = size;

    std::vector<float>  v;
    std::vector<GLuint> i;
    v.reserve(24 * 6);
    i.reserve(36);

    // Cada cara con su normal propia (sombreado plano correcto).
    addQuad(v, i, {-h,-h,z1}, { h,-h,z1}, { h, h,z1}, {-h, h,z1}, {0,0, 1});  // tapa
    addQuad(v, i, {-h, h,z0}, { h, h,z0}, { h,-h,z0}, {-h,-h,z0}, {0,0,-1});  // base
    addQuad(v, i, {-h,-h,z0}, { h,-h,z0}, { h,-h,z1}, {-h,-h,z1}, {0,-1,0});  // frente
    addQuad(v, i, { h, h,z0}, {-h, h,z0}, {-h, h,z1}, { h, h,z1}, {0, 1,0});  // atrás
    addQuad(v, i, {-h, h,z0}, {-h,-h,z0}, {-h,-h,z1}, {-h, h,z1}, {-1,0,0});  // izquierda
    addQuad(v, i, { h,-h,z0}, { h, h,z0}, { h, h,z1}, { h,-h,z1}, { 1,0,0});  // derecha

    Model m;
    m.create(v, i, VertexLayout::PositionNormal, GL_TRIANGLES);
    return m;
}

Model Model::createPyramid(float baseSize, float height)
{
    // Base cuadrada apoyada en Z=0, ápice sobre el eje Z (+Z del modelo).
    const float h = baseSize * 0.5f;
    const glm::vec3 apex(0.0f, 0.0f, height);
    const glm::vec3 p0(-h, -h, 0.0f);   // esquinas de la base en orden CCW
    const glm::vec3 p1( h, -h, 0.0f);   // vistas desde arriba (+Z)
    const glm::vec3 p2( h,  h, 0.0f);
    const glm::vec3 p3(-h,  h, 0.0f);

    std::vector<float>  v;
    std::vector<GLuint> i;

    // Cuatro caras laterales; addTriangle calcula la normal hacia fuera.
    addTriangle(v, i, p0, p1, apex);
    addTriangle(v, i, p1, p2, apex);
    addTriangle(v, i, p2, p3, apex);
    addTriangle(v, i, p3, p0, apex);

    // Base (mirando hacia -Z): orden horario visto desde arriba.
    addQuad(v, i, p3, p2, p1, p0, {0.0f, 0.0f, -1.0f});

    Model m;
    m.create(v, i, VertexLayout::PositionNormal, GL_TRIANGLES);
    return m;
}

Model Model::createAxes(float length)
{
    // Ejes del sistema de coordenadas del TABLERO (convención OpenCV):
    //   X rojo  -> a lo largo de las columnas de esquinas
    //   Y verde -> a lo largo de las filas
    //   Z azul  -> se dibuja hacia -Z porque, con la convención OpenCV
    //              (X derecha, Y abajo), -Z es la dirección que sale del
    //              tablero hacia la cámara ("hacia arriba" visualmente).
    // Layout: posición (xyz) + color (rgb).
    const float l = length;
    const std::vector<float> v = {
        // origen           color        extremo
        0, 0, 0,   1, 0, 0,     // X (rojo)
        l, 0, 0,   1, 0, 0,
        0, 0, 0,   0, 1, 0,     // Y (verde)
        0, l, 0,   0, 1, 0,
        0, 0, 0,   0, 0, 1,     // Z (azul)
        0, 0, -l,  0, 0, 1,
    };
    const std::vector<GLuint> i = {0, 1, 2, 3, 4, 5};

    Model m;
    m.create(v, i, VertexLayout::PositionColor, GL_LINES);
    return m;
}

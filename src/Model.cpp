// ============================================================================
//  Model.cpp
//  Creación de buffers de GPU y geometrías del proyecto.
// ============================================================================
#include "Model.h"

#include <glm/glm.hpp>
#include <cmath>
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
                   VertexLayout layout,
                   GLenum primitive)
{
    destroy();
    m_primitive  = primitive;
    m_indexCount = static_cast<GLsizei>(indices.size());

    // VAO: recuerda los enlaces de buffers y el layout de atributos.
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // VBO: atributos de vértice intercalados (6 u 8 floats por vértice).
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

    // Layout de atributos:
    //   location 0: posición (xyz)                       [todos los layouts]
    //   location 1: normal o color (xyz / rgb)           [todos los layouts]
    //   location 2: coordenadas de textura (uv)          [solo PositionNormalUV]
    const bool hasUV = (layout == VertexLayout::PositionNormalUV);
    const GLsizei stride = (hasUV ? 8 : 6) * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    if (hasUV) {
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                              (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }

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

// ---------------------------------------------------------------------------
// Primitivas suaves (personajes procedurales: Pikachu / Raichu)
// ---------------------------------------------------------------------------
namespace {
constexpr float kPi = 3.14159265358979323846f;

inline void pushVN(std::vector<float>& v, const glm::vec3& p, const glm::vec3& n)
{
    v.insert(v.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
}
} // namespace

Model Model::createEllipsoid(const glm::vec3& r, int stacks, int slices)
{
    std::vector<float>  v;
    std::vector<GLuint> idx;
    v.reserve(static_cast<size_t>((stacks + 1) * (slices + 1)) * 6);

    // Rejilla esférica parametrizada por (phi: polo a polo, theta: alrededor).
    // La normal de un elipsoide en (x,y,z) es proporcional a
    // (x/rx^2, y/ry^2, z/rz^2), lo que da sombreado correcto con radios
    // distintos (a diferencia de escalar una esfera unidad).
    for (int i = 0; i <= stacks; ++i) {
        const float phi = kPi * static_cast<float>(i) / stacks;
        const float sp = std::sin(phi), cp = std::cos(phi);
        for (int j = 0; j <= slices; ++j) {
            const float th = 2.0f * kPi * static_cast<float>(j) / slices;
            const float st = std::sin(th), ct = std::cos(th);
            const glm::vec3 p(r.x * sp * ct, r.y * sp * st, r.z * cp);
            const glm::vec3 n = glm::normalize(glm::vec3(
                p.x / (r.x * r.x), p.y / (r.y * r.y), p.z / (r.z * r.z)));
            pushVN(v, p, n);
        }
    }

    const int cols = slices + 1;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            const GLuint a = static_cast<GLuint>(i * cols + j);
            const GLuint b = static_cast<GLuint>((i + 1) * cols + j);
            idx.insert(idx.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    }

    Model m;
    m.create(v, idx, VertexLayout::PositionNormal, GL_TRIANGLES);
    return m;
}

Model Model::createCone(float radius, float height, int slices)
{
    std::vector<float>  v;
    std::vector<GLuint> idx;
    const glm::vec3 apex(0.0f, 0.0f, height);

    // Cara lateral: normal de la superficie cónica = (h·cosθ, h·sinθ, r),
    // constante a lo largo de la generatriz (deducida del producto vectorial
    // de las tangentes). Se genera un triángulo por sector.
    for (int j = 0; j < slices; ++j) {
        const float t0 = 2.0f * kPi * static_cast<float>(j) / slices;
        const float t1 = 2.0f * kPi * static_cast<float>(j + 1) / slices;
        const glm::vec3 b0(radius * std::cos(t0), radius * std::sin(t0), 0.0f);
        const glm::vec3 b1(radius * std::cos(t1), radius * std::sin(t1), 0.0f);
        const glm::vec3 n0 = glm::normalize(
            glm::vec3(height * std::cos(t0), height * std::sin(t0), radius));
        const glm::vec3 n1 = glm::normalize(
            glm::vec3(height * std::cos(t1), height * std::sin(t1), radius));
        const glm::vec3 na = glm::normalize(n0 + n1);
        const GLuint base = static_cast<GLuint>(v.size() / 6);
        pushVN(v, b0, n0);
        pushVN(v, b1, n1);
        pushVN(v, apex, na);
        idx.insert(idx.end(), {base, base + 1, base + 2});
    }

    // Tapa de la base (mira hacia -Z).
    const GLuint centerIdx = static_cast<GLuint>(v.size() / 6);
    pushVN(v, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    for (int j = 0; j < slices; ++j) {
        const float t0 = 2.0f * kPi * static_cast<float>(j) / slices;
        const float t1 = 2.0f * kPi * static_cast<float>(j + 1) / slices;
        const glm::vec3 b0(radius * std::cos(t0), radius * std::sin(t0), 0.0f);
        const glm::vec3 b1(radius * std::cos(t1), radius * std::sin(t1), 0.0f);
        const GLuint base = static_cast<GLuint>(v.size() / 6);
        pushVN(v, b1, glm::vec3(0.0f, 0.0f, -1.0f));
        pushVN(v, b0, glm::vec3(0.0f, 0.0f, -1.0f));
        idx.insert(idx.end(), {centerIdx, base, base + 1});
    }

    Model m;
    m.create(v, idx, VertexLayout::PositionNormal, GL_TRIANGLES);
    return m;
}

Model Model::createBox(const glm::vec3& h)
{
    std::vector<float>  v;
    std::vector<GLuint> i;
    v.reserve(24 * 6);
    i.reserve(36);

    addQuad(v, i, {-h.x,-h.y, h.z}, { h.x,-h.y, h.z}, { h.x, h.y, h.z}, {-h.x, h.y, h.z}, {0,0, 1});
    addQuad(v, i, {-h.x, h.y,-h.z}, { h.x, h.y,-h.z}, { h.x,-h.y,-h.z}, {-h.x,-h.y,-h.z}, {0,0,-1});
    addQuad(v, i, {-h.x,-h.y,-h.z}, { h.x,-h.y,-h.z}, { h.x,-h.y, h.z}, {-h.x,-h.y, h.z}, {0,-1,0});
    addQuad(v, i, { h.x, h.y,-h.z}, {-h.x, h.y,-h.z}, {-h.x, h.y, h.z}, { h.x, h.y, h.z}, {0, 1,0});
    addQuad(v, i, {-h.x, h.y,-h.z}, {-h.x,-h.y,-h.z}, {-h.x,-h.y, h.z}, {-h.x, h.y, h.z}, {-1,0,0});
    addQuad(v, i, { h.x,-h.y,-h.z}, { h.x, h.y,-h.z}, { h.x, h.y, h.z}, { h.x,-h.y, h.z}, { 1,0,0});

    Model m;
    m.create(v, i, VertexLayout::PositionNormal, GL_TRIANGLES);
    return m;
}

// ---------------------------------------------------------------------------
// Carga de modelos glTF 2.0 (cgltf, cabecera única en external/cgltf)
// ---------------------------------------------------------------------------
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <glm/gtc/type_ptr.hpp>
#include <cfloat>
#include <cstring>
#include <iostream>

Model Model::loadGltf(const std::string& path, float targetSize,
                      std::string& texturePathOut)
{
    texturePathOut.clear();
    Model empty;   // se devuelve si algo falla (isValid() == false)

    // ---- 1) Parseo del JSON y carga de los buffers binarios (.bin) --------
    cgltf_options options = {};
    cgltf_data*   data    = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data)
            != cgltf_result_success) {
        std::cerr << "[Model] No se pudo parsear glTF: " << path << std::endl;
        return empty;
    }
    if (cgltf_load_buffers(&options, data, path.c_str())
            != cgltf_result_success) {
        std::cerr << "[Model] No se pudieron cargar los buffers de: " << path
                  << std::endl;
        cgltf_free(data);
        return empty;
    }

    // ---- 2) Recorrido de nodos: se hornean sus transformaciones -----------
    // Cada nodo con malla aporta sus primitivas de triángulos. Las
    // posiciones se transforman con la matriz MUNDO del nodo (acumulada por
    // toda la jerarquía) y las normales con su matriz normal.
    std::vector<float>  verts;   // 8 floats/vértice: pos + normal + uv
    std::vector<GLuint> idx;

    for (cgltf_size ni = 0; ni < data->nodes_count; ++ni) {
        const cgltf_node& node = data->nodes[ni];
        if (!node.mesh)
            continue;

        float worldRaw[16];
        cgltf_node_transform_world(&node, worldRaw);
        const glm::mat4 world   = glm::make_mat4(worldRaw);
        const glm::mat3 normalM = glm::mat3(glm::transpose(glm::inverse(world)));

        for (cgltf_size pi = 0; pi < node.mesh->primitives_count; ++pi) {
            const cgltf_primitive& prim = node.mesh->primitives[pi];
            if (prim.type != cgltf_primitive_type_triangles)
                continue;   // solo triángulos (lo que exporta Sketchfab)

            // Accessors de los atributos que nos interesan.
            const cgltf_accessor* aPos = nullptr;
            const cgltf_accessor* aNrm = nullptr;
            const cgltf_accessor* aUV  = nullptr;
            for (cgltf_size ai = 0; ai < prim.attributes_count; ++ai) {
                const cgltf_attribute& att = prim.attributes[ai];
                if (att.type == cgltf_attribute_type_position) aPos = att.data;
                if (att.type == cgltf_attribute_type_normal)   aNrm = att.data;
                if (att.type == cgltf_attribute_type_texcoord && !aUV)
                    aUV = att.data;   // TEXCOORD_0
            }
            if (!aPos)
                continue;

            const GLuint base = static_cast<GLuint>(verts.size() / 8);

            for (cgltf_size vi = 0; vi < aPos->count; ++vi) {
                float p[3] = {0, 0, 0}, n[3] = {0, 0, 1}, uv[2] = {0, 0};
                cgltf_accessor_read_float(aPos, vi, p, 3);
                if (aNrm) cgltf_accessor_read_float(aNrm, vi, n, 3);
                if (aUV)  cgltf_accessor_read_float(aUV,  vi, uv, 2);

                glm::vec3 wp = glm::vec3(world * glm::vec4(p[0], p[1], p[2], 1.f));
                glm::vec3 wn = glm::normalize(normalM * glm::vec3(n[0], n[1], n[2]));

                // Cambio de convención glTF -> proyecto: glTF usa +Y arriba;
                // nuestros modelos locales usan +Z arriba (apoyados sobre el
                // tablero). Rotación de +90 grados sobre X: (x,y,z)->(x,-z,y).
                verts.insert(verts.end(), {
                    wp.x, -wp.z, wp.y,
                    wn.x, -wn.z, wn.y,
                    uv[0], uv[1]
                });
            }

            // Índices (si el primitivo no es indexado, se genera 0..N-1).
            if (prim.indices) {
                for (cgltf_size k = 0; k < prim.indices->count; ++k)
                    idx.push_back(base + static_cast<GLuint>(
                        cgltf_accessor_read_index(prim.indices, k)));
            } else {
                for (cgltf_size k = 0; k < aPos->count; ++k)
                    idx.push_back(base + static_cast<GLuint>(k));
            }
        }
    }

    if (verts.empty() || idx.empty()) {
        std::cerr << "[Model] glTF sin triángulos utilizables: " << path
                  << std::endl;
        cgltf_free(data);
        return empty;
    }

    // ---- 3) Textura baseColor (solo la primera: modelos con atlas único) --
    for (cgltf_size mi = 0; mi < data->materials_count; ++mi) {
        const cgltf_material& mat = data->materials[mi];
        if (mat.has_pbr_metallic_roughness &&
            mat.pbr_metallic_roughness.base_color_texture.texture &&
            mat.pbr_metallic_roughness.base_color_texture.texture->image &&
            mat.pbr_metallic_roughness.base_color_texture.texture->image->uri) {

            // La URI es relativa al .gltf y puede venir percent-encoded.
            std::string uri =
                mat.pbr_metallic_roughness.base_color_texture.texture->image->uri;
            cgltf_decode_uri(uri.data());
            uri.resize(std::strlen(uri.c_str()));

            const size_t slash = path.find_last_of("/\\");
            const std::string dir =
                (slash == std::string::npos) ? "" : path.substr(0, slash + 1);
            texturePathOut = dir + uri;
            break;
        }
    }

    cgltf_free(data);

    // ---- 4) Normalización: centrar, apoyar y escalar sobre el tablero -----
    glm::vec3 mn(FLT_MAX), mx(-FLT_MAX);
    for (size_t i = 0; i < verts.size(); i += 8) {
        mn = glm::min(mn, glm::vec3(verts[i], verts[i+1], verts[i+2]));
        mx = glm::max(mx, glm::vec3(verts[i], verts[i+1], verts[i+2]));
    }
    const glm::vec3 extent  = mx - mn;
    const glm::vec3 centerXY((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f, mn.z);
    const float     maxHoriz = std::max(extent.x, extent.y);
    const float     scale    = (maxHoriz > 1e-6f) ? targetSize / maxHoriz : 1.0f;

    for (size_t i = 0; i < verts.size(); i += 8) {
        verts[i]     = (verts[i]     - centerXY.x) * scale;   // centrado en X
        verts[i + 1] = (verts[i + 1] - centerXY.y) * scale;   // centrado en Y
        verts[i + 2] = (verts[i + 2] - centerXY.z) * scale;   // base en Z = 0
    }

    std::cout << "[Model] glTF cargado: " << path << " ("
              << verts.size() / 8 << " vertices, " << idx.size() / 3
              << " triangulos, textura: "
              << (texturePathOut.empty() ? "ninguna" : texturePathOut) << ")"
              << std::endl;

    Model m;
    m.create(verts, idx, VertexLayout::PositionNormalUV, GL_TRIANGLES);
    return m;
}

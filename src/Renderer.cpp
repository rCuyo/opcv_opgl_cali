// ============================================================================
//  Renderer.cpp
//  Implementación de la ventana, el fondo de cámara y la escena 3D.
// ============================================================================
#include "Renderer.h"
#include "Config.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <utility>

namespace {

// Dimensiones de los modelos, expresadas en cuadrados del tablero para que
// escalen automáticamente si cambia SQUARE_SIZE.
constexpr float CUBE_SIZE      = 2.0f * config::SQUARE_SIZE;
constexpr float PYRAMID_BASE   = 2.5f * config::SQUARE_SIZE;
constexpr float PYRAMID_HEIGHT = 2.5f * config::SQUARE_SIZE;
constexpr float AXES_LENGTH    = 3.0f * config::SQUARE_SIZE;

// Separación a cada lado del centro cuando se muestran dos personajes (tecla 5).
constexpr float CHARACTER_OFFSET = 2.8f * config::SQUARE_SIZE;

// Modelo externo glTF (tecla 6): ruta por defecto y tamaño final sobre el
// tablero (lado mayor horizontal, en cuadrados).
constexpr const char* GLTF_MODEL_PATH = "models/lp_old_computer/scene.gltf";
constexpr float       GLTF_TARGET_SIZE = 5.0f * config::SQUARE_SIZE;

/// Callback estático de GLFW: redirige al Renderer dueño de la ventana.
void keyDispatch(GLFWwindow* window, int key, int /*scancode*/,
                 int action, int /*mods*/)
{
    if (action != GLFW_PRESS)
        return;
    auto* self = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (self)
        self->dispatchKey(key);
}

// ---------------------------------------------------------------------------
// Personajes procedurales (Pikachu / Raichu)
//
// Cada personaje se construye como una lista de piezas (ModelPart). Se usa un
// puñado de primitivas suaves (elipsoides, conos y cajas) coloreadas, que se
// dibujan con el MISMO shader iluminado que el cubo/pirámide (uObjectColor por
// pieza). Así se logra un modelo reconocible sin depender de archivos .obj
// externos ni de assets con copyright. El personaje crece a lo largo de +Z
// (los pies quedan en Z≈0); m_boardCenterTransform lo pone de pie sobre el
// tablero, mirando hacia la cámara.
// ---------------------------------------------------------------------------
const glm::vec3 kYellow(0.98f, 0.82f, 0.12f);   // amarillo Pikachu
const glm::vec3 kBlack (0.07f, 0.07f, 0.07f);   // ojos, puntas de orejas
const glm::vec3 kRed   (0.86f, 0.13f, 0.13f);   // mejillas Pikachu
const glm::vec3 kBrown (0.42f, 0.28f, 0.10f);   // base de cola / cola Raichu
const glm::vec3 kOrange(0.95f, 0.55f, 0.13f);   // cuerpo Raichu
const glm::vec3 kCream (0.98f, 0.90f, 0.72f);   // panza Raichu
const glm::vec3 kYcheek(0.96f, 0.80f, 0.16f);   // mejillas Raichu

inline glm::mat4 T(float x, float y, float z)
{
    return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
}
inline glm::mat4 Rx(float deg) { return glm::rotate(glm::mat4(1.0f), glm::radians(deg), glm::vec3(1,0,0)); }
inline glm::mat4 Ry(float deg) { return glm::rotate(glm::mat4(1.0f), glm::radians(deg), glm::vec3(0,1,0)); }

void addPart(std::vector<ModelPart>& parts, Model mesh,
             const glm::mat4& xf, const glm::vec3& color)
{
    ModelPart p;
    p.mesh           = std::move(mesh);
    p.localTransform = xf;
    p.color          = color;
    parts.push_back(std::move(p));
}

std::vector<ModelPart> buildPikachu()
{
    std::vector<ModelPart> p;

    // Cuerpo y cabeza (regordete, cabeza grande).
    addPart(p, Model::createEllipsoid({1.05f, 0.95f, 1.30f}), T(0, 0, 1.45f), kYellow);
    addPart(p, Model::createEllipsoid({1.25f, 1.15f, 1.05f}), T(0, 0, 3.05f), kYellow);

    // Orejas: cono amarillo con punta negra. La cara mira hacia +Y.
    for (float s : {-1.0f, 1.0f}) {
        const glm::mat4 ear = T(0.55f * s, -0.10f, 3.70f) * Ry(28.0f * s) * Rx(-10.0f);
        addPart(p, Model::createCone(0.32f, 1.70f), ear,                   kYellow);
        addPart(p, Model::createCone(0.34f, 0.60f), ear * T(0, 0, 1.15f),  kBlack);
    }

    // Ojos, mejillas rojas y nariz sobre la cara (+Y).
    for (float s : {-1.0f, 1.0f}) {
        addPart(p, Model::createEllipsoid({0.17f, 0.10f, 0.20f}), T(0.50f * s, 0.98f, 3.25f), kBlack);
        addPart(p, Model::createEllipsoid({0.33f, 0.12f, 0.33f}), T(0.82f * s, 0.72f, 2.78f), kRed);
    }
    addPart(p, Model::createEllipsoid({0.08f, 0.07f, 0.06f}), T(0, 1.10f, 3.02f), kBlack);

    // Brazos y pies.
    for (float s : {-1.0f, 1.0f}) {
        addPart(p, Model::createEllipsoid({0.34f, 0.34f, 0.55f}), T(1.02f * s, 0.05f, 1.55f) * Ry(20.0f * s), kYellow);
        addPart(p, Model::createEllipsoid({0.42f, 0.58f, 0.30f}), T(0.52f * s, 0.15f, 0.28f), kYellow);
    }

    // Cola en zigzag (rayo) detrás del cuerpo (-Y), con base marrón.
    addPart(p, Model::createBox({0.16f, 0.10f, 0.30f}), T( 0.00f, -1.15f, 0.90f) * Ry( 35.0f), kBrown);
    addPart(p, Model::createBox({0.17f, 0.11f, 0.42f}), T( 0.28f, -1.20f, 1.50f) * Ry(-42.0f), kYellow);
    addPart(p, Model::createBox({0.17f, 0.11f, 0.48f}), T(-0.05f, -1.25f, 2.25f) * Ry( 38.0f), kYellow);
    addPart(p, Model::createBox({0.18f, 0.11f, 0.50f}), T( 0.30f, -1.30f, 3.00f) * Ry(-34.0f), kYellow);

    return p;
}

std::vector<ModelPart> buildRaichu()
{
    std::vector<ModelPart> p;

    // Cuerpo naranja más alto y estilizado, con panza crema.
    addPart(p, Model::createEllipsoid({1.00f, 0.92f, 1.45f}), T(0, 0.00f, 1.65f), kOrange);
    addPart(p, Model::createEllipsoid({0.62f, 0.45f, 0.95f}), T(0, 0.55f, 1.55f), kCream);
    addPart(p, Model::createEllipsoid({1.18f, 1.08f, 1.00f}), T(0, 0.00f, 3.45f), kOrange);

    // Orejas largas, naranja con punta marrón.
    for (float s : {-1.0f, 1.0f}) {
        const glm::mat4 ear = T(0.50f * s, -0.10f, 4.05f) * Ry(30.0f * s) * Rx(-8.0f);
        addPart(p, Model::createCone(0.28f, 2.20f), ear,                  kOrange);
        addPart(p, Model::createCone(0.30f, 0.80f), ear * T(0, 0, 1.55f), kBrown);
    }

    // Ojos, mejillas amarillas y nariz.
    for (float s : {-1.0f, 1.0f}) {
        addPart(p, Model::createEllipsoid({0.16f, 0.10f, 0.19f}), T(0.48f * s, 0.92f, 3.62f), kBlack);
        addPart(p, Model::createEllipsoid({0.30f, 0.12f, 0.30f}), T(0.78f * s, 0.66f, 3.15f), kYcheek);
    }
    addPart(p, Model::createEllipsoid({0.08f, 0.07f, 0.06f}), T(0, 1.02f, 3.40f), kBlack);

    // Brazos y pies.
    for (float s : {-1.0f, 1.0f}) {
        addPart(p, Model::createEllipsoid({0.30f, 0.30f, 0.60f}), T(0.98f * s, 0.05f, 1.70f) * Ry(18.0f * s), kOrange);
        addPart(p, Model::createEllipsoid({0.40f, 0.55f, 0.28f}), T(0.50f * s, 0.15f, 0.26f), kOrange);
    }

    // Cola larga y fina que sube por detrás y termina en un gran rayo.
    addPart(p, Model::createBox({0.10f, 0.10f, 0.70f}), T(0, -1.20f, 1.20f) * Rx(-35.0f), kBrown);
    addPart(p, Model::createBox({0.10f, 0.10f, 0.70f}), T(0, -1.90f, 2.40f) * Rx(-70.0f), kBrown);
    const glm::mat4 bolt = T(0, -2.55f, 3.10f);
    addPart(p, Model::createBox({0.50f, 0.08f, 0.22f}), bolt * Ry( 35.0f),               kBrown);
    addPart(p, Model::createBox({0.50f, 0.08f, 0.22f}), bolt * T(0.20f, 0, 0.55f) * Ry(-40.0f), kBrown);

    return p;
}

} // namespace

Renderer::~Renderer()
{
    if (m_backgroundTexture)
        glDeleteTextures(1, &m_backgroundTexture);
    if (m_gltfTexture)
        glDeleteTextures(1, &m_gltfTexture);
    if (m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}

void Renderer::loadGltfModel()
{
    // Geometría (posiciones + normales + UV) normalizada sobre el tablero.
    std::string texturePath;
    m_gltfModel = Model::loadGltf(config::resourcePath(GLTF_MODEL_PATH),
                                  GLTF_TARGET_SIZE, texturePath);
    if (!m_gltfModel.isValid()) {
        std::cout << "[Renderer] Sin modelo glTF (tecla 6 desactivada)"
                  << std::endl;
        return;
    }

    // Textura baseColor: se decodifica con OpenCV (sin dependencias extra).
    // Se sube SIN voltear: glTF define v=0 en la fila superior, igual que
    // el orden de filas con el que glTexImage2D interpreta los datos.
    if (!texturePath.empty()) {
        cv::Mat tex = cv::imread(texturePath, cv::IMREAD_COLOR);
        if (!tex.empty()) {
            cv::cvtColor(tex, tex, cv::COLOR_BGR2RGB);
            if (!tex.isContinuous())
                tex = tex.clone();

            glGenTextures(1, &m_gltfTexture);
            glBindTexture(GL_TEXTURE_2D, m_gltfTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, tex.cols, tex.rows, 0,
                         GL_RGB, GL_UNSIGNED_BYTE, tex.data);
        } else {
            std::cerr << "[Renderer] No se pudo leer la textura: "
                      << texturePath << std::endl;
        }
    }

    m_hasGltf = true;
}

void Renderer::dispatchKey(int key)
{
    if (m_keyCallback)
        m_keyCallback(key);
}

bool Renderer::init(int width, int height, const std::string& title)
{
    m_width  = width;
    m_height = height;

    // ---- Ventana y contexto OpenGL 3.3 core --------------------------------
    if (!glfwInit()) {
        std::cerr << "[Renderer] glfwInit falló" << std::endl;
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    // macOS exige contextos forward-compatible para core profile.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // La ventana no se redimensiona: su aspecto debe coincidir con el frame.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "[Renderer] No se pudo crear la ventana" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(m_window);

    // V-Sync: sincroniza el swap con el refresco del monitor (sin tearing).
    glfwSwapInterval(1);

    // Carga de punteros de funciones OpenGL a través de GLFW.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[Renderer] gladLoadGLLoader falló" << std::endl;
        return false;
    }
    std::cout << "[Renderer] OpenGL " << glGetString(GL_VERSION)
              << " | " << glGetString(GL_RENDERER) << std::endl;

    // Teclado: GLFW llama a keyDispatch, que redirige a m_keyCallback.
    glfwSetWindowUserPointer(m_window, this);
    glfwSetKeyCallback(m_window, keyDispatch);

    // ---- Shaders ------------------------------------------------------------
    const std::string shaderDir = "shaders/";
    if (!m_backgroundShader.loadFromFiles(
            config::resourcePath(shaderDir + "background.vert"),
            config::resourcePath(shaderDir + "background.frag")))
        return false;
    if (!m_modelShader.loadFromFiles(
            config::resourcePath(shaderDir + "model.vert"),
            config::resourcePath(shaderDir + "model.frag")))
        return false;
    if (!m_axesShader.loadFromFiles(
            config::resourcePath(shaderDir + "axes.vert"),
            config::resourcePath(shaderDir + "axes.frag")))
        return false;
    if (!m_texturedShader.loadFromFiles(
            config::resourcePath(shaderDir + "model_textured.vert"),
            config::resourcePath(shaderDir + "model_textured.frag")))
        return false;

    // ---- Geometrías -----------------------------------------------------------
    m_cube    = Model::createCube(CUBE_SIZE);
    m_pyramid = Model::createPyramid(PYRAMID_BASE, PYRAMID_HEIGHT);
    m_axes    = Model::createAxes(AXES_LENGTH);
    m_pikachu = buildPikachu();
    m_raichu  = buildRaichu();

    // Modelo externo glTF (tecla 6); si el archivo no existe la aplicación
    // sigue funcionando con los modelos procedurales.
    loadGltfModel();

    // Quad de fondo en coordenadas NDC directas (no necesita matrices).
    // location 0 = posición NDC, location 1 = (u, v, 0).
    // La coordenada v está invertida porque OpenCV guarda la fila 0 arriba
    // y OpenGL muestrea v=0 abajo.
    const std::vector<float> quadVerts = {
        //  x     y    z      u    v    -
        -1.f, -1.f, 0.f,   0.f, 1.f, 0.f,
         1.f, -1.f, 0.f,   1.f, 1.f, 0.f,
         1.f,  1.f, 0.f,   1.f, 0.f, 0.f,
        -1.f,  1.f, 0.f,   0.f, 0.f, 0.f,
    };
    const std::vector<GLuint> quadIdx = {0, 1, 2, 0, 2, 3};
    m_backgroundQuad.create(quadVerts, quadIdx,
                            Model::VertexLayout::PositionColor, GL_TRIANGLES);

    // ---- Textura de fondo ------------------------------------------------------
    glGenTextures(1, &m_backgroundTexture);
    glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // ---- Matrices de modelo -----------------------------------------------------
    // El sistema del tablero (OpenCV) tiene +Z entrando en el tablero, por
    // lo que los modelos (construidos creciendo hacia +Z) se giran 180°
    // alrededor de X para que se eleven HACIA LA CÁMARA. La rotación también
    // orienta correctamente sus normales.
    const glm::vec3 boardCenter(
        (config::BOARD_COLS - 1) * 0.5f * config::SQUARE_SIZE,
        (config::BOARD_ROWS - 1) * 0.5f * config::SQUARE_SIZE,
        0.0f);
    const glm::mat4 flip = glm::rotate(glm::mat4(1.0f), glm::pi<float>(),
                                       glm::vec3(1.0f, 0.0f, 0.0f));

    m_boardCenterTransform = glm::translate(glm::mat4(1.0f), boardCenter) * flip;

    // ---- Estado global de OpenGL --------------------------------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    // Filas de la textura RGB alineadas a byte (ancho*3 no es múltiplo de 4).
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return true;
}

bool Renderer::shouldClose() const
{
    return m_window == nullptr || glfwWindowShouldClose(m_window);
}

void Renderer::pollEvents()
{
    glfwPollEvents();
}

void Renderer::requestClose()
{
    if (m_window)
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Renderer::uploadBackground(const cv::Mat& frameBGR)
{
    // OpenCV entrega BGR; se convierte a RGB, formato estándar de OpenGL.
    cv::cvtColor(frameBGR, m_rgbBuffer, cv::COLOR_BGR2RGB);
    if (!m_rgbBuffer.isContinuous())
        m_rgbBuffer = m_rgbBuffer.clone();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);

    // La textura se reserva una vez; los frames siguientes solo actualizan
    // los texels (TexSubImage2D es más barato que TexImage2D).
    if (m_texWidth != m_rgbBuffer.cols || m_texHeight != m_rgbBuffer.rows) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                     m_rgbBuffer.cols, m_rgbBuffer.rows, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, m_rgbBuffer.data);
        m_texWidth  = m_rgbBuffer.cols;
        m_texHeight = m_rgbBuffer.rows;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        m_rgbBuffer.cols, m_rgbBuffer.rows,
                        GL_RGB, GL_UNSIGNED_BYTE, m_rgbBuffer.data);
    }
}

void Renderer::drawBackground()
{
    // El fondo no participa en el test de profundidad: se dibuja primero y
    // los objetos 3D siempre quedan delante.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    m_backgroundShader.use();
    m_backgroundShader.setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);
    m_backgroundQuad.draw();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::beginFrame(const cv::Mat& frameBGR)
{
    // En pantallas HiDPI (Retina) el framebuffer es mayor que la ventana:
    // el viewport debe usar el tamaño REAL del framebuffer.
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    uploadBackground(frameBGR);
    drawBackground();
}

void Renderer::drawCharacter(const std::vector<ModelPart>& parts,
                             const glm::mat4& baseModel)
{
    // El shader iluminado ya está activo con view/projection/luz fijados.
    // Cada pieza aporta su transformación local (respecto al centro del
    // personaje) y su color base.
    for (const ModelPart& part : parts) {
        m_modelShader.setMat4("uModel", baseModel * part.localTransform);
        m_modelShader.setVec3("uObjectColor", part.color);
        part.mesh.draw();
    }
}

void Renderer::drawScene(const glm::mat4& view, const glm::mat4& projection,
                         SceneModel model)
{
    // ---- Ejes XYZ en el origen del tablero ----------------------------------
    m_axesShader.use();
    m_axesShader.setMat4("uModel", glm::mat4(1.0f));
    m_axesShader.setMat4("uView", view);
    m_axesShader.setMat4("uProjection", projection);
    glLineWidth(3.0f);   // puede ser ignorado en core profile (ancho = 1)
    m_axes.draw();

    // ---- Modelos iluminados ---------------------------------------------------
    m_modelShader.use();
    m_modelShader.setMat4("uView", view);
    m_modelShader.setMat4("uProjection", projection);
    // Luz direccional fija en el espacio del MUNDO (sistema del tablero):
    // llega inclinada desde -Z (el lado de la cámara), como un foco cenital.
    m_modelShader.setVec3("uLightDir",
                          glm::normalize(glm::vec3(0.4f, 0.3f, -1.0f)));

    switch (model) {
        case SceneModel::Cube:
            m_modelShader.setMat4("uModel", m_boardCenterTransform);
            m_modelShader.setVec3("uObjectColor", glm::vec3(0.9f, 0.1f, 0.1f)); // rojo
            m_cube.draw();
            break;

        case SceneModel::Pyramid:
            m_modelShader.setMat4("uModel", m_boardCenterTransform);
            m_modelShader.setVec3("uObjectColor", glm::vec3(0.1f, 0.8f, 0.15f)); // verde
            m_pyramid.draw();
            break;

        case SceneModel::Pikachu:
            drawCharacter(m_pikachu, m_boardCenterTransform);
            break;

        case SceneModel::Raichu:
            drawCharacter(m_raichu, m_boardCenterTransform);
            break;

        case SceneModel::Both: {
            // Pikachu a la izquierda, Raichu a la derecha del centro.
            const glm::mat4 left =
                glm::translate(glm::mat4(1.0f), glm::vec3(-CHARACTER_OFFSET, 0.f, 0.f))
                * m_boardCenterTransform;
            const glm::mat4 right =
                glm::translate(glm::mat4(1.0f), glm::vec3(+CHARACTER_OFFSET, 0.f, 0.f))
                * m_boardCenterTransform;
            drawCharacter(m_pikachu, left);
            drawCharacter(m_raichu,  right);
            break;
        }

        case SceneModel::Gltf:
            if (!m_hasGltf) {
                // Sin archivo glTF: se muestra el cubo como aviso visual.
                m_modelShader.setMat4("uModel", m_boardCenterTransform);
                m_modelShader.setVec3("uObjectColor", glm::vec3(0.9f, 0.1f, 0.1f));
                m_cube.draw();
                break;
            }
            // El modelo externo usa su propio shader (luz + textura).
            m_texturedShader.use();
            m_texturedShader.setMat4("uView", view);
            m_texturedShader.setMat4("uProjection", projection);
            m_texturedShader.setVec3("uLightDir",
                                     glm::normalize(glm::vec3(0.4f, 0.3f, -1.0f)));
            m_texturedShader.setMat4("uModel", m_boardCenterTransform);
            m_texturedShader.setInt("uTexture", 1);   // unidad 1 (la 0 es el fondo)
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, m_gltfTexture);
            m_gltfModel.draw();
            glActiveTexture(GL_TEXTURE0);
            break;
    }
}

void Renderer::endFrame()
{
    // Double buffering: se dibuja en el back buffer y se intercambia de
    // golpe, evitando el parpadeo (flickering) del single buffering.
    glfwSwapBuffers(m_window);
}

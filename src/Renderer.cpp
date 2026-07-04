// ============================================================================
//  Renderer.cpp
//  Implementación de la ventana, el fondo de cámara y la escena 3D.
// ============================================================================
#include "Renderer.h"
#include "Config.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

namespace {

// Dimensiones de los modelos, expresadas en cuadrados del tablero para que
// escalen automáticamente si cambia SQUARE_SIZE.
constexpr float CUBE_SIZE      = 2.0f * config::SQUARE_SIZE;
constexpr float PYRAMID_BASE   = 2.5f * config::SQUARE_SIZE;
constexpr float PYRAMID_HEIGHT = 2.5f * config::SQUARE_SIZE;
constexpr float AXES_LENGTH    = 3.0f * config::SQUARE_SIZE;

// Separación entre modelos cuando se muestran ambos (tecla 3).
constexpr float SIDE_OFFSET    = 2.2f * config::SQUARE_SIZE;

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

} // namespace

Renderer::~Renderer()
{
    if (m_backgroundTexture)
        glDeleteTextures(1, &m_backgroundTexture);
    if (m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
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

    // ---- Geometrías -----------------------------------------------------------
    m_cube    = Model::createCube(CUBE_SIZE);
    m_pyramid = Model::createPyramid(PYRAMID_BASE, PYRAMID_HEIGHT);
    m_axes    = Model::createAxes(AXES_LENGTH);

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

void Renderer::drawScene(const glm::mat4& view, const glm::mat4& projection,
                         bool showCube, bool showPyramid)
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

    // Si solo hay un modelo se centra; con ambos, se separan sobre el eje X.
    const glm::vec3 cubeOffset    = (showCube && showPyramid)
                                    ? glm::vec3(-SIDE_OFFSET, 0.f, 0.f)
                                    : glm::vec3(0.f);
    const glm::vec3 pyramidOffset = (showCube && showPyramid)
                                    ? glm::vec3(+SIDE_OFFSET, 0.f, 0.f)
                                    : glm::vec3(0.f);

    if (showCube) {
        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), cubeOffset) * m_boardCenterTransform;
        m_modelShader.setMat4("uModel", model);
        m_modelShader.setVec3("uObjectColor", glm::vec3(0.9f, 0.1f, 0.1f)); // rojo
        m_cube.draw();
    }
    if (showPyramid) {
        const glm::mat4 model =
            glm::translate(glm::mat4(1.0f), pyramidOffset) * m_boardCenterTransform;
        m_modelShader.setMat4("uModel", model);
        m_modelShader.setVec3("uObjectColor", glm::vec3(0.1f, 0.8f, 0.15f)); // verde
        m_pyramid.draw();
    }
}

void Renderer::endFrame()
{
    // Double buffering: se dibuja en el back buffer y se intercambia de
    // golpe, evitando el parpadeo (flickering) del single buffering.
    glfwSwapBuffers(m_window);
}

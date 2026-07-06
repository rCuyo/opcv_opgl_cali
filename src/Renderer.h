// ============================================================================
//  Renderer.h
//  Ventana GLFW + contexto OpenGL 3.3 core + render de la escena AR.
//
//  ¿Cómo se sincroniza la cámara real con OpenGL?
//  ----------------------------------------------
//  1) FONDO: cada frame de OpenCV (cv::Mat BGR) se sube a una textura y se
//     dibuja sobre un quad a pantalla completa, con el test de profundidad
//     desactivado para que quede siempre detrás.
//  2) CÁMARA VIRTUAL: la matriz de proyección se construye con los
//     intrínsecos K de la cámara real (misma focal y centro óptico) y la
//     matriz de vista sale de solvePnP (misma posición y orientación).
//     Con ambas matrices iguales a las de la cámara física, cualquier
//     punto 3D del tablero se proyecta en el MISMO píxel del render que
//     en la foto => el objeto virtual queda "pegado" al tablero.
//  3) MODELOS: se dibujan después del fondo, con test de profundidad, de
//     modo que se ocluyen correctamente entre sí.
// ============================================================================
#pragma once

#include "Model.h"
#include "Shader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <opencv2/core.hpp>
#include <functional>
#include <string>

struct GLFWwindow;   // forward declaration (evita incluir glfw3.h aquí)

/// Modelo 3D que se muestra anclado al tablero (teclas 1..5).
enum class SceneModel { Cube, Pyramid, Pikachu, Raichu, Both };

/// Una pieza coloreada de un personaje procedural (p. ej. la oreja de
/// Pikachu). Cada pieza es una malla propia con su transformación local
/// respecto al centro del personaje y su color base para el shader iluminado.
struct ModelPart
{
    Model     mesh;
    glm::mat4 localTransform{1.0f};
    glm::vec3 color{1.0f};
};

/// Ventana + contexto GL + dibujo del fondo de cámara y los modelos 3D.
class Renderer
{
public:
    using KeyCallback = std::function<void(int key)>;

    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    /// Crea la ventana (del tamaño del frame de cámara), el contexto
    /// OpenGL 3.3 core, carga los shaders y crea las geometrías.
    bool init(int width, int height, const std::string& title);

    /// Registra el callback de teclado (se invoca en pollEvents).
    void setKeyCallback(KeyCallback cb) { m_keyCallback = std::move(cb); }

    // ---- Ciclo de frame ----------------------------------------------------
    bool shouldClose() const;
    void pollEvents();

    /// Limpia los buffers y dibuja el frame de cámara como fondo.
    void beginFrame(const cv::Mat& frameBGR);

    /// Dibuja la escena 3D anclada al tablero.
    /// @param view       matriz de vista (de solvePnP, convención GL)
    /// @param projection matriz de proyección (de los intrínsecos K)
    /// @param model      modelo a mostrar (teclas 1..5)
    void drawScene(const glm::mat4& view, const glm::mat4& projection,
                   SceneModel model);

    /// Intercambia los buffers (double buffering => sin flickering).
    void endFrame();

    /// Solicita el cierre de la ventana (tecla ESC).
    void requestClose();

    /// Invocado por el callback estático de GLFW (uso interno).
    void dispatchKey(int key);

private:
    /// Sube el frame BGR de OpenCV a la textura de fondo.
    void uploadBackground(const cv::Mat& frameBGR);

    /// Dibuja el quad de fondo a pantalla completa.
    void drawBackground();

    GLFWwindow* m_window = nullptr;
    int m_width  = 0;   ///< tamaño del frame de cámara (coordenadas lógicas)
    int m_height = 0;

    // -- Fondo de cámara --
    Shader m_backgroundShader;
    Model  m_backgroundQuad;
    GLuint m_backgroundTexture = 0;
    int    m_texWidth = 0, m_texHeight = 0;

    // -- Escena 3D --
    Shader m_modelShader;   ///< iluminación básica (cubo, pirámide, personajes)
    Shader m_axesShader;    ///< color por vértice (ejes XYZ)
    Model  m_cube;
    Model  m_pyramid;
    Model  m_axes;

    // -- Personajes procedurales (listas de piezas coloreadas) --
    std::vector<ModelPart> m_pikachu;
    std::vector<ModelPart> m_raichu;

    /// Dibuja un personaje (lista de piezas) con el shader iluminado ya
    /// activo; baseModel sitúa y orienta al personaje sobre el tablero.
    void drawCharacter(const std::vector<ModelPart>& parts,
                       const glm::mat4& baseModel);

    /// Traslada al centro del tablero y voltea +Z del modelo hacia la cámara.
    glm::mat4 m_boardCenterTransform{1.0f};

    cv::Mat m_rgbBuffer;   ///< frame convertido a RGB (buffer reutilizado)

    KeyCallback m_keyCallback;
};

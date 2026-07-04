// ============================================================================
//  main.cpp
//  Aplicación de Realidad Aumentada basada en tablero de ajedrez.
//
//  Pipeline de cada frame:
//    1. Captura      : webcam o imagen         (Camera)
//    2. Rectificado  : eliminación de la distorsión de lente (Calibration)
//    3. Detección    : findChessboardCorners + cornerSubPix  (ChessboardDetector)
//    4. Pose         : solvePnP + suavizado                  (PoseEstimator)
//    5. Conversión   : rvec/tvec -> matrices de OpenGL       (MathUtils)
//    6. Render       : fondo de cámara + modelos 3D          (Renderer)
//
//  Controles:
//    ESC    salir                     1  mostrar cubo
//    R      reiniciar detección       2  mostrar pirámide
//    SPACE  congelar imagen           3  mostrar ambos
//    C      capturar vista de calibración
//    K      ejecutar calibración y guardarla en YAML
//
//  Uso:
//    ar_chessboard                     # webcam por defecto
//    ar_chessboard --camera 1          # otra webcam
//    ar_chessboard --image foto.jpg    # imagen estática
//    ar_chessboard --calib params.yml  # archivo de calibración alternativo
// ============================================================================
#include "Calibration.h"
#include "Camera.h"
#include "ChessboardDetector.h"
#include "Config.h"
#include "MathUtils.h"
#include "PoseEstimator.h"
#include "Renderer.h"

#include <GLFW/glfw3.h>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

namespace {

/// Estado mutable de la aplicación compartido con el callback de teclado.
struct AppState
{
    bool showCube        = true;    // tecla 1 (modelo por defecto)
    bool showPyramid     = false;   // tecla 2
    bool captureCalibView = false;  // tecla C: capturar vista en este frame
    bool runCalibration   = false;  // tecla K: calibrar en este frame
    bool resetRequested   = false;  // tecla R
};

/// Nombre legible del modelo activo para el HUD.
std::string currentModelName(const AppState& s)
{
    if (s.showCube && s.showPyramid) return "Cubo + Piramide";
    if (s.showCube)                  return "Cubo";
    if (s.showPyramid)               return "Piramide";
    return "Ninguno";
}

/// Dibuja el HUD (texto de estado) sobre el frame con OpenCV.
void drawHUD(cv::Mat& frame, double fps, bool boardFound, const Pose& pose,
             const AppState& state, const Calibration& calib, bool frozen)
{
    const auto color   = cv::Scalar(255, 255, 255);
    const auto shadow  = cv::Scalar(0, 0, 0);
    const int  font    = cv::FONT_HERSHEY_SIMPLEX;
    const double scale = 0.55;
    int y = 25;

    // putText con sombra para que sea legible sobre cualquier fondo.
    auto line = [&](const std::string& text, cv::Scalar c = cv::Scalar(255,255,255)) {
        cv::putText(frame, text, {11, y + 1}, font, scale, shadow, 2, cv::LINE_AA);
        cv::putText(frame, text, {10, y},     font, scale, c,      1, cv::LINE_AA);
        y += 24;
    };

    std::ostringstream ss;
    ss.precision(1);
    ss << std::fixed << "FPS: " << fps
       << "   Resolucion: " << frame.cols << "x" << frame.rows;
    line(ss.str());

    line(boardFound ? "Chessboard: DETECTADO" : "Chessboard: buscando...",
         boardFound ? cv::Scalar(80, 255, 80) : cv::Scalar(80, 80, 255));

    if (pose.valid) {
        std::ostringstream ps;
        ps.precision(2);
        ps << std::fixed << "Pose: t=(" << pose.tvec[0] << ", "
           << pose.tvec[1] << ", " << pose.tvec[2]
           << ")  dist=" << pose.distance();
        line(ps.str());
    } else {
        line("Pose: ---");
    }

    line("Modelo [1/2/3]: " + currentModelName(state));

    std::ostringstream cs;
    cs << "Calibracion: "
       << (calib.isCalibrated() ? "OK" : "APROXIMADA (pulsa C x"
                                         + std::to_string(config::MIN_CALIBRATION_FRAMES)
                                         + ", luego K)")
       << "   vistas: " << calib.viewCount();
    line(cs.str(), calib.isCalibrated() ? cv::Scalar(80, 255, 80)
                                        : cv::Scalar(80, 200, 255));

    if (frozen)
        line("[SPACE] IMAGEN CONGELADA", cv::Scalar(80, 200, 255));
}

/// Analiza los argumentos de línea de comandos.
struct CmdArgs
{
    int         cameraId = config::DEFAULT_CAMERA_ID;
    std::string imagePath;      // vacío => modo webcam
    std::string calibPath;      // vacío => archivo por defecto
};

CmdArgs parseArgs(int argc, char** argv)
{
    CmdArgs args;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--image"  && i + 1 < argc) args.imagePath = argv[++i];
        else if (arg == "--camera" && i + 1 < argc) args.cameraId = std::stoi(argv[++i]);
        else if (arg == "--calib"  && i + 1 < argc) args.calibPath = argv[++i];
        else if (arg == "--help") {
            std::cout << "Uso: ar_chessboard [--camera N] [--image ruta] "
                         "[--calib ruta.yml]" << std::endl;
            std::exit(0);
        }
    }
    return args;
}

} // namespace

int main(int argc, char** argv)
{
    const CmdArgs args = parseArgs(argc, argv);

    // ---- 1. Fuente de imagen ------------------------------------------------
    Camera camera;
    const bool opened = args.imagePath.empty()
                            ? camera.openWebcam(args.cameraId)
                            : camera.openImage(args.imagePath);
    if (!opened) {
        std::cerr << "No se pudo abrir la fuente de video/imagen." << std::endl;
        return 1;
    }
    const cv::Size frameSize = camera.frameSize();

    // ---- 2. Calibración ----------------------------------------------------
    const std::string calibFile = args.calibPath.empty()
                                      ? config::defaultCalibrationFile()
                                      : args.calibPath;
    Calibration calibration;
    if (!calibration.load(calibFile)) {
        std::cout << "Sin archivo de calibracion (" << calibFile
                  << "); se usan intrinsecos aproximados." << std::endl;
        calibration.setApproximate(frameSize);
    }

    // ---- 3. Detector y estimador de pose -------------------------------------
    ChessboardDetector detector(config::BOARD_COLS, config::BOARD_ROWS,
                                config::SQUARE_SIZE);
    PoseEstimator poseEstimator(config::POSE_SMOOTHING_ALPHA);

    // ---- 4. Ventana y OpenGL --------------------------------------------------
    Renderer renderer;
    if (!renderer.init(frameSize.width, frameSize.height,
                       "AR Chessboard - OpenCV + OpenGL"))
        return 1;

    // ---- 5. Teclado -------------------------------------------------------------
    AppState state;
    renderer.setKeyCallback([&](int key) {
        switch (key) {
            case GLFW_KEY_ESCAPE: renderer.requestClose();        break;
            case GLFW_KEY_SPACE:  camera.toggleFrozen();          break;
            case GLFW_KEY_R:      state.resetRequested = true;    break;
            case GLFW_KEY_1:      state.showCube = true;  state.showPyramid = false; break;
            case GLFW_KEY_2:      state.showCube = false; state.showPyramid = true;  break;
            case GLFW_KEY_3:      state.showCube = true;  state.showPyramid = true;  break;
            case GLFW_KEY_C:      state.captureCalibView = true;  break;
            case GLFW_KEY_K:      state.runCalibration   = true;  break;
            default: break;
        }
    });

    // solvePnP se ejecuta sobre el frame RECTIFICADO, así que la distorsión
    // efectiva es cero (la real ya fue eliminada por Calibration::undistort).
    const cv::Mat zeroDist = cv::Mat::zeros(1, 5, CV_64F);

    // Si el tablero se pierde unos pocos frames (parpadeo de detección) se
    // mantiene la última pose para evitar que el modelo aparezca y desaparezca.
    constexpr int GRACE_FRAMES = 15;
    int framesSinceSeen = GRACE_FRAMES;
    Pose lastPose;

    double fps = 0.0;
    auto lastTime = std::chrono::steady_clock::now();

    cv::Mat raw, frame;
    std::vector<cv::Point2f> corners;

    // =========================================================================
    //  Bucle principal
    // =========================================================================
    while (!renderer.shouldClose()) {
        // ---- FPS (media móvil exponencial) ----------------------------------
        const auto now = std::chrono::steady_clock::now();
        const double dt = std::chrono::duration<double>(now - lastTime).count();
        lastTime = now;
        if (dt > 0.0)
            fps = 0.9 * fps + 0.1 * (1.0 / dt);

        // ---- Peticiones de teclado pendientes -------------------------------
        if (state.resetRequested) {
            state.resetRequested = false;
            poseEstimator.reset();
            framesSinceSeen = GRACE_FRAMES;
            lastPose.valid  = false;
            camera.setFrozen(false);
            std::cout << "[App] Deteccion reiniciada" << std::endl;
        }

        // ---- Captura ----------------------------------------------------------
        if (!camera.grab(raw)) {
            std::cerr << "[App] No se pudo leer un frame" << std::endl;
            break;
        }

        // ---- Calibración en vivo ----------------------------------------------
        // Las vistas se capturan sobre el frame CRUDO: calibrateCamera debe
        // ver la distorsión real de la lente para poder estimarla.
        if (state.captureCalibView) {
            state.captureCalibView = false;
            std::vector<cv::Point2f> rawCorners;
            if (detector.detect(raw, rawCorners))
                calibration.addView(detector.objectPoints(), rawCorners);
            else
                std::cout << "[App] Tablero no visible: vista descartada"
                          << std::endl;
        }
        if (state.runCalibration) {
            state.runCalibration = false;
            if (calibration.viewCount() >= config::MIN_CALIBRATION_FRAMES) {
                if (calibration.calibrate(frameSize) >= 0.0) {
                    calibration.save(calibFile);
                    calibration.clearViews();
                    poseEstimator.reset();
                }
            } else {
                std::cout << "[App] Faltan vistas: "
                          << calibration.viewCount() << "/"
                          << config::MIN_CALIBRATION_FRAMES << std::endl;
            }
        }

        // ---- Rectificado + detección -------------------------------------------
        calibration.undistort(raw, frame);

        const bool found = detector.detect(frame, corners);
        if (found) {
            // solvePnP con distorsión cero (frame ya rectificado) y suavizado.
            lastPose = poseEstimator.estimate(detector.objectPoints(), corners,
                                              calibration.cameraMatrix(),
                                              zeroDist);
            framesSinceSeen = 0;
        } else {
            ++framesSinceSeen;
            if (framesSinceSeen > GRACE_FRAMES) {
                lastPose.valid = false;
                poseEstimator.reset();   // el filtro no debe mezclar rachas
            }
        }

        // ---- Anotaciones 2D (esquinas + HUD) --------------------------------------
        if (found)
            detector.drawCorners(frame, corners, found);
        drawHUD(frame, fps, found, lastPose, state, calibration,
                camera.isFrozen());

        // ---- Render -------------------------------------------------------------
        renderer.beginFrame(frame);

        if (lastPose.valid) {
            const glm::mat4 view = mathutils::buildViewMatrix(lastPose.rvec,
                                                              lastPose.tvec);
            const glm::mat4 proj = mathutils::buildProjectionMatrix(
                calibration.cameraMatrix(),
                frameSize.width, frameSize.height,
                config::NEAR_PLANE, config::FAR_PLANE);

            renderer.drawScene(view, proj, state.showCube, state.showPyramid);
        }

        renderer.endFrame();
        renderer.pollEvents();
    }

    std::cout << "[App] Fin" << std::endl;
    return 0;
}

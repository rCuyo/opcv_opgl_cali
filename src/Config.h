// ============================================================================
//  Config.h
//  Constantes globales de configuración del proyecto.
//
//  Aquí se centralizan los parámetros que el usuario puede querer modificar:
//  el tamaño del tablero, el tamaño de los cuadrados, rutas de recursos, etc.
// ============================================================================
#pragma once

#include <filesystem>
#include <string>

namespace config {

// ---------------------------------------------------------------------------
// Tablero de ajedrez (Chessboard)
// ---------------------------------------------------------------------------
// Número de ESQUINAS INTERNAS del patrón (no de cuadrados).
// Un tablero impreso de 10x7 cuadrados tiene 9x6 esquinas internas.
constexpr int   BOARD_COLS  = 9;      // esquinas internas horizontales
constexpr int   BOARD_ROWS  = 6;      // esquinas internas verticales

// Longitud del lado de cada cuadrado. La unidad es arbitraria (define la
// escala del mundo 3D). Si se usa 25.0 => milímetros reales; con 1.0 el
// mundo queda medido "en cuadrados", lo que simplifica dimensionar modelos.
constexpr float SQUARE_SIZE = 1.0f;

// ---------------------------------------------------------------------------
// Cámara / render
// ---------------------------------------------------------------------------
constexpr int   DEFAULT_CAMERA_ID = 0;      // índice de la webcam por defecto
constexpr float NEAR_PLANE        = 0.1f;   // plano cercano de la proyección
constexpr float FAR_PLANE         = 1000.f; // plano lejano de la proyección

// Suavizado de pose: factor de mezcla exponencial en [0,1].
// 1.0 = sin suavizado (pose cruda); valores bajos = más suave pero con lag.
constexpr float POSE_SMOOTHING_ALPHA = 0.5f;

// ---------------------------------------------------------------------------
// Calibración
// ---------------------------------------------------------------------------
// Número mínimo de vistas del tablero para ejecutar calibrateCamera().
constexpr int MIN_CALIBRATION_FRAMES = 10;

// Archivo por defecto donde se guardan/cargan los parámetros de cámara.
inline std::string defaultCalibrationFile();

// ---------------------------------------------------------------------------
// Recursos
// ---------------------------------------------------------------------------
// Devuelve la ruta a un recurso. Primero intenta la carpeta "resources"
// del directorio de trabajo (copiada por CMake junto al ejecutable); si no
// existe, usa la ruta del árbol de código definida por CMake (RESOURCE_DIR).
inline std::string resourcePath(const std::string& relative)
{
    namespace fs = std::filesystem;
    const fs::path local = fs::path("resources") / relative;
    if (fs::exists(local))
        return local.string();
#ifdef RESOURCE_DIR
    return (fs::path(RESOURCE_DIR) / relative).string();
#else
    return local.string();
#endif
}

inline std::string defaultCalibrationFile()
{
    return resourcePath("calibration/camera_params.yml");
}

} // namespace config

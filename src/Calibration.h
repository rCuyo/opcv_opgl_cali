// ============================================================================
//  Calibration.h
//  Calibración de la cámara con OpenCV.
//
//  La calibración estima:
//    - Camera Matrix (K): parámetros intrínsecos
//          | fx  0  cx |
//          |  0 fy  cy |     fx,fy = distancias focales en píxeles
//          |  0  0   1 |     cx,cy = centro óptico (punto principal)
//    - Distortion Coefficients: k1,k2,p1,p2,k3 (distorsión radial y
//      tangencial de la lente).
//
//  Proceso: se capturan varias vistas del tablero desde distintos ángulos.
//  Para cada vista se conocen los puntos 3D del tablero (objectPoints) y
//  sus proyecciones 2D detectadas (imagePoints). cv::calibrateCamera()
//  minimiza el error de reproyección sobre todas las vistas para hallar
//  K y los coeficientes de distorsión.
//
//  Los parámetros se guardan/cargan en YAML mediante cv::FileStorage.
// ============================================================================
#pragma once

#include <opencv2/core.hpp>
#include <string>
#include <vector>

/// Gestiona los parámetros intrínsecos de la cámara: calibración en vivo,
/// carga/guardado en YAML y valores aproximados de respaldo.
class Calibration
{
public:
    Calibration();

    // ---- Calibración en vivo -------------------------------------------
    /// Registra una vista del tablero (puntos 3D del modelo + esquinas 2D).
    void addView(const std::vector<cv::Point3f>& objectPoints,
                 const std::vector<cv::Point2f>& imagePoints);

    /// Ejecuta cv::calibrateCamera() con las vistas acumuladas.
    /// @param imageSize resolución de los frames usados
    /// @return RMS del error de reproyección, o un valor < 0 si falla
    double calibrate(const cv::Size& imageSize);

    /// Número de vistas acumuladas para calibrar.
    int  viewCount() const { return static_cast<int>(m_imagePoints.size()); }

    /// Elimina las vistas acumuladas (tecla R durante la captura).
    void clearViews();

    // ---- Persistencia ----------------------------------------------------
    /// Guarda K, distorsión, resolución y error RMS en un archivo YAML/XML.
    bool save(const std::string& path) const;

    /// Carga los parámetros desde un archivo YAML/XML.
    bool load(const std::string& path);

    // ---- Acceso a parámetros ---------------------------------------------
    /// true si hay parámetros válidos (calibrados o cargados de archivo).
    bool isCalibrated() const { return m_calibrated; }

    /// Si no hay calibración real, genera una aproximación razonable a
    /// partir de la resolución (FOV ~60 grados, sin distorsión). Permite
    /// usar la aplicación sin calibrar, con alineación aproximada.
    void setApproximate(const cv::Size& imageSize);

    const cv::Mat& cameraMatrix() const { return m_cameraMatrix; }
    const cv::Mat& distCoeffs()   const { return m_distCoeffs; }
    double         rmsError()     const { return m_rmsError; }

    /// Precalcula los mapas de rectificación (initUndistortRectifyMap) y
    /// elimina la distorsión del frame con cv::remap. Tras esto, el frame
    /// se corresponde con una cámara pinhole ideal de matriz K y distorsión
    /// cero: exactamente el modelo de cámara que usa OpenGL.
    void undistort(const cv::Mat& src, cv::Mat& dst);

private:
    cv::Mat m_cameraMatrix;   ///< matriz intrínseca K (3x3, CV_64F)
    cv::Mat m_distCoeffs;     ///< coeficientes de distorsión (1x5, CV_64F)
    double  m_rmsError   = -1.0;
    bool    m_calibrated = false;

    // Vistas acumuladas para calibrar.
    std::vector<std::vector<cv::Point3f>> m_objectPoints;
    std::vector<std::vector<cv::Point2f>> m_imagePoints;

    // Mapas de rectificación cacheados (se recalculan si cambia el tamaño).
    cv::Mat  m_undistortMap1, m_undistortMap2;
    cv::Size m_mapSize{0, 0};
};

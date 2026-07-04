// ============================================================================
//  PoseEstimator.h
//  Estimación de la pose del tablero con cv::solvePnP y suavizado temporal.
//
//  ¿Cómo se calcula la pose?
//  -------------------------
//  solvePnP resuelve el problema "Perspective-n-Point": dadas N
//  correspondencias entre puntos 3D del objeto (las esquinas del tablero en
//  su propio sistema de coordenadas, con Z=0) y sus proyecciones 2D en la
//  imagen, más los intrínsecos de la cámara (K, distorsión), calcula la
//  transformación rígida que lleva puntos del sistema del OBJETO al sistema
//  de la CÁMARA:
//
//        X_cam = R * X_obj + t
//
//  donde R se devuelve compacta como rvec (vector de Rodrigues: dirección =
//  eje de rotación, módulo = ángulo) y t como tvec. Al ser el tablero plano
//  y con >= 4 puntos, se usa el método IPPE (Infinitesimal Plane-based Pose
//  Estimation), especializado en objetos planos: más preciso y estable que
//  el método iterativo genérico para este caso.
//
//  Suavizado
//  ---------
//  El ruido de detección produce vibración ("jitter") en la pose. Se aplica
//  un filtro exponencial: la traslación se interpola linealmente (mix) y la
//  rotación se interpola esféricamente (slerp de cuaterniones), que es la
//  forma correcta de promediar rotaciones sin artefactos.
// ============================================================================
#pragma once

#include <opencv2/core.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

/// Resultado de una estimación de pose.
struct Pose
{
    cv::Vec3d rvec;        ///< rotación (Rodrigues), objeto -> cámara
    cv::Vec3d tvec;        ///< traslación, objeto -> cámara
    bool      valid = false;

    /// Distancia de la cámara al origen del tablero (norma de tvec).
    double distance() const { return cv::norm(tvec); }
};

/// Estima y suaviza la pose del tablero respecto de la cámara.
class PoseEstimator
{
public:
    /// @param smoothingAlpha factor de mezcla en (0,1]; 1 = sin suavizado
    explicit PoseEstimator(float smoothingAlpha);

    /// Calcula la pose con solvePnP y aplica el suavizado temporal.
    /// @param objectPoints puntos 3D del tablero (Z = 0)
    /// @param imagePoints  esquinas 2D detectadas (mismo orden)
    /// @param cameraMatrix intrínsecos K
    /// @param distCoeffs   distorsión (pasar ceros si el frame ya está
    ///                     rectificado con undistort)
    /// @return pose suavizada; .valid == false si solvePnP falló
    Pose estimate(const std::vector<cv::Point3f>& objectPoints,
                  const std::vector<cv::Point2f>& imagePoints,
                  const cv::Mat& cameraMatrix,
                  const cv::Mat& distCoeffs);

    /// Descarta el historial de suavizado (tecla R o tablero perdido).
    void reset();

    /// Última pose suavizada.
    const Pose& current() const { return m_smoothed; }

private:
    float m_alpha;             ///< peso de la medición nueva
    Pose  m_smoothed;          ///< estado del filtro
    bool  m_hasPrevious = false;

    glm::quat m_prevQuat{1.f, 0.f, 0.f, 0.f};  ///< rotación previa (filtro)
    glm::vec3 m_prevT{0.f};                    ///< traslación previa (filtro)
};

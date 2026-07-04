// ============================================================================
//  PoseEstimator.cpp
//  Implementación de solvePnP + filtro exponencial de pose.
// ============================================================================
#include "PoseEstimator.h"
#include "MathUtils.h"

#include <opencv2/calib3d.hpp>
#include <glm/gtc/quaternion.hpp>

PoseEstimator::PoseEstimator(float smoothingAlpha)
    : m_alpha(smoothingAlpha)
{
}

void PoseEstimator::reset()
{
    m_hasPrevious     = false;
    m_smoothed.valid  = false;
}

Pose PoseEstimator::estimate(const std::vector<cv::Point3f>& objectPoints,
                             const std::vector<cv::Point2f>& imagePoints,
                             const cv::Mat& cameraMatrix,
                             const cv::Mat& distCoeffs)
{
    cv::Vec3d rvec, tvec;

    // IPPE: método cerrado especializado en objetos PLANOS (todos los
    // puntos con Z=0), como nuestro tablero. Devuelve la solución con
    // menor error de reproyección de las dos físicamente posibles.
    bool ok = cv::solvePnP(objectPoints, imagePoints,
                           cameraMatrix, distCoeffs,
                           rvec, tvec,
                           false, cv::SOLVEPNP_IPPE);
    if (!ok) {
        m_smoothed.valid = false;
        return m_smoothed;
    }

    // ---- Suavizado temporal ------------------------------------------------
    // La rotación se pasa a cuaternión para interpolar con slerp; promediar
    // rvecs directamente no es correcto (el espacio de rotaciones no es
    // lineal) y produce artefactos cuando el ángulo cambia de signo.
    glm::quat qNew = mathutils::rvecToQuat(rvec);
    glm::vec3 tNew(static_cast<float>(tvec[0]),
                   static_cast<float>(tvec[1]),
                   static_cast<float>(tvec[2]));

    if (m_hasPrevious) {
        // Los cuaterniones q y -q representan la misma rotación: se alinea
        // el hemisferio para que slerp tome siempre el camino corto.
        if (glm::dot(m_prevQuat, qNew) < 0.0f)
            qNew = -qNew;

        qNew = glm::normalize(glm::slerp(m_prevQuat, qNew, m_alpha));
        tNew = glm::mix(m_prevT, tNew, m_alpha);
    }

    m_prevQuat    = qNew;
    m_prevT       = tNew;
    m_hasPrevious = true;

    // Se reconvierte el estado filtrado al formato rvec/tvec de OpenCV.
    m_smoothed.rvec  = mathutils::quatToRvec(qNew);
    m_smoothed.tvec  = cv::Vec3d(tNew.x, tNew.y, tNew.z);
    m_smoothed.valid = true;
    return m_smoothed;
}

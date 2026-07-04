// ============================================================================
//  MathUtils.cpp
//  Implementación de las conversiones OpenCV <-> OpenGL.
// ============================================================================
#include "MathUtils.h"

#include <opencv2/calib3d.hpp>   // cv::Rodrigues

namespace mathutils {

glm::mat4 buildViewMatrix(const cv::Vec3d& rvec, const cv::Vec3d& tvec)
{
    // 1) rvec (Rodrigues compacto) -> matriz de rotación 3x3.
    cv::Mat R;
    cv::Rodrigues(rvec, R);

    // 2) Se monta [R|t] aplicando a la vez el cambio de convención de ejes
    //    OpenCV -> OpenGL: F = diag(1,-1,-1,1). Multiplicar F * [R|t]
    //    equivale a NEGAR las filas 2 y 3 (índices 1 y 2) de R y de t:
    //
    //        V(gl) = |  r00  r01  r02  tx |
    //                | -r10 -r11 -r12 -ty |
    //                | -r20 -r21 -r22 -tz |
    //                |   0    0    0    1 |
    //
    // 3) GLM es column-major: view[col][fila]. cv::Mat es row-major:
    //    R.at(fila, col). El volcado de abajo hace la transposición
    //    de almacenamiento implícitamente.
    glm::mat4 view(1.0f);
    for (int row = 0; row < 3; ++row) {
        const double sign = (row == 0) ? 1.0 : -1.0;   // niega filas Y y Z
        for (int col = 0; col < 3; ++col)
            view[col][row] = static_cast<float>(sign * R.at<double>(row, col));
        view[3][row] = static_cast<float>(sign * tvec[row]);
    }
    return view;
}

glm::mat4 buildProjectionMatrix(const cv::Mat& cameraMatrix,
                                int width, int height,
                                float nearPlane, float farPlane)
{
    // Intrínsecos de la cámara real.
    const float fx = static_cast<float>(cameraMatrix.at<double>(0, 0));
    const float fy = static_cast<float>(cameraMatrix.at<double>(1, 1));
    const float cx = static_cast<float>(cameraMatrix.at<double>(0, 2));
    const float cy = static_cast<float>(cameraMatrix.at<double>(1, 2));
    const float w  = static_cast<float>(width);
    const float h  = static_cast<float>(height);
    const float n  = nearPlane;
    const float f  = farPlane;

    // Derivación (para un punto ya en coordenadas de cámara GL, es decir,
    // después de aplicar la matriz de vista con el flip Y/Z incluido):
    //
    //   Pinhole real:   u = fx * X_cv / Z_cv + cx      (píxeles, +v abajo)
    //   Relación GL:    X_cv = X_gl,  Y_cv = -Y_gl,  Z_cv = -Z_gl
    //
    //   NDC de OpenGL:  x_ndc = 2u/w - 1,   y_ndc = 1 - 2v/h
    //   Clip space:     x_clip = x_ndc * w_clip, con w_clip = -Z_gl
    //
    //   Igualando términos se obtiene la matriz (column-major en GLM):
    glm::mat4 proj(0.0f);
    proj[0][0] =  2.0f * fx / w;               // escala X (focal horizontal)
    proj[1][1] =  2.0f * fy / h;               // escala Y (focal vertical)
    proj[2][0] =  1.0f - 2.0f * cx / w;        // desplazamiento del centro óptico
    proj[2][1] =  2.0f * cy / h - 1.0f;        //   (cx,cy no suele ser el centro exacto)
    proj[2][2] = -(f + n) / (f - n);           // mapeo de profundidad estándar
    proj[2][3] = -1.0f;                        // w_clip = -Z_gl (divide por profundidad)
    proj[3][2] = -2.0f * f * n / (f - n);
    return proj;
}

glm::quat rvecToQuat(const cv::Vec3d& rvec)
{
    // El vector de Rodrigues codifica eje * ángulo; glm::angleAxis
    // construye el cuaternión equivalente.
    const double angle = cv::norm(rvec);
    if (angle < 1e-12)
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);   // identidad

    const glm::vec3 axis(static_cast<float>(rvec[0] / angle),
                         static_cast<float>(rvec[1] / angle),
                         static_cast<float>(rvec[2] / angle));
    return glm::angleAxis(static_cast<float>(angle), axis);
}

cv::Vec3d quatToRvec(const glm::quat& q)
{
    const float     angle = glm::angle(q);
    const glm::vec3 axis  = glm::axis(q);
    return cv::Vec3d(axis.x * angle, axis.y * angle, axis.z * angle);
}

} // namespace mathutils

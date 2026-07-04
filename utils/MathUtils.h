// ============================================================================
//  MathUtils.h
//  Conversiones entre los sistemas de coordenadas de OpenCV y OpenGL.
//
//  El puente OpenCV -> OpenGL
//  ==========================
//  OpenCV y OpenGL usan convenciones de cámara DISTINTAS:
//
//      OpenCV (visión):                 OpenGL (gráficos):
//        +X derecha                       +X derecha
//        +Y hacia ABAJO                   +Y hacia ARRIBA
//        +Z hacia DELANTE                 +Z hacia el ESPECTADOR
//        (la cámara mira +Z)              (la cámara mira -Z)
//
//  Pasar de una a otra equivale a rotar 180 grados alrededor del eje X,
//  es decir, multiplicar por la matriz  F = diag(1, -1, -1, 1).
//
//  * Matriz de VISTA:  solvePnP da [R|t] tal que X_cam(cv) = R*X_obj + t.
//    La vista de OpenGL es  V = F * [R|t]  (se niegan las filas 2 y 3).
//    Además GLM almacena las matrices por COLUMNAS, mientras cv::Mat las
//    almacena por FILAS, así que también hay que transponer el volcado.
//
//  * Matriz de PROYECCIÓN: se construye desde los intrínsecos K para que
//    la cámara virtual proyecte EXACTAMENTE igual que la cámara real:
//    un punto 3D debe caer en el mismo píxel en el render que en la foto.
//    Se derivan las ecuaciones de proyección pinhole (u = fx*X/Z + cx) y
//    se reescriben en coordenadas normalizadas de dispositivo (NDC).
// ============================================================================
#pragma once

#include <opencv2/core.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mathutils {

/// Construye la matriz de VISTA de OpenGL a partir de rvec/tvec de solvePnP.
/// Aplica el cambio de convención de ejes OpenCV -> OpenGL (ver cabecera).
glm::mat4 buildViewMatrix(const cv::Vec3d& rvec, const cv::Vec3d& tvec);

/// Construye la matriz de PROYECCIÓN de OpenGL a partir de la matriz
/// intrínseca K y la resolución de la imagen, de modo que la cámara
/// virtual reproduzca la perspectiva de la cámara real.
/// @param cameraMatrix K (3x3, CV_64F)
/// @param width,height resolución del frame en píxeles
/// @param nearPlane,farPlane planos de recorte de OpenGL
glm::mat4 buildProjectionMatrix(const cv::Mat& cameraMatrix,
                                int width, int height,
                                float nearPlane, float farPlane);

/// Convierte un vector de Rodrigues (OpenCV) en cuaternión (GLM).
glm::quat rvecToQuat(const cv::Vec3d& rvec);

/// Convierte un cuaternión (GLM) en vector de Rodrigues (OpenCV).
cv::Vec3d quatToRvec(const glm::quat& q);

} // namespace mathutils

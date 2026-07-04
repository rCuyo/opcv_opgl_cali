// ============================================================================
//  ChessboardDetector.cpp
//  Implementación de la detección del tablero.
// ============================================================================
#include "ChessboardDetector.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

ChessboardDetector::ChessboardDetector(int boardCols, int boardRows,
                                       float squareSize)
    : m_boardSize(boardCols, boardRows)
    , m_squareSize(squareSize)
{
    // Se construye el modelo 3D del tablero UNA sola vez.
    // findChessboardCorners devuelve las esquinas fila a fila empezando
    // por una esquina del patrón, así que generamos los puntos 3D en el
    // mismo orden: y = fila, x = columna, z = 0 (plano del tablero).
    m_objectPoints.reserve(static_cast<size_t>(boardCols) * boardRows);
    for (int row = 0; row < boardRows; ++row)
        for (int col = 0; col < boardCols; ++col)
            m_objectPoints.emplace_back(col * squareSize,
                                        row * squareSize,
                                        0.0f);
}

bool ChessboardDetector::detect(const cv::Mat& frame,
                                std::vector<cv::Point2f>& corners) const
{
    corners.clear();

    // La detección trabaja sobre escala de grises.
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // Flags:
    //  - ADAPTIVE_THRESH: umbral adaptativo, robusto ante iluminación variable.
    //  - NORMALIZE_IMAGE: ecualiza antes de umbralizar.
    //  - FAST_CHECK: descarta rápidamente frames sin tablero (clave para
    //    mantener el frame-rate cuando el patrón no está a la vista).
    const int flags = cv::CALIB_CB_ADAPTIVE_THRESH |
                      cv::CALIB_CB_NORMALIZE_IMAGE |
                      cv::CALIB_CB_FAST_CHECK;

    bool found = cv::findChessboardCorners(gray, m_boardSize, corners, flags);
    if (!found)
        return false;

    // Refinamiento subpíxel: ventana de búsqueda de 11x11 píxeles alrededor
    // de cada esquina; termina tras 30 iteraciones o cuando el desplazamiento
    // es menor a 0.1 px. Sin este paso la pose vibra apreciablemente.
    const cv::TermCriteria criteria(
        cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1);
    cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                     criteria);

    return true;
}

void ChessboardDetector::drawCorners(cv::Mat& frame,
                                     const std::vector<cv::Point2f>& corners,
                                     bool found) const
{
    // Utilidad estándar de OpenCV: dibuja círculos de colores unidos por
    // líneas, mostrando el orden de detección de las esquinas.
    cv::drawChessboardCorners(frame, m_boardSize, corners, found);
}

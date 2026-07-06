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

    // ---- 1) Detector "SB" (sector-based / transformada de Radon) ----------
    // Introducido en OpenCV 4, es MUCHO más robusto que el clásico ante:
    //   - perspectiva extrema (tablero visto casi de canto / muy lateral),
    //   - deformación proyectiva fuerte, desenfoque y baja iluminación.
    // Además ya devuelve esquinas con precisión subpíxel, así que no
    // necesita cornerSubPix. Se pide NORMALIZE (robustez a iluminación),
    // EXHAUSTIVE (búsqueda a fondo, clave en ángulos difíciles) y ACCURACY
    // (refinamiento fino de cada esquina).
    const int sbFlags = cv::CALIB_CB_NORMALIZE_IMAGE |
                        cv::CALIB_CB_EXHAUSTIVE |
                        cv::CALIB_CB_ACCURACY;
    if (cv::findChessboardCornersSB(gray, m_boardSize, corners, sbFlags) &&
        corners.size() == static_cast<size_t>(m_boardSize.area()))
        return true;

    // ---- 2) Fallback: detector clásico sobre imagen realzada con CLAHE ----
    // CLAHE (ecualización adaptativa con límite de contraste) levanta el
    // contraste local de tableros mal iluminados o en penumbra lateral,
    // dando una segunda oportunidad cuando SB no converge.
    cv::Mat enhanced;
    cv::createCLAHE(2.0, cv::Size(8, 8))->apply(gray, enhanced);

    corners.clear();
    const int flags = cv::CALIB_CB_ADAPTIVE_THRESH |
                      cv::CALIB_CB_NORMALIZE_IMAGE;   // sin FAST_CHECK: no descartar
    if (!cv::findChessboardCorners(enhanced, m_boardSize, corners, flags))
        return false;

    // Refinamiento subpíxel del método clásico (SB ya lo trae incorporado).
    const cv::TermCriteria criteria(
        cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1);
    cv::cornerSubPix(enhanced, corners, cv::Size(11, 11), cv::Size(-1, -1),
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

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

    const cv::TermCriteria subpixCriteria(
        cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1);

    // ---- 1) Detector clásico con FAST_CHECK (caso normal, ~3-4 ms) --------
    // Es el camino rápido: con el tablero razonablemente de frente lo
    // encuentra casi siempre, y FAST_CHECK descarta en ~20 ms los frames
    // sin tablero en vez de gastar cientos de ms buscando a fondo.
    // Medido a 1280x720: clásico = 2.5 ms; SB+EXHAUSTIVE = 133-215 ms/frame,
    // que limitaba TODO el programa a <8 FPS.
    const int fastFlags = cv::CALIB_CB_ADAPTIVE_THRESH |
                          cv::CALIB_CB_NORMALIZE_IMAGE |
                          cv::CALIB_CB_FAST_CHECK;
    if (cv::findChessboardCorners(gray, m_boardSize, corners, fastFlags)) {
        // El clásico necesita refinamiento subpíxel explícito.
        cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                         subpixCriteria);
        return true;
    }

    // ---- 2) Fallback robusto: SB a media resolución (~20-30 ms) -----------
    // El detector "SB" (sector-based, OpenCV >= 4) es mucho más robusto ante
    // perspectiva extrema, desenfoque y baja iluminación, pero es caro a
    // resolución completa. Ejecutarlo a media resolución (con CLAHE para
    // levantar el contraste en penumbra) conserva esa robustez a una
    // fracción del coste; la precisión se recupera después refinando las
    // esquinas a resolución completa con cornerSubPix.
    cv::Mat half;
    cv::resize(gray, half, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
    cv::createCLAHE(2.0, cv::Size(8, 8))->apply(half, half);

    corners.clear();
    if (!cv::findChessboardCornersSB(half, m_boardSize, corners,
                                     cv::CALIB_CB_NORMALIZE_IMAGE) ||
        corners.size() != static_cast<size_t>(m_boardSize.area()))
        return false;

    // Las esquinas se detectaron a mitad de escala: se llevan a resolución
    // completa y se refinan sobre la imagen original.
    for (cv::Point2f& c : corners)
        c *= 2.0f;
    cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                     subpixCriteria);

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

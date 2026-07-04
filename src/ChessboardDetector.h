// ============================================================================
//  ChessboardDetector.h
//  Detección del patrón de tablero de ajedrez con OpenCV.
//
//  ¿Cómo funciona la detección?
//  ----------------------------
//  1) cv::findChessboardCorners() busca en la imagen el patrón de
//     esquinas internas (intersecciones entre cuadrados blancos y negros).
//     Internamente binariza la imagen, localiza cuadriláteros negros y
//     agrupa sus esquinas en una rejilla del tamaño solicitado
//     (BOARD_COLS x BOARD_ROWS). Devuelve las esquinas ordenadas
//     fila a fila, lo que permite asociarlas 1:1 con puntos 3D conocidos.
//
//  2) cv::cornerSubPix() refina cada esquina con precisión subpíxel:
//     itera sobre el gradiente de la imagen alrededor de cada esquina
//     hasta converger al punto de silla exacto. Este refinamiento es
//     crítico para que la calibración y la pose sean estables (sin él,
//     el objeto 3D "vibraría" sobre el tablero).
// ============================================================================
#pragma once

#include <opencv2/core.hpp>
#include <vector>

/// Detecta las esquinas internas de un tablero de ajedrez en un frame.
class ChessboardDetector
{
public:
    /// @param boardCols  esquinas internas horizontales (p. ej. 9)
    /// @param boardRows  esquinas internas verticales   (p. ej. 6)
    /// @param squareSize longitud del lado de un cuadrado (unidades de mundo)
    ChessboardDetector(int boardCols, int boardRows, float squareSize);

    /// Busca el tablero en el frame BGR.
    /// @param frame    imagen de entrada (color BGR)
    /// @param corners  [out] esquinas detectadas en píxeles, orden fila a fila
    /// @return true si se encontró el patrón completo
    bool detect(const cv::Mat& frame, std::vector<cv::Point2f>& corners) const;

    /// Dibuja las esquinas detectadas sobre el frame (visualización).
    void drawCorners(cv::Mat& frame,
                     const std::vector<cv::Point2f>& corners,
                     bool found) const;

    /// Devuelve los puntos 3D del tablero en su propio sistema de
    /// coordenadas: la esquina interna superior-izquierda es el origen,
    /// X avanza por columnas, Y por filas y Z = 0 (el tablero es plano).
    /// Estos puntos se corresponden 1:1 con las esquinas 2D detectadas.
    const std::vector<cv::Point3f>& objectPoints() const { return m_objectPoints; }

    cv::Size boardSize()  const { return m_boardSize; }
    float    squareSize() const { return m_squareSize; }

private:
    cv::Size m_boardSize;                       ///< (cols, rows) internas
    float    m_squareSize;                      ///< lado del cuadrado
    std::vector<cv::Point3f> m_objectPoints;    ///< modelo 3D del tablero
};

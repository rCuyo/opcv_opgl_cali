// ============================================================================
//  Camera.h
//  Fuente de imágenes de la aplicación.
//
//  Abstrae dos modos de captura detrás de la misma interfaz:
//    - Webcam en vivo (cv::VideoCapture)
//    - Imagen estática cargada desde disco (cv::imread)
//
//  De esta forma el resto del programa no necesita saber de dónde vienen
//  los frames (principio de inversión de dependencias).
// ============================================================================
#pragma once

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <string>

/// Fuente de frames: webcam o imagen estática.
class Camera
{
public:
    Camera() = default;
    ~Camera();

    // No copiable: posee un recurso de hardware (VideoCapture).
    Camera(const Camera&)            = delete;
    Camera& operator=(const Camera&) = delete;

    /// Abre la webcam con el índice indicado. Devuelve false si falla.
    bool openWebcam(int deviceId);

    /// Carga una imagen desde disco que se devolverá en cada grab().
    bool openImage(const std::string& path);

    /// Obtiene el siguiente frame en color BGR (formato nativo de OpenCV).
    /// En modo imagen devuelve siempre una copia de la misma imagen.
    /// Devuelve false si no se pudo leer un frame.
    bool grab(cv::Mat& frame);

    /// Congela / descongela la captura. Mientras está congelada, grab()
    /// devuelve el último frame capturado (útil para inspeccionar la escena).
    void   setFrozen(bool frozen) { m_frozen = frozen; }
    bool   isFrozen() const       { return m_frozen; }
    void   toggleFrozen()         { m_frozen = !m_frozen; }

    bool      isOpen()  const { return m_isImage || m_capture.isOpened(); }
    bool      isImage() const { return m_isImage; }
    cv::Size  frameSize() const { return m_frameSize; }

private:
    cv::VideoCapture m_capture;          ///< captura de video (modo webcam)
    cv::Mat          m_staticImage;      ///< imagen fija (modo imagen)
    cv::Mat          m_lastFrame;        ///< último frame (para SPACE/freeze)
    cv::Size         m_frameSize{0, 0};  ///< resolución de la fuente
    bool             m_isImage = false;  ///< true si la fuente es una imagen
    bool             m_frozen  = false;  ///< true si la captura está congelada
};

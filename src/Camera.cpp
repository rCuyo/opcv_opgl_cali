// ============================================================================
//  Camera.cpp
//  Implementación de la fuente de imágenes (webcam o imagen estática).
// ============================================================================
#include "Camera.h"

#include <opencv2/imgcodecs.hpp>
#include <iostream>

Camera::~Camera()
{
    if (m_capture.isOpened())
        m_capture.release();
}

bool Camera::openWebcam(int deviceId)
{
    m_isImage = false;

    // CAP_ANY deja que OpenCV elija el backend nativo de cada plataforma
    // (AVFoundation en macOS, V4L2 en Linux, MSMF/DirectShow en Windows).
    if (!m_capture.open(deviceId, cv::CAP_ANY)) {
        std::cerr << "[Camera] No se pudo abrir la webcam con id "
                  << deviceId << std::endl;
        return false;
    }

    // Se solicita una resolución razonable; el driver puede ignorarla.
    m_capture.set(cv::CAP_PROP_FRAME_WIDTH,  1280);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    // Se captura un frame de prueba para conocer la resolución real.
    cv::Mat test;
    if (!m_capture.read(test) || test.empty()) {
        std::cerr << "[Camera] La webcam se abrió pero no entrega frames"
                  << std::endl;
        return false;
    }
    m_frameSize = test.size();
    m_lastFrame = test.clone();

    std::cout << "[Camera] Webcam " << deviceId << " abierta a "
              << m_frameSize.width << "x" << m_frameSize.height << std::endl;
    return true;
}

bool Camera::openImage(const std::string& path)
{
    m_staticImage = cv::imread(path, cv::IMREAD_COLOR);
    if (m_staticImage.empty()) {
        std::cerr << "[Camera] No se pudo cargar la imagen: " << path
                  << std::endl;
        return false;
    }
    m_isImage   = true;
    m_frameSize = m_staticImage.size();
    m_lastFrame = m_staticImage.clone();

    std::cout << "[Camera] Imagen cargada (" << m_frameSize.width << "x"
              << m_frameSize.height << "): " << path << std::endl;
    return true;
}

bool Camera::grab(cv::Mat& frame)
{
    // Con la captura congelada (tecla SPACE) se reutiliza el último frame.
    if (m_frozen) {
        if (m_lastFrame.empty())
            return false;
        m_lastFrame.copyTo(frame);
        return true;
    }

    if (m_isImage) {
        // La imagen se copia para que las anotaciones (esquinas, HUD)
        // no se acumulen frame a frame sobre el original.
        m_staticImage.copyTo(frame);
        m_lastFrame = m_staticImage;
        return true;
    }

    if (!m_capture.isOpened())
        return false;

    if (!m_capture.read(frame) || frame.empty())
        return false;

    m_lastFrame = frame.clone();
    return true;
}

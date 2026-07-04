// ============================================================================
//  Calibration.cpp
//  Implementación de la calibración de cámara y su persistencia YAML.
// ============================================================================
#include "Calibration.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>

Calibration::Calibration()
{
    // Valores neutros hasta calibrar o cargar de archivo.
    m_cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    m_distCoeffs   = cv::Mat::zeros(1, 5, CV_64F);
}

void Calibration::addView(const std::vector<cv::Point3f>& objectPoints,
                          const std::vector<cv::Point2f>& imagePoints)
{
    m_objectPoints.push_back(objectPoints);
    m_imagePoints.push_back(imagePoints);
    std::cout << "[Calibration] Vista capturada (" << viewCount() << ")"
              << std::endl;
}

void Calibration::clearViews()
{
    m_objectPoints.clear();
    m_imagePoints.clear();
}

double Calibration::calibrate(const cv::Size& imageSize)
{
    if (m_imagePoints.empty()) {
        std::cerr << "[Calibration] No hay vistas capturadas" << std::endl;
        return -1.0;
    }

    std::vector<cv::Mat> rvecs, tvecs;   // pose de cada vista (no se usan)

    // calibrateCamera minimiza la suma de errores de reproyección al
    // cuadrado sobre todas las vistas, optimizando simultáneamente K,
    // los coeficientes de distorsión y la pose de cada vista.
    m_rmsError = cv::calibrateCamera(m_objectPoints, m_imagePoints,
                                     imageSize,
                                     m_cameraMatrix, m_distCoeffs,
                                     rvecs, tvecs);

    m_calibrated = true;
    m_mapSize    = cv::Size(0, 0);   // invalida los mapas de undistort

    std::cout << "[Calibration] Calibrada con " << viewCount()
              << " vistas. RMS = " << m_rmsError << " px" << std::endl;
    std::cout << "[Calibration] K =\n" << m_cameraMatrix << std::endl;
    std::cout << "[Calibration] dist = " << m_distCoeffs << std::endl;

    return m_rmsError;
}

bool Calibration::save(const std::string& path) const
{
    cv::FileStorage fs(path, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        std::cerr << "[Calibration] No se pudo escribir: " << path << std::endl;
        return false;
    }

    // cv::FileStorage serializa cv::Mat automáticamente a YAML/XML.
    fs << "camera_matrix"           << m_cameraMatrix;
    fs << "distortion_coefficients" << m_distCoeffs;
    fs << "avg_reprojection_error"  << m_rmsError;
    fs.release();

    std::cout << "[Calibration] Parámetros guardados en " << path << std::endl;
    return true;
}

bool Calibration::load(const std::string& path)
{
    // Comprobación previa: evita el mensaje de error interno de OpenCV
    // cuando el archivo simplemente no existe todavía.
    if (!std::filesystem::exists(path))
        return false;

    cv::FileStorage fs(path, cv::FileStorage::READ);
    if (!fs.isOpened())
        return false;

    fs["camera_matrix"]           >> m_cameraMatrix;
    fs["distortion_coefficients"] >> m_distCoeffs;
    fs["avg_reprojection_error"]  >> m_rmsError;
    fs.release();

    if (m_cameraMatrix.empty() || m_cameraMatrix.rows != 3) {
        std::cerr << "[Calibration] Archivo inválido: " << path << std::endl;
        return false;
    }
    if (m_distCoeffs.empty())
        m_distCoeffs = cv::Mat::zeros(1, 5, CV_64F);

    m_cameraMatrix.convertTo(m_cameraMatrix, CV_64F);
    m_distCoeffs.convertTo(m_distCoeffs, CV_64F);

    m_calibrated = true;
    m_mapSize    = cv::Size(0, 0);

    std::cout << "[Calibration] Parámetros cargados de " << path
              << " (RMS previo = " << m_rmsError << ")" << std::endl;
    return true;
}

void Calibration::setApproximate(const cv::Size& imageSize)
{
    // Aproximación pinhole sin datos de calibración:
    //   - punto principal en el centro de la imagen
    //   - focal tal que el FOV horizontal sea ~60 grados:
    //         fx = (w/2) / tan(30 deg)  ≈ 0.866 * w
    const double fx = (imageSize.width / 2.0) / std::tan(30.0 * CV_PI / 180.0);

    m_cameraMatrix = (cv::Mat_<double>(3, 3) <<
        fx, 0., imageSize.width  / 2.0,
        0., fx, imageSize.height / 2.0,
        0., 0., 1.);
    m_distCoeffs = cv::Mat::zeros(1, 5, CV_64F);
    m_rmsError   = -1.0;
    m_calibrated = false;   // sigue sin ser una calibración real
    m_mapSize    = cv::Size(0, 0);

    std::cout << "[Calibration] Usando intrínsecos APROXIMADOS (fx = " << fx
              << "). Calibra con C/K para mayor precisión." << std::endl;
}

void Calibration::undistort(const cv::Mat& src, cv::Mat& dst)
{
    // Sin calibración real la distorsión es cero: no hay nada que corregir.
    if (!m_calibrated) {
        src.copyTo(dst);
        return;
    }

    // Los mapas solo dependen de K, la distorsión y el tamaño de imagen,
    // así que se calculan una vez y se reutilizan (remap es mucho más
    // rápido que cv::undistort frame a frame).
    if (m_mapSize != src.size()) {
        cv::initUndistortRectifyMap(m_cameraMatrix, m_distCoeffs,
                                    cv::Mat(),          // sin rectificación
                                    m_cameraMatrix,     // misma K de salida
                                    src.size(), CV_16SC2,
                                    m_undistortMap1, m_undistortMap2);
        m_mapSize = src.size();
    }

    cv::remap(src, dst, m_undistortMap1, m_undistortMap2, cv::INTER_LINEAR);
}

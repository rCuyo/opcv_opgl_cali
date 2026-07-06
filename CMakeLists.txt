# ============================================================================
#  Proyecto: Realidad Aumentada con Chessboard (OpenCV + OpenGL + C++17)
#
#  Dependencias:
#    - OpenCV 4.x   -> find_package (debe estar instalado en el sistema)
#    - GLFW 3.x     -> find_package; si no existe se descarga con FetchContent
#    - GLM          -> find_package; si no existe se descarga con FetchContent
#    - GLAD         -> incluido en external/glad (loader de funciones OpenGL)
#
#  Compila en Windows (MSVC/MinGW), Linux (GCC/Clang) y macOS (Clang).
# ============================================================================
cmake_minimum_required(VERSION 3.16)
project(ARChessboard LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

# ----------------------------------------------------------------------------
# OpenCV (obligatorio, instalado en el sistema)
# ----------------------------------------------------------------------------
find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs videoio calib3d)
message(STATUS "OpenCV encontrado: ${OpenCV_VERSION}")

# ----------------------------------------------------------------------------
# GLFW: primero se busca en el sistema; si no está, se descarga.
# ----------------------------------------------------------------------------
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW no encontrado en el sistema, descargando con FetchContent...")
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.9)
    FetchContent_MakeAvailable(glfw)
else()
    message(STATUS "GLFW encontrado en el sistema")
endif()

# ----------------------------------------------------------------------------
# GLM: primero se busca en el sistema; si no está, se descarga.
# ----------------------------------------------------------------------------
find_package(glm QUIET)
if(NOT glm_FOUND)
    message(STATUS "GLM no encontrado en el sistema, descargando con FetchContent...")
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1)
    FetchContent_MakeAvailable(glm)
else()
    message(STATUS "GLM encontrado en el sistema")
endif()

# Algunas distribuciones de GLM solo exportan directorios de include,
# sin el target moderno glm::glm. Se normaliza aquí.
if(NOT TARGET glm::glm)
    if(TARGET glm)
        add_library(glm::glm ALIAS glm)
    else()
        add_library(glm_headers INTERFACE)
        target_include_directories(glm_headers INTERFACE ${GLM_INCLUDE_DIRS})
        add_library(glm::glm ALIAS glm_headers)
    endif()
endif()

# ----------------------------------------------------------------------------
# GLAD: loader de funciones de OpenGL 3.3 core (incluido en el repositorio)
# ----------------------------------------------------------------------------
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# ----------------------------------------------------------------------------
# Ejecutable principal
# ----------------------------------------------------------------------------
add_executable(ar_chessboard
    src/main.cpp
    src/Camera.cpp
    src/ChessboardDetector.cpp
    src/Calibration.cpp
    src/PoseEstimator.cpp
    src/Renderer.cpp
    src/Shader.cpp
    src/Model.cpp
    utils/MathUtils.cpp
)

target_include_directories(ar_chessboard PRIVATE
    src
    utils
    external/cgltf              # parser glTF de cabecera única
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(ar_chessboard PRIVATE
    ${OpenCV_LIBS}
    glfw
    glad
    glm::glm
)

# En Linux, GLAD usa dlopen indirectamente a través de GLFW; se enlaza libdl.
if(UNIX AND NOT APPLE)
    target_link_libraries(ar_chessboard PRIVATE ${CMAKE_DL_LIBS})
endif()

# En macOS los dylibs de OpenCV usan install names @rpath/...: el ejecutable
# necesita el directorio de librerías de OpenCV en su rpath.
if(APPLE AND DEFINED OpenCV_INSTALL_PATH)
    set_target_properties(ar_chessboard PROPERTIES
        BUILD_RPATH "${OpenCV_INSTALL_PATH}/lib")
endif()

# ----------------------------------------------------------------------------
# Recursos: se define la ruta de recursos del árbol de código como fallback
# y además se copian junto al ejecutable para poder distribuirlo.
# ----------------------------------------------------------------------------
target_compile_definitions(ar_chessboard PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/resources"
    GLFW_INCLUDE_NONE)   # GLFW no debe incluir cabeceras OpenGL del sistema

add_custom_command(TARGET ar_chessboard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:ar_chessboard>/resources"
    COMMENT "Copiando recursos junto al ejecutable")

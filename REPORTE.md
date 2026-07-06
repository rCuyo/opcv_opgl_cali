# Reporte de estudio — AR Chessboard (OpenCV + OpenGL + C++17)

Documento de estudio del proyecto: cómo funciona el sistema de principio a
fin y **dónde está el código fundamental** que lo hace posible, con archivo
y línea exacta de cada pieza clave.

---

## 1. Idea general en una frase

Se detecta un tablero de ajedrez en la imagen de la cámara, se calcula
**dónde está la cámara respecto al tablero** (pose), y se configura una
cámara virtual de OpenGL **idéntica a la real** para que los objetos 3D se
proyecten exactamente sobre el tablero de la foto.

## 2. Pipeline de cada frame

```
 ┌─────────┐   ┌────────────┐   ┌───────────┐   ┌──────────┐   ┌────────────┐   ┌────────┐
 │ CAPTURA │ → │ RECTIFICADO │ → │ DETECCIÓN │ → │   POSE   │ → │ CONVERSIÓN │ → │ RENDER │
 │ Camera  │   │ Calibration │   │ Detector  │   │ solvePnP │   │ MathUtils  │   │Renderer│
 └─────────┘   └────────────┘   └───────────┘   └──────────┘   └────────────┘   └────────┘
   cv::Mat        sin distorsión    54 esquinas     rvec, tvec     matrices V y P    frame +
   BGR            de lente          subpíxel        suavizados     de OpenGL         objetos 3D
```

Este pipeline completo se orquesta en el **bucle principal**:
`src/main.cpp:218-313`.

---

## 3. Los 7 bloques de código fundamentales

> Sin cualquiera de estos bloques el proyecto NO funciona. Ordenados según
> el flujo de datos.

### Bloque 1 — Detección de esquinas (rápida primero, robusta después)
**`src/ChessboardDetector.cpp`**: detector clásico con `FAST_CHECK`
(camino rápido, ~4 ms) y fallback SB a media resolución (~25 ms)

```cpp
// 1) Camino rápido: clásico + FAST_CHECK (caso normal, tablero de frente).
if (cv::findChessboardCorners(gray, m_boardSize, corners, fastFlags)) {
    cv::cornerSubPix(gray, corners, ...);       // refinamiento subpíxel
    return true;
}
// 2) Fallback robusto: SB (sector-based) a MEDIA resolución + CLAHE,
//    para perspectiva extrema o poca luz; luego se reescala x2 y se
//    refina subpíxel sobre la imagen completa.
cv::resize(gray, half, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
cv::createCLAHE(2.0, cv::Size(8, 8))->apply(half, half);
if (!cv::findChessboardCornersSB(half, m_boardSize, corners, ...)) return false;
for (cv::Point2f& c : corners) c *= 2.0f;
cv::cornerSubPix(gray, corners, ...);
```

> Lección de rendimiento: `findChessboardCornersSB` con `EXHAUSTIVE` a
> resolución completa cuesta 133-215 ms/frame (≈5 FPS); el orden
> rápido-primero + fallback barato deja 3.7 ms con tablero (60 FPS con
> V-Sync) y 50 ms buscándolo.

- Ambos detectores devuelven las 9×6 esquinas **siempre en el mismo orden**
  (fila a fila). Ese orden es lo que permite emparejar cada esquina 2D con
  su punto 3D del tablero.
- La precisión **subpíxel** (nativa en SB, vía `cornerSubPix` en el
  fallback) **es lo que evita que el objeto 3D vibre**: sin ella, el error
  de ±1 px se traduce en jitter.

El modelo 3D del tablero (los puntos con los que se emparejan) se genera una
sola vez en el constructor: `src/ChessboardDetector.cpp:20-24` — puntos
`(col·S, fila·S, 0)`, porque el tablero es plano (Z=0).

### Bloque 2 — Calibración de cámara
**`src/Calibration.cpp:47`**

```cpp
m_rmsError = cv::calibrateCamera(m_objectPoints, m_imagePoints, imageSize,
                                 m_cameraMatrix, m_distCoeffs, rvecs, tvecs);
```

Con ≥10 vistas del tablero estima la **matriz intrínseca K** (focales fx,fy
y centro óptico cx,cy) y los **coeficientes de distorsión** de la lente.
K es el "ADN" de la cámara: todo lo demás (pose y proyección) depende de él.
- Guardado/carga YAML con `cv::FileStorage`: `src/Calibration.cpp:65` y `:88`.
- Eliminación de la distorsión (`initUndistortRectifyMap` + `remap`):
  `src/Calibration.cpp:148-156`. Tras esto el frame se comporta como una
  cámara pinhole ideal — el único modelo que OpenGL sabe reproducir.

### Bloque 3 — Estimación de pose
**`src/PoseEstimator.cpp:32`**

```cpp
bool ok = cv::solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs,
                       rvec, tvec, false, cv::SOLVEPNP_IPPE);
```

Resuelve: "¿qué rotación R y traslación t colocan los puntos 3D del tablero
de modo que se proyecten justo en las esquinas detectadas?". El resultado
define la relación `X_cam = R·X_obj + t`. `SOLVEPNP_IPPE` es el método
especializado en objetos planos.

**Suavizado anti-vibración** — `src/PoseEstimator.cpp:56-57`:

```cpp
qNew = glm::normalize(glm::slerp(m_prevQuat, qNew, m_alpha)); // rotación
tNew = glm::mix(m_prevT, tNew, m_alpha);                      // traslación
```

La rotación se interpola como **cuaternión (slerp)** porque promediar
ángulos linealmente es matemáticamente incorrecto.

### Bloque 4 — Matriz de VISTA (el puente OpenCV → OpenGL)
**`utils/MathUtils.cpp:29-36`**

```cpp
const double sign = (row == 0) ? 1.0 : -1.0;   // niega filas Y y Z
view[col][row] = static_cast<float>(sign * R.at<double>(row, col));
view[3][row]   = static_cast<float>(sign * tvec[row]);
```

Tres conceptos condensados en 4 líneas:
1. OpenCV mira hacia +Z con Y abajo; OpenGL mira hacia −Z con Y arriba.
   El cambio de convención es `V = diag(1,−1,−1,1) · [R|t]` → negar las
   filas 2ª y 3ª.
2. `cv::Mat` guarda por filas y GLM por columnas → el volcado
   `view[col][row] = R(row,col)` transpone el almacenamiento.
3. La traslación va en la 4ª columna (`view[3]`).

### Bloque 5 — Matriz de PROYECCIÓN desde K
**`utils/MathUtils.cpp:64-70`**

```cpp
proj[0][0] =  2.0f * fx / w;          // focal horizontal
proj[1][1] =  2.0f * fy / h;          // focal vertical
proj[2][0] =  1.0f - 2.0f * cx / w;   // centro óptico descentrado
proj[2][1] =  2.0f * cy / h - 1.0f;
proj[2][2] = -(f + n) / (f - n);      // profundidad estándar
proj[2][3] = -1.0f;                   // w_clip = -Z (división de perspectiva)
proj[3][2] = -2.0f * f * n / (f - n);
```

Es la ecuación pinhole real (`u = fx·X/Z + cx`) reescrita en coordenadas de
clip de OpenGL. **Esto es lo que hace que la cámara virtual tenga la misma
perspectiva que la webcam** (`glm::perspective` no serviría: no admite
centro óptico descentrado). Verificado: coincide con `cv::projectPoints`
con error < 0.0001 px.

### Bloque 6 — Geometría en GPU (OpenGL moderno)
**`src/Model.cpp:64-99`**

```cpp
glGenVertexArrays(1, &m_vao);                          // VAO: layout
glBufferData(GL_ARRAY_BUFFER, ..., GL_STATIC_DRAW);    // VBO: vértices
glBufferData(GL_ELEMENT_ARRAY_BUFFER, ...);            // EBO: índices
glVertexAttribPointer(0, 3, GL_FLOAT, ...);            // atributo posición
glVertexAttribPointer(1, 3, GL_FLOAT, ...);            // atributo normal/color
...
glDrawElements(m_primitive, m_indexCount, GL_UNSIGNED_INT, nullptr);
```

Los vértices se suben **una sola vez** a la GPU; cada frame solo se lanza el
draw call. El vertex shader hace la transformación completa
(`resources/shaders/model.vert`):

```glsl
gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
```

### Bloque 7 — Composición AR (fondo + 3D)
**`src/Renderer.cpp:315`** (subida del frame) y **`:325`** (orden de dibujo)

```cpp
glTexSubImage2D(GL_TEXTURE_2D, ...);   // el frame de OpenCV → textura GL
...
glDisable(GL_DEPTH_TEST);              // el fondo se dibuja SIN profundidad
```

El frame de la cámara se dibuja primero como quad a pantalla completa con el
test de profundidad apagado (siempre queda detrás); luego los modelos 3D con
profundidad activada. `glfwSwapBuffers` (`src/Renderer.cpp:425`) hace el
double buffering que elimina el flickering.

---

## 4. El bucle principal que une todo

**`src/main.cpp:218-313`** — el "esqueleto" del programa:

```cpp
while (!renderer.shouldClose()) {
    camera.grab(raw);                                     // 1. capturar
    calibration.undistort(raw, frame);                    // 2. rectificar
    bool found = detector.detect(frame, corners);         // 3. detectar
    if (found)
        lastPose = poseEstimator.estimate(...);           // 4. pose
    renderer.beginFrame(frame);                           // 5. fondo
    if (lastPose.valid) {
        glm::mat4 view = mathutils::buildViewMatrix(...); // 6. conversión
        glm::mat4 proj = mathutils::buildProjectionMatrix(...);
        renderer.drawScene(view, proj, state.model);      // 7. objetos 3D
                                       // teclas 1..5: Cubo, Piramide,
                                       // Pikachu, Raichu o ambos
    }
    renderer.endFrame();                                  // 8. swap
    renderer.pollEvents();                                //    teclado
}
```

Detalle de estabilidad: si la detección falla unos pocos frames se conserva
la última pose (`GRACE_FRAMES`, `src/main.cpp:205`) para que el modelo
no parpadee.

---

## 5. Mapa de archivos por importancia

| Prioridad | Archivo | Rol | Sin él... |
|---|---|---|---|
| ★★★ | `utils/MathUtils.cpp` | Puente OpenCV→OpenGL (V y P) | el objeto no se alinea con el tablero |
| ★★★ | `src/PoseEstimator.cpp` | solvePnP + suavizado | no se sabe dónde está el tablero |
| ★★★ | `src/ChessboardDetector.cpp` | Detección de esquinas | no hay nada que rastrear |
| ★★★ | `src/main.cpp` | Bucle y orquestación | nada se ejecuta |
| ★★☆ | `src/Renderer.cpp` | Ventana, fondo, escena | no hay imagen en pantalla |
| ★★☆ | `src/Calibration.cpp` | K + distorsión + YAML | alineación imprecisa (usa aproximados) |
| ★★☆ | `src/Model.cpp` + `shaders/` | Geometrías (cubo, pirámide, ejes, y primitivas elipsoide/cono/caja con las que se componen Pikachu y Raichu) y GLSL | no hay objetos 3D |
| ★☆☆ | `src/Shader.cpp`, `src/Camera.cpp` | Infraestructura | (reemplazables, poco algorítmicos) |
| ★☆☆ | `src/Config.h` | Constantes (9×6, suavizado...) | valores dispersos por el código |
| ★☆☆ | `external/glad/` | Carga de funciones OpenGL | no compila/arranca el render |

## 6. Conceptos clave para el examen/defensa

1. **¿Por qué un tablero de ajedrez?** Sus esquinas internas son puntos de
   silla detectables con precisión subpíxel y su geometría es conocida de
   antemano → correspondencias 3D↔2D perfectas y gratuitas.
2. **¿Qué es la pose?** La transformación rígida (R, t) del sistema de
   coordenadas del tablero al de la cámara. 6 grados de libertad.
3. **¿Por qué hay que "convertir" entre OpenCV y OpenGL?** Convenciones de
   ejes opuestas (Y abajo/arriba, Z delante/detrás) y almacenamiento de
   matrices opuesto (filas/columnas).
4. **¿Por qué no usar `glm::perspective`?** Porque la cámara real tiene un
   centro óptico que no está en el centro de la imagen y focales fx≠fy;
   la proyección debe construirse desde K.
5. **¿De dónde sale la estabilidad?** cornerSubPix (precisión) + slerp/mix
   (suavizado temporal) + periodo de gracia (continuidad) + V-Sync y double
   buffering (presentación sin parpadeo).

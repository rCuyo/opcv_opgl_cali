// ============================================================================
// model.frag - Iluminación básica (ambiente + difusa Lambert + borde).
//
// Luz direccional fija definida en el espacio del mundo (el tablero).
// Suficiente para dar volumen al cubo y la pirámide sin necesidad de
// materiales complejos.
// ============================================================================
#version 330 core

in  vec3 vNormal;
in  vec3 vWorldPos;
out vec4 FragColor;

uniform vec3 uObjectColor;   // color base del modelo (rojo/verde)
uniform vec3 uLightDir;      // dirección DE la luz (normalizada, mundo)

void main()
{
    vec3 n = normalize(vNormal);

    // Componente ambiente: evita que las caras en sombra queden negras.
    float ambient = 0.40;

    // Luz principal (key): difusa de Lambert, máxima cuando la cara mira a la
    // luz. Da el volumen y las sombras propias del modelo.
    float key = max(dot(n, -uLightDir), 0.0) * 0.65;

    // Luz de relleno (fill): tenue y desde el lado opuesto. Evita que la cara
    // que mira a la cámara quede plana (solo ambiente) cuando el objeto se ve
    // de frente, sin eliminar el modelado de la luz principal.
    float fill = max(dot(n, uLightDir), 0.0) * 0.25;

    vec3 color = uObjectColor * (ambient + key + fill);
    FragColor = vec4(color, 1.0);
}

// ============================================================================
// model.vert - Transformación de vértices de los modelos 3D.
//
// gl_Position = P * V * M * vertice:
//   M (uModel)      : coloca el modelo sobre el tablero
//   V (uView)       : pose de la cámara real (derivada de solvePnP)
//   P (uProjection) : intrínsecos de la cámara real (derivada de K)
// ============================================================================
#version 330 core

layout (location = 0) in vec3 aPos;      // posición en espacio del modelo
layout (location = 1) in vec3 aNormal;   // normal en espacio del modelo

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;    // normal en espacio del mundo
out vec3 vWorldPos;  // posición en espacio del mundo (para especular)

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // Matriz normal: transpuesta de la inversa de la parte 3x3 del modelo.
    // Correcta incluso con escalas no uniformes en uModel.
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;

    gl_Position = uProjection * uView * worldPos;
}

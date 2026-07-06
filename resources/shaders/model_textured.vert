// ============================================================================
// model_textured.vert - Vértices de modelos glTF con textura.
// Igual que model.vert pero propaga además las coordenadas UV.
// ============================================================================
#version 330 core

layout (location = 0) in vec3 aPos;      // posición en espacio del modelo
layout (location = 1) in vec3 aNormal;   // normal en espacio del modelo
layout (location = 2) in vec2 aUV;       // coordenadas de textura (glTF)

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;    // normal en espacio del mundo
out vec2 vUV;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);

    // Matriz normal: correcta incluso con escalas no uniformes en uModel.
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vUV = aUV;

    gl_Position = uProjection * uView * worldPos;
}

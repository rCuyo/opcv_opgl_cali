// ============================================================================
// background.vert - Quad de fondo a pantalla completa.
// Las posiciones ya vienen en NDC (-1..1), no se aplican matrices.
// ============================================================================
#version 330 core

layout (location = 0) in vec3 aPos;   // posición en NDC
layout (location = 1) in vec3 aUV;    // coordenadas de textura en .xy

out vec2 vUV;

void main()
{
    vUV = aUV.xy;
    gl_Position = vec4(aPos, 1.0);
}

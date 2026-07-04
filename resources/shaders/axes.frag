// ============================================================================
// axes.frag - Color plano por vértice: X rojo, Y verde, Z azul.
// ============================================================================
#version 330 core

in  vec3 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}

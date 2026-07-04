// ============================================================================
// background.frag - Muestra el frame de la cámara (textura RGB).
// ============================================================================
#version 330 core

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture;   // frame de la cámara subido por OpenCV

void main()
{
    FragColor = vec4(texture(uTexture, vUV).rgb, 1.0);
}

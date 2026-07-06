// ============================================================================
// model_textured.frag - Iluminación básica sobre el color de la textura.
// Mismo esquema de luces que model.frag (ambiente + key + fill), pero el
// color base sale de la textura baseColor del glTF en vez de un uniform.
// ============================================================================
#version 330 core

in  vec3 vNormal;
in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture;   // textura baseColor del modelo glTF
uniform vec3      uLightDir;  // dirección DE la luz (normalizada, mundo)

void main()
{
    vec3 base = texture(uTexture, vUV).rgb;
    vec3 n = normalize(vNormal);

    float ambient = 0.45;                                // sombra no negra
    float key  = max(dot(n, -uLightDir), 0.0) * 0.60;    // luz principal
    float fill = max(dot(n,  uLightDir), 0.0) * 0.20;    // relleno tenue

    FragColor = vec4(base * (ambient + key + fill), 1.0);
}

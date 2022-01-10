#version 450

layout(set = 1, binding = 0) uniform sampler2D colorTexture;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(texture(colorTexture, fragTexCoords).rgb * fragColor, 1.0);
}
#version 330 core

in vec3 oColor;
in vec2 oUV;

out vec4 fragColor;

void main() {
    fragColor = vec4(oColor,1.0);
}
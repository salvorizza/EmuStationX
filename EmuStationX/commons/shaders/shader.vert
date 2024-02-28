#version 330 core

in ivec2 aPos;
in uvec2 aUV;
in uvec3 aColor;

uniform ivec2 uOffset;
uniform uvec2 uTopLeft;
uniform uvec2 uBottomRight;

out vec3 oColor;
out vec2 oUV;

void main() {
    ivec2 position = aPos;

    float xpos = (float(position.x) / 512) - 1.0;
    float ypos = 1.0 - (float(position.y) / 256);
    gl_Position = vec4(xpos, ypos, 0.0, 1.0);
    oColor = vec3(
        float(aColor.r) / 255,
        float(aColor.g) / 255,
        float(aColor.b) / 255
    );
    oUV = vec2(
        float(aUV.x) / 255,
        float(aUV.y) / 255
    );
}
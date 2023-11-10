#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

uniform mat4 uProjectionMatrix;

out vec4 oColor;
out vec2 oUV;

void main() {
    gl_Position = uProjectionMatrix * vec4(aPos.x, aPos.y, 0.0, 1.0);
    oColor = aColor;
    oUV = aUV;
}
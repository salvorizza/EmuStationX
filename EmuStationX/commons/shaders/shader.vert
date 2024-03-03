#version 450 core

 layout(location = 0) in ivec2 aPos;
 layout(location = 1) in uvec2 aUV;
 layout(location = 2) in uvec3 aColor;
 layout(location = 3) in uint aTextured;
 layout(location = 4) in uvec2 aClutUV;
 layout(location = 5) in uint aBPP;

uniform ivec2 uOffset;
uniform uvec2 uTopLeft;
uniform uvec2 uBottomRight;

out vec3 oColor;
out vec2 oUV;
flat out uint oTextured;
out vec2 oClutUV;
flat out uint oBPP;

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
        float(aUV.x),
        float(aUV.y)
    );
     oClutUV = vec2(
        float(aClutUV.x),
        float(aClutUV.y)
    );
    oTextured = aTextured;
    oBPP = aBPP;
}
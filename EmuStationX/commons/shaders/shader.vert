#version 450 core

layout(location = 0) in ivec2 aPos;
layout(location = 1) in uvec2 aUV;
layout(location = 2) in uvec3 aColor;
layout(location = 3) in uint aTextured;
layout(location = 4) in uvec2 aClutUV;
layout(location = 5) in uint aBPP;
layout(location = 6) in uint aSemiTransparency;

uniform ivec2 uOffset;
uniform uvec2 uTopLeft;
uniform uvec2 uBottomRight;

out vec3 oColor;
out vec2 oUV;
flat out uint oTextured;
out vec2 oClutUV;
flat out uint oBPP;
flat out uint oSemiTransparency;

vec2 mapPointToRange(ivec2 point, uvec2 topLeft, uvec2 bottomRight) {
    uvec2 range = bottomRight - topLeft;
    vec2 normalizedPoint = vec2(point - topLeft) / vec2(range);
    vec2 mappedPoint = vec2(normalizedPoint.x * 2.0 - 1.0, normalizedPoint.y * -2.0 + 1);
    return mappedPoint;
}

void main() {
    ivec2 position = aPos + uOffset;

    /*
    * float xpos = (float(position.x) / 320) - 1.0;
    * float ypos = 1.0 - (float(position.y) / 240);
    * gl_Position = vec4(xpos, ypos, 0.0, 1.0);
    */

    vec2 mapped = mapPointToRange(position,uvec2(0,0),uvec2(640,480));
    gl_Position = vec4(mapped.x,mapped.y,0,1);
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
    oSemiTransparency = aSemiTransparency;
}
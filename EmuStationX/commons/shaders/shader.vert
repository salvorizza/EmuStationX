#version 450 core

layout(location = 0) in ivec2 aPos;
layout(location = 1) in uvec2 aUV;
layout(location = 2) in uvec3 aColor;
layout(location = 3) in uint aTextured;
layout(location = 4) in uvec2 aClutUV;
layout(location = 5) in uint aBPP;
layout(location = 6) in uint aSemiTransparency;
layout(location = 7) in uint aDither;

out vec3 oColor;
out vec2 oUV;
flat out uint oTextured;
out vec2 oClutUV;
flat out uint oBPP;
flat out uint oSemiTransparency;
flat out uint oDither;

vec2 mapPointToRange(vec2 point, vec2 topLeft, vec2 bottomRight) {
    vec2 range = bottomRight - topLeft;
    vec2 normalizedPoint = (point - topLeft) / (range);
    vec2 mappedPoint = vec2(normalizedPoint.x * 2.0 - 1.0, normalizedPoint.y * -2.0 + 1);
    return mappedPoint;
}

void main() {
    vec2 position = vec2(aPos.x,aPos.y);

    vec2 mapped = mapPointToRange(position,vec2(0,0),vec2(1024,512));
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
    oDither = aDither;
}
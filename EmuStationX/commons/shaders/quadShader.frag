#version 450 core

layout(location = 0) in vec4 oColor;
layout(location = 1) in vec2 oUV;

uniform float uBorderWidth;
uniform float uAspectRatio;

out vec4 fragColor;

vec4 blend(vec4 src, vec4 append) {
    return vec4(src.rgb * (1.0 - append.a) + append.rgb * append.a,1.0 - (1.0 - src.a) * (1.0 - append.a));
}

float insideBox(vec2 v, vec4 pRect) {
    vec2 s = step(pRect.xy, v) - step(pRect.zw, v);
    return s.x * s.y;
}

vec4 drawRect(vec2 pos, vec4 rect, vec4 color) {
    vec4 result = color;
    
    result.a *= insideBox(pos, vec4(rect.xy, rect.xy+rect.zw));
    return result;
}

void main() {
    vec4 result = oColor * 0.9;

    float pos = uBorderWidth;
    float size = 1.0 - (uBorderWidth * 2.0);
    vec4 rect = vec4(vec2(pos,pos * uAspectRatio), vec2(size,size * uAspectRatio));
    result = blend(result, drawRect(oUV, rect, oColor));

    fragColor = result;
}
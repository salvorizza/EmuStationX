#version 450 core

in vec3 oColor;
in vec2 oUV;

layout(binding=0) uniform sampler2D uVRAM16;  
layout(binding=1) uniform sampler2D uVRAM8;   
layout(binding=2) uniform sampler2D uVRAM4;   

out vec4 fragColor;

void main() {
    vec4 color4 = texture2D(uVRAM4,oUV / vec2(4096.0,512.0));
    vec4 color8 = texture2D(uVRAM8,oUV);
    vec4 color16 = texture2D(uVRAM16,oUV);

    fragColor = vec4(oColor,1.0) + (color4 * 100);

}
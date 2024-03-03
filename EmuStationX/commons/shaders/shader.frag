#version 450 core

in vec3 oColor;
in vec2 oUV;
flat in uint oTextured;
in vec2 oClutUV;
flat in uint oBPP;

layout(binding=0) uniform usampler2D uVRAM16;  
layout(binding=1) uniform usampler2D uVRAM8;   
layout(binding=2) uniform usampler2D uVRAM4;   

out vec4 fragColor;

vec4 split_colors(uint data)
{
    vec4 color;
    color.r = (data << 3) & 0xf8;
    color.g = (data >> 2) & 0xf8;
    color.b = (data >> 7) & 0xf8;
    color.a = 255.0f;

    return color;
}

void main() {
    vec4 color = vec4(oColor,1.0);
    if(oTextured == 1) {
        vec2 size4 = textureSize(uVRAM4,0);
        vec2 size16 = textureSize(uVRAM16,0);

        vec2 uvIndex = oUV / size4;
        uvec4 index =  texture(uVRAM4,uvIndex);
        vec2 uvPalette = vec2(oClutUV.x + index.r,oClutUV.y) / size16;

        uvec4 paletteColor = texture(uVRAM16, uvPalette);
        //color = split_colors(paletteColor.r);
        color=vec4(float(index.r) / 16.0f,0,0,1);
    }

    fragColor = color;

}
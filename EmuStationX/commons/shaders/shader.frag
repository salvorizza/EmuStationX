#version 450 core

in vec3 oColor;
in vec2 oUV;
flat in uint oTextured;
in vec2 oClutUV;
flat in uint oBPP;
flat in uint oSemiTransparency;

layout(binding=0) uniform usampler2D uVRAM16;  
layout(binding=1) uniform usampler2D uVRAM8;   
layout(binding=2) uniform usampler2D uVRAM4;   
layout(binding=3) uniform sampler2D uVRAM;   

out vec4 fragColor;

#define BPP_4 4u
#define BPP_8 8u
#define BPP_16 16u

#define B2PlusF2 0u
#define BPlusF 1u
#define BMinusF 2u
#define BPlusF4 3u

vec4 from_15bit(uint data)
{
    vec4 color;
    color.r = ((data << 3) & 0xf8) / 255.0;
    color.g = ((data >> 2) & 0xf8) / 255.0;
    color.b = ((data >> 7) & 0xf8) / 255.0;

    if(data >= 0x8000u && (data >> 15) == 1u) {
        vec4 previousColor = texture(uVRAM,gl_FragCoord.xy / textureSize(uVRAM,0));

        switch(oSemiTransparency) {
            case B2PlusF2: {
                color = 0.5 * previousColor + 0.5 * color;
                break;
            }

            case BPlusF: {
                color = 1.0 * previousColor + 1.0 * color;
                break;
            }

            case BMinusF: {
                color = 1.0 * previousColor - 1.0 * color;
                break;
            }

            case BPlusF4: {
                color = 1.0 * previousColor + 0.25 * color;
                break;
            }
        }
    }

    color.a = 1.0;

    return color;
}

void main() {
    vec4 color = vec4(oColor,1.0);

    if(oTextured == 1) {
        vec2 size4 = textureSize(uVRAM4,0);
        vec2 size8 = textureSize(uVRAM8,0);
        vec2 size16 = textureSize(uVRAM16,0);
        vec2 uvColor = vec2(0,0);

        switch (oBPP) {
            case BPP_4: {
                vec2 uvIndex = oUV / size4;
                uvec4 index =  texture(uVRAM4,uvIndex);
                uvColor = vec2(oClutUV.x + index.r,oClutUV.y) / size16;
                break;
            }

            case BPP_8: {
                vec2 uvIndex = oUV / size8;
                uvec4 index =  texture(uVRAM8,uvIndex);
                uvColor = vec2(oClutUV.x + index.r,oClutUV.y) / size16;
                break;
            }

            case BPP_16: {
                uvColor = oUV / size16;
                break;
            }
        }

        
        uvec4 paletteColor = texture(uVRAM16, uvColor);
        
        if(paletteColor.r == 0u) discard;

        color = from_15bit(paletteColor.r);
    }

    fragColor = color;

}
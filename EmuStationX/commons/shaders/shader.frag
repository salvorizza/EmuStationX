#version 450 core

in vec3 oColor;
in vec2 oUV;
flat in uint oTextured;
in vec2 oClutUV;
flat in uint oBPP;
flat in uint oSemiTransparency;
flat in uint oDither;
flat in uint oRawTexture;

layout(binding=0) uniform sampler2D uVRAM;   

uniform int uCheckMask;
uniform int uForceAlpha;

out vec4 fragColor;

#define BPP_4 4u
#define BPP_8 8u
#define BPP_16 16u

#define B2PlusF2 0u
#define BPlusF 1u
#define BMinusF 2u
#define BPlusF4 3u

const mat4 dither = mat4(
    vec4(-4,+0,-3,+1),
    vec4(+2,-2,+3,-1),
    vec4(-3,+1,-4,+0),
    vec4(+3,-1,+2,-2)
);

int float_5bit(float value) {
    return int(round(value * 31.0 + 0.5));
}

vec4 sample_vram(ivec2 coords) {
    coords &= ivec2(1023,511);
    return texelFetch(uVRAM, coords, 0);
}

int sample_16bit(ivec2 coords) {
    vec4 color = sample_vram(coords);

    int r = float_5bit(color.r);
    int g = float_5bit(color.g);
    int b = float_5bit(color.b);
    int a = int(ceil(color.a));

    int data = (a << 15) | (b << 10) | (g << 5) | (r);

    return data;
}

int texel_8bit(ivec2 coords) {
    ivec2 vram_coords = ivec2(coords.x >> 1,511 - coords.y);
    int data = sample_16bit(vram_coords);
    int shift = (coords.x & 1) << 3;
    int texel = (data >> shift) & 0xFF;
    return texel;
}

int texel_4bit(ivec2 coords) {
    ivec2 vram_coords = ivec2(coords.x >> 2,511 - coords.y);
    int data = sample_16bit(vram_coords);
    int shift = (coords.x & 3) << 2;
    int texel = (data >> shift) & 0xF;
    return texel;
}

vec4 saturate_5bit(ivec4 color8) {
    vec4 result = vec4(0);

    color8.r = clamp(color8.r,0,255) >> 3;
    color8.g = clamp(color8.g,0,255) >> 3;
    color8.b = clamp(color8.b,0,255) >> 3;

    result.r = color8.r / 31.0;
    result.g = color8.g / 31.0;
    result.b = color8.b / 31.0;
    result.a = color8.a / 255.0;

    return result;
}

vec4 apply_dither(vec4 color) {
    vec4 result = vec4(0);

    ivec4 color8 = ivec4(color * 255);

    int x = int(mod(gl_FragCoord.x,4));
    int y = int(mod(gl_FragCoord.y,4));
    color8 += int(dither[x][y]);

    result = saturate_5bit(color8);

    return result;
}

vec4 blend_colors(vec4 background, vec4 foreground, uint blendFunc) {
    vec4 result = vec4(0);

    switch(blendFunc) {
        case B2PlusF2: {
            result = 0.5 * background + 0.5 * foreground;
            break;
        }

        case BPlusF: {
            result = 1.0 * background + 1.0 * foreground;
            break;
        }

        case BMinusF: {
            result = 1.0 * background - 1.0 * foreground;
            break;
        }

        case BPlusF4: {
            result = 1.0 * background + 0.25 * foreground;
            break;
        }

        default: {
            result = foreground;
            break;
        }
    }

    result = clamp(result,vec4(0),vec4(1));

    return result;
}

void main() {
    vec4 color = vec4(oColor,1.0);
    vec4 previousColor = texelFetch(uVRAM,ivec2(gl_FragCoord.xy),0);

    if(uCheckMask == 1 && previousColor.a == 1) discard;

    if(oTextured == 1) {
        ivec2 uvColor = ivec2(0,0);

        switch (oBPP) {
            case BPP_4: {
                uvColor = ivec2(oClutUV.x + texel_4bit(ivec2(oUV)),511 - oClutUV.y);  
                break;
            }

            case BPP_8: {
                uvColor = ivec2(oClutUV.x + texel_8bit(ivec2(oUV)),511 - oClutUV.y);  
                break;
            }

            case BPP_16: {
                uvColor = ivec2(oUV.x, 511 - oUV.y);
                break;
            }
        }
       
        color = sample_vram(uvColor);
        if(color == vec4(0,0,0,0)) discard;
        if(oRawTexture == 0u) {
            color = (color * vec4(oColor,1.0)) / (128.0 / 255.0);
            if(oDither == 1u) {
                //color = apply_dither(color);
            }
        }

        if(color.a > 0) {
            color = blend_colors(previousColor, color, oSemiTransparency);
        }
    } else {

        if(oDither == 1u) {
            color = apply_dither(color);
        } else {
            ivec4 color8 = ivec4(color * 255);
            color = saturate_5bit(color8);
        }
        color.a = 0;

        color = blend_colors(previousColor, color, oSemiTransparency);
    }

    if(uForceAlpha == 1) {
        color.a = 1;
    }

    fragColor = color;

}
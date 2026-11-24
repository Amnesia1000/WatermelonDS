/*
    Copyright 2016-2025 melonDS team

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#ifndef MAIN_SHADERS_H
#define MAIN_SHADERS_H

const char* kScreenVS = R"(#version 140

uniform vec2 uScreenSize;
uniform mat2x3 uTransform;

in vec2 vPosition;
in vec2 vTexcoord;

smooth out vec2 fTexcoord;

void main()
{
    vec4 fpos;

    fpos.xy = vec3(vPosition, 1.0) * uTransform;

    fpos.xy = ((fpos.xy * 2.0) / uScreenSize) - 1.0;
    fpos.y *= -1;
    fpos.z = 0.0;
    fpos.w = 1.0;

    gl_Position = fpos;
    fTexcoord = vTexcoord;
}
)";

const char* kScreenFS = R"(#version 140

uniform sampler2D ScreenTex;
uniform int uShaderMode;

smooth in vec2 fTexcoord;

out vec4 oColor;

vec3 fetchDS(vec2 uv)
{
    return texture(ScreenTex, uv).bgr;
}

void main()
{
    vec2 texSize = vec2(textureSize(ScreenTex, 0));

    if (uShaderMode == 1)
    {
        // CRT-Lottes style: scanlines + phosphor grille + warm contrast
        vec2 texel = fTexcoord * texSize;
        vec2 fl = floor(texel);
        vec2 fr = fract(texel);
        vec2 sharpUv = (fl + clamp((fr - 0.5) * 1.5 + 0.5, 0.0, 1.0)) / texSize;
        vec3 col = fetchDS(sharpUv);
        col = pow(col, vec3(1.15));

        float scan = sin(fr.y * 3.14159265);
        float scanWeight = mix(0.62, 1.05, pow(scan, 0.7));

        float mask = mod(gl_FragCoord.x, 3.0);
        vec3 grille = vec3(
            mask < 1.0 ? 1.04 : 0.90,
            (mask >= 1.0 && mask < 2.0) ? 1.04 : 0.90,
            mask >= 2.0 ? 1.04 : 0.90
        );

        oColor = vec4(clamp(col * scanWeight * grille * 1.08, 0.0, 1.0), 1.0);
    }
    else if (uShaderMode == 2)
    {
        // LCD3x: authentic Nintendo DS LCD pixel grid & RGB subpixel mask
        vec3 col = fetchDS(fTexcoord);
        vec2 texel = fTexcoord * texSize;
        vec2 fr = fract(texel);

        float borderX = smoothstep(0.0, 0.14, fr.x) * smoothstep(1.0, 0.86, fr.x);
        float borderY = smoothstep(0.0, 0.14, fr.y) * smoothstep(1.0, 0.86, fr.y);
        float grid = mix(0.70, 1.0, borderX * borderY);

        float sub = fr.x * 3.0;
        vec3 subMask = vec3(
            sub < 1.0 ? 1.05 : 0.92,
            (sub >= 1.0 && sub < 2.0) ? 1.05 : 0.92,
            sub >= 2.0 ? 1.05 : 0.92
        );

        oColor = vec4(clamp(col * grid * subMask * 1.06, 0.0, 1.0), 1.0);
    }
    else if (uShaderMode == 3)
    {
        // ScaleFx / Sharp Bilinear: crisp anti-aliased scaling
        vec2 texel = fTexcoord * texSize;
        vec2 fl = floor(texel - 0.5) + 0.5;
        vec2 f = texel - fl;
        vec2 w = fwidth(texel);
        vec2 alpha = clamp((f - 0.5) / max(w, vec2(0.001)) + 0.5, 0.0, 1.0);
        vec2 uv = (fl + alpha) / texSize;
        oColor = vec4(fetchDS(uv), 1.0);
    }
    else if (uShaderMode == 4)
    {
        // xBRZ-style edge-aware 2D pixel-art smoothing
        vec2 px = 1.0 / texSize;
        vec2 texel = fTexcoord * texSize;
        vec2 f = fract(texel);
        vec2 center = (floor(texel) + 0.5) * px;

        vec3 c  = fetchDS(center);
        vec3 n  = fetchDS(center + vec2(0.0, -px.y));
        vec3 s  = fetchDS(center + vec2(0.0,  px.y));
        vec3 w  = fetchDS(center + vec2(-px.x, 0.0));
        vec3 e  = fetchDS(center + vec2( px.x, 0.0));
        vec3 nw = fetchDS(center + vec2(-px.x, -px.y));
        vec3 ne = fetchDS(center + vec2( px.x, -px.y));
        vec3 sw = fetchDS(center + vec2(-px.x,  px.y));
        vec3 se = fetchDS(center + vec2( px.x,  px.y));

        vec3 res = c;
        if (f.x < 0.5 && f.y < 0.5 && distance(w, n) < 0.15 && distance(c, nw) > 0.2 && (0.5 - f.x) + (0.5 - f.y) > 0.5)
            res = mix(c, (w + n) * 0.5, 0.75);
        else if (f.x >= 0.5 && f.y < 0.5 && distance(e, n) < 0.15 && distance(c, ne) > 0.2 && (f.x - 0.5) + (0.5 - f.y) > 0.5)
            res = mix(c, (e + n) * 0.5, 0.75);
        else if (f.x < 0.5 && f.y >= 0.5 && distance(w, s) < 0.15 && distance(c, sw) > 0.2 && (0.5 - f.x) + (f.y - 0.5) > 0.5)
            res = mix(c, (w + s) * 0.5, 0.75);
        else if (f.x >= 0.5 && f.y >= 0.5 && distance(e, s) < 0.15 && distance(c, se) > 0.2 && (f.x - 0.5) + (f.y - 0.5) > 0.5)
            res = mix(c, (e + s) * 0.5, 0.75);

        oColor = vec4(res, 1.0);
    }
    else
    {
        oColor = vec4(fetchDS(fTexcoord), 1.0);
    }
}
)";

#endif // MAIN_SHADERS_H

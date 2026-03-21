#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    // Content rect in UV space (0-1) — only apply grading inside this rect
    float crLeft;
    float crTop;
    float crRight;
    float crBottom;

    // Color grading uniforms (all in UI-scale: -100..100 etc.)
    float brightness;   // -100 to 100
    float contrast;     // -100 to 100
    float saturation;   // -100 to 100
    float exposure;     // -100 to 100
    float temperature;  // -100 to 100
    float tintVal;      // -100 to 100
    float highlights;   // -100 to 100
    float shadows;      // -100 to 100
    float vibrance;     // -100 to 100
    float hueShift;     // -180 to 180
    float fade;         //    0 to 100
    float vignetteAmt;  //    0 to 100
};

layout(binding = 1) uniform sampler2D source;

void main() {
    vec2 uv = qt_TexCoord0;
    vec4 tex = texture(source, uv);

    // Check if this pixel is outside the video content rect — pass through unchanged
    if (uv.x < crLeft || uv.x > crRight || uv.y < crTop || uv.y > crBottom) {
        fragColor = tex * qt_Opacity;
        return;
    }

    vec3 c = tex.rgb;

    // Remap UV to content-local coordinates (0-1 within video area) for vignette
    vec2 contentUV = (uv - vec2(crLeft, crTop)) / vec2(crRight - crLeft, crBottom - crTop);

    // Exposure (EV-like stops)
    c *= pow(2.0, exposure * 0.01);

    // Brightness
    c += brightness * 0.01;

    // Contrast (pivot at 0.5)
    c = vec3(0.5) + (c - vec3(0.5)) * (1.0 + contrast * 0.01);

    // Highlights (brighten values > 0.5)
    float hf = highlights * 0.01;
    vec3 hMask = smoothstep(0.35, 0.65, c);
    c += hMask * (c - vec3(0.5)) * hf;

    // Shadows (darken/lighten values < 0.5)
    float sf = shadows * 0.01;
    vec3 sMask = vec3(1.0) - smoothstep(0.35, 0.65, c);
    c += sMask * (vec3(0.5) - c) * sf * -1.0;

    c = clamp(c, 0.0, 1.0);

    // Temperature (warm = shift R up / B down)
    c.r += temperature * 0.002;
    c.b -= temperature * 0.002;

    // Tint (shift G)
    c.g += tintVal * 0.002;

    c = clamp(c, 0.0, 1.0);

    // Saturation + Vibrance
    float gray = dot(c, vec3(0.299, 0.587, 0.114));
    float satF = 1.0 + saturation * 0.01;
    if (abs(vibrance) > 0.01) {
        float maxC = max(c.r, max(c.g, c.b));
        float minC = min(c.r, min(c.g, c.b));
        float curSat = maxC > 0.0 ? (maxC - minC) / maxC : 0.0;
        satF += vibrance * 0.01 * (1.0 - curSat);
    }
    c = vec3(gray) + (c - vec3(gray)) * satF;

    // Hue rotation
    if (abs(hueShift) > 0.1) {
        float angle = hueShift * 3.14159265 / 180.0;
        float cosA = cos(angle);
        float sinA = sin(angle);
        float k = 0.57735; // 1/sqrt(3)
        mat3 hm = mat3(
            cosA + (1.0 - cosA) / 3.0,
            (1.0 - cosA) / 3.0 - k * sinA,
            (1.0 - cosA) / 3.0 + k * sinA,
            (1.0 - cosA) / 3.0 + k * sinA,
            cosA + (1.0 - cosA) / 3.0,
            (1.0 - cosA) / 3.0 - k * sinA,
            (1.0 - cosA) / 3.0 - k * sinA,
            (1.0 - cosA) / 3.0 + k * sinA,
            cosA + (1.0 - cosA) / 3.0
        );
        c = hm * c;
    }

    // Fade (lift blacks toward mid-gray, matte look)
    if (fade > 0.0) {
        float fl = fade * 0.01 * 0.3;
        c = c + (vec3(0.5) - c) * fl;
    }

    // Vignette (radial darkening — relative to video content, not full panel)
    if (vignetteAmt > 0.0) {
        vec2 vigUV = contentUV - vec2(0.5);
        float dist = length(vigUV) * 1.4142;
        float vig = 1.0 - vignetteAmt * 0.01 * dist * dist;
        c *= vig;
    }

    fragColor = vec4(clamp(c, 0.0, 1.0), tex.a) * qt_Opacity;
}

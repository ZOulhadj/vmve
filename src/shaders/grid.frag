#version 450

layout (location = 0) in vec2 uv;
layout (location = 1) in float grid_size;

layout (location=0) out vec4 out_FragColor;

// size of one cell
float gridCellSize = 0.0;

// color of thin lines
vec4 gridColorThin = vec4(0.5, 0.5, 0.5, 1.0);

// color of thick lines (every tenth line)
vec4 gridColorThick = vec4(0.0, 0.0, 1.0, 1.0);

// minimum number of pixels between cell lines before LOD switch should occur.
const float gridMinPixelsBetweenCells = 1.0;

float log10(float x)
{
    return log(x) / log(10.0);
}

float satf(float x)
{
    return clamp(x, 0.0, 1.0);
}

vec2 satv(vec2 x)
{
    return clamp(x, vec2(0.0), vec2(1.0));
}

float max2(vec2 v)
{
    return max(v.x, v.y);
}

vec4 gridColor(vec2 uv)
{
    vec2 dudv = vec2(
    length(vec2(dFdx(uv.x), dFdy(uv.x))),
    length(vec2(dFdx(uv.y), dFdy(uv.y)))
    );

    float lodLevel = max(0.0, log10((length(dudv) * gridMinPixelsBetweenCells) / gridCellSize) + 1.0);
    float lodFade = fract(lodLevel);

    // cell sizes for lod0, lod1 and lod2
    float lod0 = gridCellSize * pow(10.0, floor(lodLevel));
    float lod1 = lod0 * 10.0;
    float lod2 = lod1 * 10.0;

    // each anti-aliased line covers up to 4 pixels
    dudv *= 4.0;

    // calculate absolute distances to cell line centers for each lod and pick max X/Y to get coverage alpha value
    float lod0a = max2( vec2(1.0) - abs(satv(mod(uv, lod0) / dudv) * 2.0 - vec2(1.0)) );
    float lod1a = max2( vec2(1.0) - abs(satv(mod(uv, lod1) / dudv) * 2.0 - vec2(1.0)) );
    float lod2a = max2( vec2(1.0) - abs(satv(mod(uv, lod2) / dudv) * 2.0 - vec2(1.0)) );

    // blend between falloff colors to handle LOD transition
    vec4 c;
    if (lod2a > 0.0) {
        c = gridColorThick;
    } else if (lod1a > 0.0) {
        c = mix(gridColorThick, gridColorThin, lodFade);
        //c = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    } else {
        c = gridColorThin;
        discard;
    }

    // calculate opacity falloff based on distance to grid extents
    float opacityFalloff = (1.0 - satf(length(uv) / grid_size));

    // blend between LOD level alphas and scale with opacity falloff
    if (lod2a > 0.0) {
        c.a *= lod2a * opacityFalloff;
    } else if (lod1a > 0.0) {
        c.a *= lod1a * opacityFalloff;
    } else {
        c.a *= (lod0a * (1.0 - lodFade)) * opacityFalloff;
    }

    return c;
}



void main()
{
    out_FragColor = gridColor(uv);
}
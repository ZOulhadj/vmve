cbuffer view_projection
{
    float4x4 view;
    float4x4 proj;
};


struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

VS_OUTPUT main_vertex(float4 inPos : POSITION, float4 inColor : COLOR)
{
    float4x4 vp = view * proj;

    VS_OUTPUT output;
    //output.Pos = mul(inPos, vp);
    output.Pos = inPos;
    output.Color = inColor;

    return output;
}

float4 main_pixel(VS_OUTPUT input) : SV_TARGET
{
    return input.Color;
}
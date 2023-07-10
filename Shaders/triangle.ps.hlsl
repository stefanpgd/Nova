struct PixelData
{
    float time;
};
ConstantBuffer<PixelData> pixelData : register(b1);

struct PixelInput
{
    float4 Position : SV_Position;
    float3 color : COLOR;
    float3 fragPos : COLOR1;
};

float N21(float2 uv)
{
    return frac(sin(uv.x * 21.281 + uv.y * 93.182) * 5821.92);
}

float Line(float2 uv)
{
    return smoothstep(0.0, 0.05, uv.x) - smoothstep(0.0, 0.95, uv.x);
}

float4 main(PixelInput IN) : SV_Target0
{
    float2 uv = abs(IN.fragPos.xy) * 0.2f;
    float2 scale = float2(256, 64);
    
    float2 lUV = frac(uv * scale);
    float2 gID = floor(uv * scale);

    float rowNoise = N21(float2(0.0, gID.y));
    float dir = ((rowNoise * 2.0) - 1.0) + 0.2;
    gID.x += floor(pixelData.time * dir * 30.);
    
    float cellNoise = N21(gID);
    float drawBlock = float(cellNoise > 0.28);
    int even = int(gID.y) % 2;

    float3 col = float3(Line(lUV), Line(lUV), Line(lUV)) * drawBlock * float(even);
    col *= frac(sin(gID.y)) + 0.24;
    col *= float3(0.2, 0.94, 0.322);
    
    float4 c = float4(float3(col), 1.0);
    
    return float4(c);
}
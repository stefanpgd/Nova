struct PixelInput
{
    float4 Position : SV_Position;
    float3 color : COLOR;
};

float1 N21(float2 uv)
{
    return frac(sin(uv.x * 18.281 + uv.y * 102.381) * 5023.182);
}

float1 SimplexNoise(float2 uv, float1 scale)
{
    float2 gID = floor(uv * scale);
    float2 localUV = frac(uv * scale);
    localUV = smoothstep(0.0, 1.0, localUV);
    
    float2 br = float2(gID + float2(1.0, 0.0));
    float1 b = lerp(N21(gID), N21(br), localUV.x);
    
    float2 tl = float2(gID + float2(0.0, 1.0));
    float2 tr = float2(gID + float2(1.0, 1.0));
    float1 t = lerp(N21(tl), N21(tr), localUV.x);
    
    return lerp(b, t, localUV.y);
}

float4 main(PixelInput IN) : SV_Target0
{
    float2 uv = IN.Position / float2(1280, 720);
    return float4(IN.color * SimplexNoise(uv, 50.5), 1.0f);
}
struct PixelInput
{
    float4 Position : SV_Position;
    float3 color : COLOR;
};

float4 main(PixelInput IN) : SV_Target0
{
    return float4(IN.color, 1.0f);
}
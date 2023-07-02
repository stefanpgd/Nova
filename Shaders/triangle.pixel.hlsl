struct PixelInput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};

float4 main(PixelInput IN) : SV_TARGET
{
    return IN.Color;
}
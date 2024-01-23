struct PixelIN
{
    float3 Color : Color0;
};

float4 main(PixelIN IN) : SV_TARGET
{
    float3 normalC = (IN.Color + float3(1.0, 1.0, 1.0)) * 0.5;
    
    float3 lightDir = normalize(float3(0.4, -1.0f, 0.0f));
    float diff = max(dot(IN.Color, -lightDir), 0.0);
    
    float3 materialColor = float3(1.0, 0.4, 0.1);
    
    float3 ambient = 0.03f * materialColor;
    float3 diffuseColor = diff * materialColor;
    
    return float4(ambient + diffuseColor, 1.0f);
}
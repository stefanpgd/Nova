struct PixelIN
{
    float3 Color : Color;
    float3 Normal : Normal;
};

float4 main(PixelIN IN) : SV_TARGET
{
    float3 normalC = (IN.Normal + float3(1.0, 1.0, 1.0)) * 0.5;
    
    float3 lightDir = normalize(float3(0.0f, -1.0f, 0.0f));
    float diff = max(dot(IN.Normal, -lightDir), 0.0);
    
    float3 ambient = IN.Color * 0.2f;
    float3 diffuse = IN.Color * diff;
    
    float3 output = min(ambient + diffuse, float3(1.0, 1.0, 1.0));
    
    return float4(normalC, 1.0f);
}
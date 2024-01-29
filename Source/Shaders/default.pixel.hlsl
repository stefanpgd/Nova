struct PixelIN
{
    float3 Color : Color;
    float3 Normal : Normal;
    float3 ViewDirection : View;
};

float4 main(PixelIN IN) : SV_TARGET
{
    float3 normalC = (IN.Normal + float3(1.0, 1.0, 1.0)) * 0.5;
    
    float3 lightDir = normalize(float3(0.0f, -1.0f, 0.0f));
    float diff = max(dot(IN.Normal, -IN.ViewDirection), 0.0);
    
    float3 ambient = IN.Color * 0.3f;
    float3 diffuse = IN.Color * diff;
    
    float3 output = max(min(ambient + diffuse, float3(1.0, 1.0, 1.0)), float3(0.0f, 0.0f, 0.0f));
    
    float gamma = 1.0 / 2.4;
    output = pow(output, float3(gamma, gamma, gamma));
    
    return float4(output, 1.0f);
}
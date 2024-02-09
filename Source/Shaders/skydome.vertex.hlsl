struct VertexPosColor
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VertexOutput
{
    float2 TexCoord : TexCoord;
    float4 Position : SV_POSITION;
};

struct SceneInfo
{
    float3 CameraFront;
};
ConstantBuffer<SceneInfo> Scene : register(b0);

VertexOutput main(VertexPosColor IN)
{
    VertexOutput OUT;
    OUT.Position = float4(IN.Position, 0.5);
    
    float pi = 3.14159265;
    float3 dir = normalize(Scene.CameraFront);
    float u = atan2(dir.x, dir.z) / (2.0 * pi) + 0.5;
    
    OUT.TexCoord = float2(abs(IN.TexCoord.x + u), abs(1.0 - IN.TexCoord.y));
    
    return OUT;
}
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

struct Transform
{
    matrix MVP;
};
ConstantBuffer<Transform> CameraTransform : register(b1);

VertexOutput main(VertexPosColor IN)
{
    VertexOutput OUT;
    
    OUT.Position = mul(CameraTransform.MVP, float4(IN.Position, 1.0f));
    OUT.TexCoord = IN.TexCoord;
    
    return OUT;
}
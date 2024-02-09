struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
	float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct VertexOutput
{
    float2 TexCoord : TexCoord;
    float3 CameraFront : CameraFront;
    float3 FragPosition : FragPosition;
    float4 Position : SV_POSITION;
};

struct SceneInfo
{
    float3 CameraFront;
};
ConstantBuffer<SceneInfo> Scene : register(b1);

VertexOutput main(VertexPosColor IN) 
{
    VertexOutput OUT;
    OUT.Position = float4(IN.Position, 1.0f);
    OUT.FragPosition = IN.Position;
    OUT.TexCoord = IN.TexCoord;
    OUT.CameraFront = Scene.CameraFront;
    
    return OUT;
}
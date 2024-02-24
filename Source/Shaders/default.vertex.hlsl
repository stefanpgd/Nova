struct TransformData
{
	matrix MVP;
	matrix Model;
    matrix Light;
};
ConstantBuffer<TransformData> Transform : register(b0);
 
struct SceneInfo
{
    float3 CameraPosition;
};
ConstantBuffer<SceneInfo> Scene : register(b1);

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
	float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};
 
struct VertexShaderOutput
{
	float3 Color : Color;
    float3x3 TBN : TBN;
    float3 Normal : Normal;
    float3 FragPosition : FragPosition;
    float4 FragLight : FragLight;
    float3 CameraPosition : CameraPosition;
    float2 TexCoord : TexCoord;
	float4 Position : SV_Position;
};
 
VertexShaderOutput main(VertexPosColor IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(Transform.MVP, float4(IN.Position, 1.0f));
    
    float3 normal = normalize(mul(Transform.Model, float4(IN.Normal, 0.0f)).xyz);
    float3 tangent = normalize(mul(Transform.Model, float4(IN.Tangent, 0.0f)).xyz);
    float3 biTangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, biTangent, normal);
    OUT.TBN = TBN;
    OUT.Normal = normal;
    
    OUT.FragPosition = mul(Transform.Model, float4(IN.Position, 1.0f)).rgb;
    OUT.FragLight = mul(Transform.Light, float4(OUT.FragPosition, 1.0f));
    
    OUT.Color = IN.Color;
    OUT.CameraPosition = Scene.CameraPosition;
    OUT.TexCoord = IN.TexCoord;
    
	return OUT;
}
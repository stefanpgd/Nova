struct TransformData
{
	matrix MVP;
	matrix Model;
};
ConstantBuffer<TransformData> Transform : register(b0);
 
struct SceneInfo
{
    float3 viewDirection;
};
ConstantBuffer<SceneInfo> Scene : register(b1);

struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
	float3 Color : COLOR;
};
 
struct VertexShaderOutput
{
	float3 Color : Color;
    float3 Normal : Normal;
    float3 FragPosition : FragPosition;
	float4 Position : SV_Position;
};
 
VertexShaderOutput main(VertexPosColor IN)
{
	VertexShaderOutput OUT;

	// TODO: Use Model instead of MVP for Normal transformation
	OUT.Position = mul(Transform.MVP, float4(IN.Position, 1.0f));
    OUT.Normal = normalize(mul(Transform.Model, float4(IN.Normal, 0.0f)).xyz);
    OUT.Color = IN.Color;
    OUT.FragPosition = mul(Transform.MVP, float4(IN.Position, 0.0f)).xyz;
 
	return OUT;
}
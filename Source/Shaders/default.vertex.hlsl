struct TransformData
{
	matrix MVP;
};
ConstantBuffer<TransformData> Transform : register(b0);
 
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
	float4 Position : SV_Position;
};
 
VertexShaderOutput main(VertexPosColor IN)
{
	VertexShaderOutput OUT;
 
	// TODO: Use Model instead of MVP for Normal transformation
	OUT.Position = mul(Transform.MVP, float4(IN.Position, 1.0f));
    OUT.Normal = normalize(mul(Transform.MVP, float4(IN.Normal, 0.0f)).xyz);
    OUT.Color = IN.Color;
 
	return OUT;
}
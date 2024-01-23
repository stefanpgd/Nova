struct TransformData
{
	matrix MVP;
};
ConstantBuffer<TransformData> Transform : register(b0);
 
struct VertexPosColor
{
	float3 Position : POSITION;
	float3 Color : COLOR;
};
 
struct VertexShaderOutput
{
	float3 Color : Color0;
	float4 Position : SV_Position;
};
 
VertexShaderOutput main(VertexPosColor IN)
{
	VertexShaderOutput OUT;
 
	OUT.Position = mul(Transform.MVP, float4(IN.Position, 1.0f));
    OUT.Color = normalize(mul(float4(IN.Color, 0.0), Transform.MVP).xyz);
 
	return OUT;
}
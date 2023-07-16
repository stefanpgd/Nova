struct ModelViewProjection
{
    matrix MVP;
};
ConstantBuffer<ModelViewProjection> MVPCB : register(b0);

// This will be the layout of our vertexbuffer
struct VertexIN
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct VertexOUT
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float3 fragPos : COLOR1;
};

// Here we tell HLSL that the input of our shader program should be a VertexPosColor struct
// meaning that for every vertex in the buffer we offset by 6 floats or 24 bytes
VertexOUT main(VertexIN IN)
{
    VertexOUT OUT;
    
    OUT.Position = mul(MVPCB.MVP, float4(IN.Position, 1.0f));
    OUT.Normal = normalize(mul(MVPCB.MVP, float4(IN.Normal, 0.0f)));
    OUT.fragPos = float3(OUT.Position.xyz);
    
	return OUT;
}
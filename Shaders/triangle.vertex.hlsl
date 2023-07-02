struct ModelViewProjection
{
    matrix MVP;
};
ConstantBuffer<ModelViewProjection> MVPCB : register(b0);

// This will be the layout of our vertexbuffer
struct VertexPosColor
{
    float3 Position : POSITION;
    float3 Color : COLOR;
};

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
};


// Here we tell HLSL that the input of our shader program should be a VertexPosColor struct
// meaning that for every vertex in the buffer we offset by 6 floats or 24 bytes
VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;
    
    OUT.Position = mul(MVPCB.MVP, float4(IN.Position, 1.0f));
    OUT.Color = float4(IN.Color, 1.0f);
    
	return OUT;
}
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

VertexOutput main(VertexPosColor IN)
{
    VertexOutput OUT;
    OUT.Position = float4(IN.Position, 1.0);
    OUT.TexCoord = IN.TexCoord;
    
    return OUT;
}
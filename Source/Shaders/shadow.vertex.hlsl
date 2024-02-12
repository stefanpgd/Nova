struct Transform
{
    matrix VP;
};
ConstantBuffer<Transform> LightTransform : register(b0);

float4 main( float4 pos : POSITION ) : SV_POSITION
{
    float4 outputPosition = mul(LightTransform.VP, float4(pos.xyz, 1.0));
    return outputPosition;
}
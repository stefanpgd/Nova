struct Transform
{
    matrix VP;
    matrix Model;
};
ConstantBuffer<Transform> LightTransform : register(b0);

float4 main( float4 pos : POSITION ) : SV_POSITION
{
    matrix MVP = mul(LightTransform.VP, LightTransform.Model);
    
    float4 outputPosition = mul(MVP, float4(pos.xyz, 1.0));
    return outputPosition;
}
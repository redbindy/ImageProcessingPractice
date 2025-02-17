struct Input
{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD;
};

Texture2D image : register(t0);
SamplerState samplerLinear : register(s0);

float4 main(Input pos) : SV_TARGET
{
    return image.Sample(samplerLinear, pos.texCoord);
}
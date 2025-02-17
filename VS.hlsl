struct Output
{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD;
};

Output main(float2 pos : POSITION)
{
    Output output;
    output.pos = float4(pos, 0.f, 1.f);
    output.texCoord = (float2(pos.x, -pos.y) - float2(-1.f, -1.f)) / 2;
    
    return output;
}
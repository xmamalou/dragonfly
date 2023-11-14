struct FragIn
{
    [[vk::location(0)]] float3 color : COLOR0;
};

struct FragOut
{
    [[vk::location(0)]] float3 color : COLOR0;
};

FragOut frag_main(FragIn input, uint index : SV_VertexID)
{
    FragOut output;
    
    output.color = input.color;
    
    return output;
}
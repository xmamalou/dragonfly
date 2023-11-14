struct VertexIn
{
    [[vk::location(0)]] float3 pos : POSITION0;
    [[vk::location(1)]] float3 color : COLOR0;
};

struct VertexOut
{
    float4 pos : SV_Position;
    [[vk::location(0)]] float3 color : COLOR0;
};

VertexOut vertex_main(VertexIn input, uint index : SV_VertexID)
{
    float4 finalPos = { input.pos[0], input.pos[1], input.pos[2], 0 };
    
    VertexOut output;
    output.color = input.color * index;
    output.pos = finalPos;
    
    return output;
}
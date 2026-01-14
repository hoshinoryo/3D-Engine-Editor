/*==============================================================================

   Offscreen用頂点シェーダー [shader_vertex_picking.hlsl]



==============================================================================*/

cbuffer CBPicking : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 proj;
    uint     objectId;
    uint3    _pad;
};

struct VS_IN
{
    float3 pos : POSITION;
};

struct VS_OUT
{
    float4 posH : SV_POSITION;
};

VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    
    // Position transfomation
    float4 w = mul(float4(vi.pos, 1.0f), world);
    float4 v = mul(w, view);
    vo.posH  = mul(v, proj);
    
    return vo;
}
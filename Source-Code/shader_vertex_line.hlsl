/*==============================================================================

   線描画用頂点シェーダー [shader_vertex_line.hlsl]



==============================================================================*/

cbuffer WorldCB : register(b0)
{
    float4x4 world;
};

cbuffer ViewCB : register(b1)
{
    float4x4 view;
};

cbuffer ProjCB : register(b2)
{
    float4x4 proj;
};

struct VS_IN
{
    float3 pos : POSITION0;
    float4 color : COLOR0;
};

struct VS_OUT
{
    float4 svPos : SV_POSITION;
    float4 color : COLOR0;
};


VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    
    // Position transfomation
    float4 wPos = mul(float4(vi.pos, 1.0f), world);
    float4 vPos = mul(wPos, view);
    vo.svPos = mul(vPos, proj);
    
    vo.color = vi.color;
    
    return vo;
}
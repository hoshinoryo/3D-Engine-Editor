/*==============================================================================

   Offscreen用ピクセルシェーダー [shader_pixel_picking.hlsl]



==============================================================================*/

cbuffer CBPicking : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 proj;
    uint objectId;
    uint3 _pad;
};

struct PS_IN
{
    float4 posH : SV_POSITION;
};

uint main(PS_IN pi) : SV_TARGET0
{
    return objectId;
}
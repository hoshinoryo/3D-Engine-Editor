/*==============================================================================

   3Dアニメーションなしモデル描画用頂点シェーダー [shader_vertex_3d_static.hlsl]

   * vertex shader didn't do any light simulation *


==============================================================================*/

// 定数バッファ

// static const uint MAX_BONES = 256;

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4x4 world;
};

cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 view;
};

cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4x4 proj;
};

struct VS_IN
{
    float4 posL : POSITION0; // local position
    float3 normalL : NORMAL0; // local normal
    float3 tangentL : TANGENT0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};


struct VS_OUT
{
    float4 posH : SV_POSITION; // clip position
    float4 posW : POSITION0; // world position
    float4 normalW : NORMAL0; // world normal
    float3 tangentW : TANGENT0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};


//=============================================================================
// 頂点シェーダ
//=============================================================================


VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    
    // ローカル空間の値を一旦コピー
    float4 localPos = vi.posL;
    float3 localNormal = vi.normalL;
    float3 localTangent = vi.tangentL;
    
    // 座標変換（スキニング後の頂点をworld/view.projへ）
    float4 mtxW = mul(localPos, world);
    float4 mtxWV = mul(mtxW, view);
    vo.posH = mul(mtxWV, proj);
    
    // 法線・tangentをワールド空間へ
    float3 normalW = mul(float4(localNormal, 0.0f), world).xyz;
    normalW = normalize(normalW);
    vo.normalW = float4(normalW, 0.0f);
    
    float3 tangentW = mul(float4(localTangent, 0.0f), world).xyz;
    tangentW = normalize(tangentW);
    vo.tangentW = tangentW;
    
    vo.posW = mtxW;
    vo.color = vi.color;
    vo.uv = vi.uv;
    
    return vo;
}

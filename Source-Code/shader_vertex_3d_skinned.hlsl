/*==============================================================================

   3Dアニメーションありモデル描画用頂点シェーダー [shader_vertex_3d_skinned.hlsl]

   * vertex shader didn't do any light simulation *


==============================================================================*/

// 定数バッファ

static const uint MAX_BONES = 256;

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

// スキニング用定数バッファ
cbuffer SkinningCB : register(b3)
{
    float4x4 boneMatrices[MAX_BONES];
};

struct VS_IN
{
    float4 posL : POSITION0; // local position
    float3 normalL : NORMAL0; // local normal
    float3 tangentL : TANGENT0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    
    uint4 boneIndex : BLENDINDICES0;
    float4 boneWeight : BLENDWEIGHT0;
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
    
    float4 skinnedPos;
    float3 skinnedNormal;
    float3 skinnedTangent;
    
    float weightSum = vi.boneWeight.x + vi.boneWeight.y + vi.boneWeight.z + vi.boneWeight.w;
    
    if (weightSum > 0.0001f)
    {
        skinnedPos = 0.0f;
        skinnedNormal = 0.0f;
        skinnedTangent = 0.0f;
        
        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            uint idx = vi.boneIndex[i];
            float w = vi.boneWeight[i];
            
            if (idx >= MAX_BONES)
                continue;
            
            if (w > 0.0f)
            {
                float4x4 M = boneMatrices[idx];
                
                // position
                skinnedPos += mul(localPos, M) * w;

                // normal
                float3 n = mul(float4(localNormal, 0.0f), M).xyz;
                skinnedNormal += n * w;
                
                // tangent
                float3 t = mul(float4(localTangent, 0.0f), M).xyz;
                skinnedTangent += t * w;
            }
        }
        
        skinnedNormal = normalize(skinnedNormal);
        skinnedTangent = normalize(skinnedTangent);
    }
    else
    {
        skinnedPos = vi.posL;
        skinnedNormal = vi.normalL;
        skinnedTangent = vi.tangentL;
    }

    // 座標変換（スキニング後の頂点をworld/view.projへ）
    float4 mtxW = mul(skinnedPos, world);
    float4 mtxWV = mul(mtxW, view);
    vo.posH = mul(mtxWV, proj);
    
    // 法線・tangentをワールド空間へ
    float3 normalW = mul(float4(skinnedNormal, 0.0f), world).xyz;
    normalW = normalize(normalW);
    vo.normalW = float4(normalW, 0.0f);
    
    float3 tangentW = mul(float4(skinnedTangent, 0.0f), world).xyz;
    tangentW = normalize(tangentW);
    vo.tangentW = tangentW;
    
    vo.posW = mtxW;
    vo.color = vi.color;
    vo.uv = vi.uv;
    
    return vo;
}

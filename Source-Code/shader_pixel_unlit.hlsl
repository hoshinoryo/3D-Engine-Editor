/*==============================================================================

   Unlitピクセルシェーダー [shader_pixel_unlit.hlsl]

--------------------------------------------------------------------------------

==============================================================================*/

cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4 diffuse_color;
};

struct PS_IN
{
    float4 posH : SV_POSITION; // clip position
    float4 posW : POSITION0;   // world position
    float4 normalW : NORMAL0;  // world normal
    float3 tangentW : TANGENT0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

Texture2D diffTex; // diffuse texture
SamplerState samp; // テクスチャサンプラ

float4 main(PS_IN pi) : SV_TARGET
{
    return diffTex.Sample(samp, pi.uv) * diffuse_color;
    // return float4(1, 0, 1, 1);
}
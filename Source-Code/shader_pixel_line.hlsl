/*==============================================================================

   線描画用ピクセルシェーダー [shader_pixel_line.hlsl]

   * No textrue sampling, no lighting constant buffer

==============================================================================*/

struct PS_IN
{
    float4 svPos : SV_POSITION;
    float4 color : COLOR0;
};

float4 main(PS_IN pi) : SV_TARGET
{
    return pi.color;
}
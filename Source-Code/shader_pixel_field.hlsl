/*==============================================================================

   フィールド描画用ピクセルシェーダー [shader_pixel_field.hlsl]
														 Author : Sato Youhei
														 Date   : 2025/09/26
--------------------------------------------------------------------------------

==============================================================================*/

cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4 diffuse_color;
};

cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4 ambient_color;
};

cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4 directional_world_vector;
    float4 directional_color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

cbuffer VS_CONSTANT_BUFFER : register(b3)
{
    float3 eye_posW;
    float specular_power = 30.0f;
    float4 specular_color = { 0.0f, 0.0f, 1.0f, 1.0f };
};

struct PS_IN
{
    float4 posH : SV_POSITION; // システム定義の頂点位置（クリップ空間座標）
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float4 blend : COLOR0;
    float2 uv : TEXCOORD0;
};

Texture2D tex0 : register(t0); // テクスチャ
Texture2D tex1 : register(t1);

SamplerState samp; // テクスチャサンプラ


float4 main(PS_IN pi) : SV_TARGET
{
    float2 uv;
    float angle = 3.14159f / 180.0f;
    uv.x = pi.uv.x * cos(angle) + pi.uv.y * sin(angle);
    uv.y = -pi.uv.x * sin(angle) + pi.uv.y * cos(angle);
    
    // float4 tex_color = tex0.Sample(samp, pi.uv) * pi.blend.g + tex1.Sample(samp, pi.uv) * pi.blend.r;
    float4 tex_color = tex0.Sample(samp, pi.uv) * 0.5f + tex1.Sample(samp, pi.uv) * 0.5f;
    
    // 材質の色
    float3 material_color = tex_color.rgb * pi.blend.rgb * diffuse_color.rgb;
    
    // 平行光源
    float4 normalW = normalize(pi.normalW);
    // float dl = max(0.0f, dot(-directional_world_vector, normalW));
    float dl = (dot(-directional_world_vector, normalW) + 1.0f) * 0.5f;
    float3 diffuse = material_color * directional_color.rgb * dl;
    
    // 環境光（ambient light）
    float3 ambient = material_color * ambient_color.rgb;
    
    // スペキュラ
    float3 toEye = normalize(eye_posW - pi.posW.xyz);
    float3 r = reflect(normalize(directional_world_vector), normalW);
    float t = pow(max(dot(r, toEye), 0.0f), specular_power);
    float3 specular = specular_color.rgb * t;
    
    // 最終カラー
    float3 color = ambient + diffuse + specular;
    
    return float4(color, 1.0f);
    
    // return tex0.Sample(samp, uv) * 0.5f + tex1.Sample(samp, pi.uv) * 0.5f;
    // return tex0.Sample(samp, pi.uv) * pi.color.g + tex1.Sample(samp, pi.uv) * pi.color.r;
}

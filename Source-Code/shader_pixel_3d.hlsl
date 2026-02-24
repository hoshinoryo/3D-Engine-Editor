/*==============================================================================

   3D描画用ピクセルシェーダー [shader_pixel_3d.hlsl]  (Phong Model)

--------------------------------------------------------------------------------

==============================================================================*/

cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4 diffuse_color;
};

cbuffer PS_CONSTANT_BUFFER : register(b1)
{
    float4 ambient_color;
};

cbuffer PS_CONSTANT_BUFFER : register(b2)
{
    float4 directional_world_vector;
    float4 directional_color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

// from shader: set specular
cbuffer PS_CONSTANT_BUFFER : register(b3)
{
    float3 eye_posW;
    float specular_power;
    float4 specular_color;
};

struct PointLight
{
    float3 posW;
    float range;
    float4 color;
};

cbuffer PS_CONSTANT_BUFFER : register(b4)
{
    PointLight point_light[4];
    int point_light_count;
    float3 point_light_dummy;
};

cbuffer ShadowPS_CONSTANT_BUFFER : register(b5)
{
    float4x4 lightViewProj;
    float    shadowBias;
    float    shadowStrength;
    float2   shadowTexelSize;
};

struct PS_IN
{
    float4 posH     : SV_POSITION; // システム定義の頂点位置（クリップ空間座標）
    float4 posW     : POSITION0;
    float4 normalW  : NORMAL0;
    float3 tangentW : TANGENT0;
    float4 color    : COLOR0;
    float2 uv       : TEXCOORD0;
    
    float4 posLightH : TEXCOORD1;
};


Texture2D diffTex   : register(t0);  // diffuse texture
Texture2D normalTex : register(t1);
Texture2D specTex   : register(t2);
Texture2D shadowMap : register(t7);

SamplerState samp : register(s0); // テクスチャサンプラ
SamplerComparisonState shadowSamp : register(s7);


float2 ShadowUV(float4 posLightH)
{
    float2 uv = posLightH.xy / posLightH.w;
    uv.x = uv.x * 0.5f + 0.5f;
    uv.y = uv.y * -0.5f + 0.5f;
    return uv;
}

float ShadowDepth(float4 posLightH)
{
    //float3 ndc = posLightH / posLightH.w;
    return posLightH.z / posLightH.w;
}

float CalcShadow(float4 posLightH, float3 normalW)
{
    if (posLightH.w <= 1e-4f)
        return 1.0f;
    
    float2 uv = ShadowUV(posLightH);
    float depth = ShadowDepth(posLightH);
    
    if (uv.x <= 0.0f || uv.x >= 1.0f || uv.y <= 0.0f || uv.y >= 1.0f || depth <= 0.0f || depth >= 1.0f)
        return 1.0f;
    
    // slope-scaled bias
    float3 N = normalize(normalW);
    float3 L = normalize(-directional_world_vector.xyz);
    
    float ndotl = saturate(dot(N, L));
    
    float base = shadowBias;
    float bias = base * (1.0f - ndotl) + base * 0.25f;
    
    float sum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            float2 o = float2(x, y) * shadowTexelSize;
            sum += shadowMap.SampleCmpLevelZero(shadowSamp, uv + o, depth - bias);

        }
    }
    
    //return shadowMap.SampleCmpLevelZero(shadowSamp, uv, depth - bias);
    return sum / 9.0f;
}


float4 main(PS_IN pi) : SV_TARGET
{
    // diffuse sampling
    float3 material_color = diffTex.Sample(samp, pi.uv).rgb * pi.color.rgb * diffuse_color.rgb;
    
    // normal map sampling
    float3 nTS = normalTex.Sample(samp, pi.uv).xyz * 2.0f - 1.0f;
    
    // specular map sampling
    float3 specSamp = specTex.Sample(samp, pi.uv).rgb;
    
    // TBN space
    float3 N = normalize(pi.normalW.xyz);
    float3 T = normalize(pi.tangentW);
    T = normalize(T - N * dot(T, N));
    float3 B = cross(N, T);
    float3 normalW = normalize(mul(nTS, float3x3(T, B, N)));
    
    float3 V = normalize(eye_posW - pi.posW.xyz);
    
    // ---- Shadow ----
    //float shadow = CalcShadow(pi.posLightH, normalW);
    float vis = CalcShadow(pi.posLightH, normalW);
    float shadowTerm = lerp(1.0f, vis, shadowStrength);
    
    // ---- Ambient Light ----
    float3 ambient = material_color * ambient_color.rgb;
    
    // ---- Directional Light ----
    float3 LightDir = normalize(-directional_world_vector.xyz); // from pixel to light
    float dl = max(0.0f, dot(LightDir, normalW));
    float3 diffuse = material_color * directional_color.rgb * dl * shadowTerm;
    
    // ---- Specular Light ----
    float3 r = reflect(-LightDir, normalW);
    float t = pow(saturate(dot(r, V)), specular_power);
    float3 specular = specular_color.rgb * specSamp * t * shadowTerm;

    // final color
    float3 color = ambient + diffuse + specular;

    // ---- Point Light ----
    for (int i = 0; i < point_light_count; i++)
    {
        // 点光源から面（ピクセル）へのベクトルを算出
        float3 lightToPixel_vec = point_light[i].posW - pi.posW.xyz;
        //float3 pixelToLight = pi.posW.xyz - point_light[i].posW;
        
        // 面（ピクセル）とライトの距離を測る
        float distance = length(lightToPixel_vec);
        float3 lightToPixel = lightToPixel_vec / max(distance, 1e-4);
    
        // 影響力の計算
        float A = saturate(1.0f - distance / point_light[i].range);
        A *= A;
    
        // 点光源と面（ピクセル）との向きを考慮を入れる
        float dl = max(0.0f, dot(lightToPixel, normalW));
        
        // 点光源の影響を加算する
        color += material_color * point_light[i].color.rgb * A * dl;
        
        // 点光源のスペキュラ
        float3 r = reflect(-lightToPixel, normalW);
        float t = pow(saturate(dot(r, V)), specular_power);
        float3 point_light_specular = specular_color.rgb * point_light[i].color.rgb * specSamp * t;
        
        color += point_light_specular * A;
    }
    
    float alpha = diffTex.Sample(samp, pi.uv).a * diffuse_color.a * pi.color.a;
    
    return float4(color, alpha);
}


/*==============================================================================

   Outline描画用ピクセルシェーダー [shader_pixel_outline_post.hlsl]



==============================================================================*/

cbuffer CBOutline : register(b0)
{
    uint selectedId;
    uint width;
    uint height;
    uint thickness;
    float4 color;
};

Texture2D<uint> idTex : register(t0);

float4 main(float4 pos : SV_POSITION) : SV_Target
{
    int2 p = int2(pos.xy);
    
    // clamp
    p.x = clamp(p.x, 0, int(width) - 1);
    p.y = clamp(p.y, 0, int(height) - 1);
    
    uint c = idTex.Load(int3(p, 0));
    if (c != selectedId)
        return float4(0, 0, 0, 0);
    
    int t = int(thickness);
    
    [loop]
    for (int dy = -t; dy <= t; ++dy)
    {
        [loop]
        for (int dx = -t; dx <= t; ++dx)
        {
            if (dx == 0 && dy == 0)
                continue;
            
            int2 q = p + int2(dx, dy);
            q.x = clamp(q.x, 0, int(width) - 1);
            q.y = clamp(q.y, 0, int(height) - 1);
            
            uint v = idTex.Load(int3(q, 0));
            if (v != selectedId)
                return color;
        }

    }

    return float4(0, 0, 0, 0);
}

/*==============================================================================

   Outline描画用ピクセルシェーダー（Post process）

   このシェーダーは、PickingPassで作った「ObjectID画像（idTex）」を参照して、
   選択中のオブジェクト（selectedId）の輪郭だけを抽出し、Outline色を出力します。

   基本原理：
   1) 現在のピクセル座標pのObjectID = idTex[p]を読む
   2) それがselectedIdでなければ → 透明（Outline無し）
   3) selectedIdなら周囲（近傍）のピクセルをチェックする
      周囲にselectedIdではないピクセルが1つでもあれば、そこは「境界（輪郭）」なので
      → Outline色を出力する
   4) 周囲がすべてselectedIdなら「内部ピクセル」なので
      → 透明（Outline無し）

   thickness：
   ・輪郭判定に使う近傍の半径（ピクセル単位）
   ・1なら上下左右＋斜めの1ピクセル範囲、2なら2ピクセル範囲…というように太さが変わる

==============================================================================*/
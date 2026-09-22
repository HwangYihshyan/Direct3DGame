//--------------------------------------------------------------------
//
// ファイル名：ScreenPS.hlsl
// お仕事　　：出来上がった画面へ最後の味付けをする
//
// Direct2D で描いた1枚の絵を受け取り、
//
//     ・画面の四隅を落として中央へ目を寄せる（ビネット）
//     ・被弾した直後は縁を赤くにじませる
//
// を掛けてからバックバッファへ出す。
//
// 元の絵はそのまま残したいので、どちらも控えめにしている。
//
//--------------------------------------------------------------------

Texture2D    SceneTexture : register(t0);
SamplerState SceneSampler : register(s0);

cbuffer ScreenParams : register(b0)
{
    // 被弾直後 1.0 → 0.0 へ落ちる
    float DamageFlash;

    // ビネットの強さ。0 で無効
    float VignetteScale;

    // 予備（16バイト境界にそろえるため）
    float Reserved0;
    float Reserved1;
};

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float3 color = SceneTexture.Sample(SceneSampler, uv).rgb;

    //----------------------------------------------------------------
    // 中央からの距離。四隅ほど大きくなる。
    //----------------------------------------------------------------
    float2 offset = uv - 0.5f;

    // 横長の画面なので、横方向を少し詰めて円に近づける
    offset.x *= 0.85f;

    float distance2 = dot(offset, offset);

    //----------------------------------------------------------------
    // ビネット
    //   中央は 1.0、四隅で暗くなる。
    //----------------------------------------------------------------
    float shade = saturate(1.0f - distance2 * VignetteScale);

    color *= lerp(1.0f, shade, 0.80f);

    //----------------------------------------------------------------
    // 被弾したときの赤み
    //   中央は残し、縁だけ赤くする。
    //   画面全体を赤くすると敵が見えなくなるため。
    //----------------------------------------------------------------
    float edge = saturate(distance2 * 5.0f);

    color = lerp(
        color,
        float3(0.85f, 0.10f, 0.12f),
        DamageFlash * edge * 0.55f
    );

    return float4(color, 1.0f);
}

//--------------------------------------------------------------------
//
// ファイル名：ScreenVS.hlsl
// お仕事　　：画面全体を覆う三角形を1枚作る
//
// 頂点バッファは使わない。
// SV_VertexID（0,1,2）から座標を計算するので、
// 頂点バッファも入力レイアウトも要らない。
//
//   id=0 → (-1, +1)  左上
//   id=1 → (+3, +1)  右へはみ出す
//   id=2 → (-1, -3)  下へはみ出す
//
// 画面より大きい三角形1枚で覆うことで、
// 四角形（2枚の三角形）にありがちな対角線の継ぎ目が出ない。
//
//--------------------------------------------------------------------

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

VSOutput main(uint id : SV_VertexID)
{
    VSOutput output;

    // (0,0) (2,0) (0,2)
    output.uv = float2((id << 1) & 2, id & 2);

    // UV を そのままクリップ座標へ。Y は上下が逆
    output.position = float4(
        output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f),
        0.0f,
        1.0f
    );

    return output;
}

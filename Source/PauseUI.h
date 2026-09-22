/*--------------------------------------------------------------------

ファイル名：PauseUI.h
お仕事　　：ポーズ画面

設計書 Ver.3 19-6.4
    Resume / Option / Restart / Title

Escで開き、もう一度Escかカーソルの選択で閉じる。
開いている間、GameSceneは更新を止める。

----------------------------------------------------------------------*/

#ifndef PAUSEUI_H
#define PAUSEUI_H

#include "InputTracker.h"
#include "UIRenderer.h"

enum class PauseResult
{
    None,       // 変化なし
    Resume,     // ゲームへ戻る
    Option,     // 設定画面へ（ポーズは開いたままにする）
    Restart,    // 同じステージをやり直す
    Title       // タイトルへ
};

class PauseUI
{
public:
    void Open();
    void Close();

    bool IsOpen() const { return m_open; }

    // 開いている間だけ入力を処理する
    PauseResult Update(
        const InputTracker& input,
        float mouseX,
        float mouseY,
        float deltaTime = 0.016f
    );

    void Draw(
        const UIRenderer& ui,
        float screenWidth,
        float screenHeight
    ) const;

private:
    static constexpr int ITEM_COUNT = 4;

    bool m_open = false;
    int m_selected = 0;
    float m_blink = 0.0f;
};

#endif

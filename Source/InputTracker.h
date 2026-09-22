/*--------------------------------------------------------------------

ファイル名：InputTracker.h
お仕事　　：キー／マウスの「押した瞬間」を取る

GetAsyncKeyState を毎フレーム保存して、
前フレームとの差分で押下エッジを判定する。

----------------------------------------------------------------------*/

#ifndef INPUTTRACKER_H
#define INPUTTRACKER_H

#include <windows.h>

class InputTracker
{
public:
    // 毎フレーム先頭で1回だけ呼ぶ
    void Capture()
    {
        for (int i = 0; i < 256; ++i)
        {
            m_previous[i] = m_current[i];

            m_current[i] =
                (GetAsyncKeyState(i) & 0x8000) != 0;
        }
    }

    //--------------------------------------------------------
    // 今の状態を「前フレームもそうだった」ことにする。
    //
    // 別の画面へ行って戻ってきたときに使う。
    // これをしないと、押しっぱなしのキーが
    // 戻った瞬間に「押した」と判定されてしまう。
    //--------------------------------------------------------
    void Sync()
    {
        for (int i = 0; i < 256; ++i)
        {
            m_current[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
            m_previous[i] = m_current[i];
        }
    }

    void Clear()
    {
        for (int i = 0; i < 256; ++i)
        {
            m_current[i] = false;
            m_previous[i] = false;
        }
    }

    // 押しっぱなし
    bool IsDown(int virtualKey) const
    {
        if (virtualKey < 0 || virtualKey >= 256)
        {
            return false;
        }

        return m_current[virtualKey];
    }

    // 押した瞬間
    bool IsPressed(int virtualKey) const
    {
        if (virtualKey < 0 || virtualKey >= 256)
        {
            return false;
        }

        return m_current[virtualKey] && !m_previous[virtualKey];
    }

private:
    bool m_current[256] = {};
    bool m_previous[256] = {};
};

#endif

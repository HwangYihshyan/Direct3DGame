/*--------------------------------------------------------------------

ファイル名：AudioManager.cpp
お仕事　　：BGM と SE の再生

流れ：
    mp3  →  Media Foundation で 16bit PCM へ展開  →  XAudio2 で再生

読み込みに失敗したパスは「失敗した」と覚えておき、
毎フレーム読み直さないようにしている。

このファイルは UTF-8（BOM 付き）で保存すること。
SE のファイル名に日本語を使っているため。

----------------------------------------------------------------------*/

#define NOMINMAX
#include "AudioManager.h"

#include <windows.h>

#include <xaudio2.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <wrl/client.h>

#include <map>
#include <memory>
#include <vector>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

using Microsoft::WRL::ComPtr;

namespace
{
    //--------------------------------------------------------
    // SE のファイル。SeId の並びと必ず同じ順にすること。
    //
    // ファイル名は半角英数のみ。
    // 日本語名は zip でやり取りした際に文字化けし、
    // 開けなくなることがあったため。
    //--------------------------------------------------------
    const wchar_t* SE_PATH[static_cast<int>(SeId::Count)] =
    {
        L"Assets\\sound\\se_shot.mp3",             // 弾発射
        L"Assets\\sound\\se_headshot.mp3",         // ヘッドショット
        L"Assets\\sound\\se_dash.mp3",             // ダッシュ音
        L"Assets\\sound\\se_enemy_hit.mp3",        // 敵とぶつかる音
        L"Assets\\sound\\se_break.mp3",            // 物体破壊
        L"Assets\\sound\\se_chest.mp3",            // 宝箱を開ける
        L"Assets\\sound\\se_heal.mp3",             // ゲージ回復
        L"Assets\\sound\\se_skill_assault.mp3",    // 突撃兵Kスキル
        L"Assets\\sound\\se_skill_assault_l.mp3",  // 突撃兵Lスキル
        L"Assets\\sound\\se_skill_heavy.mp3",      // 重装兵Kスキル
        L"Assets\\sound\\se_skill_heavy_l.mp3",    // 重装兵Lスキル
        L"Assets\\sound\\se_menu_move.mp3",        // カーソル項目移動
        L"Assets\\sound\\se_menu_decide.mp3",      // 選択項目決定
        L"Assets\\sound\\se_menu_back.mp3",        // 戻る
        L"Assets\\sound\\se_scene_change.mp3"      // 設定背景遷移時
    };

    // 1つの SE を同時に何本まで重ねるか
    constexpr int SE_VOICE_COUNT = 4;

    //--------------------------------------------------------
    // 展開済みの波形
    //--------------------------------------------------------
    struct SoundData
    {
        WAVEFORMATEX format{};
        std::vector<BYTE> samples;
    };

    //--------------------------------------------------------
    // SE 1種類ぶん（波形＋再生枠）
    //--------------------------------------------------------
    struct SeEntry
    {
        SoundData data;

        IXAudio2SourceVoice* voices[SE_VOICE_COUNT] = {};

        int next = 0;

        // 読み込みに成功したか。false ならもう読みに行かない。
        bool valid = false;
    };

    bool g_ready = false;
    bool g_mediaFoundation = false;

    ComPtr<IXAudio2> g_xaudio;
    IXAudio2MasteringVoice* g_masteringVoice = nullptr;

    //--------------------------------------------------------
    // BGM
    //   g_bgmData を作り直すときは、必ず先に voice を破棄する。
    //   XAudio2 は波形を参照しているだけなので、
    //   先に開放すると再生中のバッファが消えてしまう。
    //--------------------------------------------------------
    IXAudio2SourceVoice* g_bgmVoice = nullptr;
    SoundData g_bgmData;
    std::wstring g_bgmPath;

    // 今鳴っているのがループするBGMか、1回だけの曲か
    bool g_bgmLooping = true;

    std::map<std::wstring, std::unique_ptr<SeEntry>> g_seCache;

    float g_master = 0.8f;
    float g_bgm = 0.8f;
    float g_se = 0.8f;

    float BgmVolume() { return g_master * g_bgm; }
    float SeVolume() { return g_master * g_se; }

    //--------------------------------------------------------
    // 相対パスを絶対パスへ直す
    //   Media Foundation は URL として解釈するため、
    //   "Assets\\sound\\..." のままでは開けないことがある。
    //--------------------------------------------------------
    std::wstring ToFullPath(const std::wstring& path)
    {
        wchar_t buffer[MAX_PATH]{};

        const DWORD length = GetFullPathNameW(
            path.c_str(),
            MAX_PATH,
            buffer,
            nullptr
        );

        if (length == 0 || length >= MAX_PATH)
        {
            return path;
        }

        return std::wstring(buffer);
    }

    //--------------------------------------------------------
    // mp3 / wav を 16bit PCM へ展開する
    //--------------------------------------------------------
    bool DecodeToPcm(const std::wstring& path, SoundData& out)
    {
        const std::wstring fullPath = ToFullPath(path);

        ComPtr<IMFSourceReader> reader;

        HRESULT hr = MFCreateSourceReaderFromURL(
            fullPath.c_str(),
            nullptr,
            reader.GetAddressOf()
        );

        if (FAILED(hr))
        {
            // ファイルが無い場合もここへ来る
            return false;
        }

        //----------------------------------------------------
        // 出力形式を PCM に指定する。
        // 具体的なサンプリングレートは読み込んだ側に合わせる。
        //----------------------------------------------------
        ComPtr<IMFMediaType> request;

        hr = MFCreateMediaType(request.GetAddressOf());

        if (FAILED(hr))
        {
            return false;
        }

        request->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        request->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);

        hr = reader->SetCurrentMediaType(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
            nullptr,
            request.Get()
        );

        if (FAILED(hr))
        {
            return false;
        }

        ComPtr<IMFMediaType> actual;

        hr = reader->GetCurrentMediaType(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
            actual.GetAddressOf()
        );

        if (FAILED(hr))
        {
            return false;
        }

        WAVEFORMATEX* waveFormat = nullptr;
        UINT32 waveSize = 0;

        hr = MFCreateWaveFormatExFromMFMediaType(
            actual.Get(),
            &waveFormat,
            &waveSize
        );

        if (FAILED(hr) || waveFormat == nullptr)
        {
            return false;
        }

        out.format = *waveFormat;

        // PCM なので拡張領域は要らない
        out.format.cbSize = 0;

        CoTaskMemFree(waveFormat);

        //----------------------------------------------------
        // 最後まで読み出してつなげる
        //----------------------------------------------------
        for (;;)
        {
            DWORD flags = 0;
            ComPtr<IMFSample> sample;

            hr = reader->ReadSample(
                static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
                0,
                nullptr,
                &flags,
                nullptr,
                sample.GetAddressOf()
            );

            if (FAILED(hr))
            {
                return false;
            }

            if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                break;
            }

            if (!sample)
            {
                continue;
            }

            ComPtr<IMFMediaBuffer> buffer;

            hr = sample->ConvertToContiguousBuffer(buffer.GetAddressOf());

            if (FAILED(hr))
            {
                return false;
            }

            BYTE* data = nullptr;
            DWORD length = 0;

            hr = buffer->Lock(&data, nullptr, &length);

            if (FAILED(hr))
            {
                return false;
            }

            out.samples.insert(out.samples.end(), data, data + length);

            buffer->Unlock();
        }

        return !out.samples.empty();
    }

    void DestroyVoice(IXAudio2SourceVoice*& voice)
    {
        if (voice)
        {
            voice->Stop(0);
            voice->FlushSourceBuffers();
            voice->DestroyVoice();

            voice = nullptr;
        }
    }

    //--------------------------------------------------------
    // SE を1つ用意する。読み込み済みならそれを返す。
    //--------------------------------------------------------
    SeEntry* AcquireSe(const std::wstring& path)
    {
        auto found = g_seCache.find(path);

        if (found != g_seCache.end())
        {
            return found->second.get();
        }

        auto entry = std::make_unique<SeEntry>();

        if (DecodeToPcm(path, entry->data))
        {
            entry->valid = true;

            for (int i = 0; i < SE_VOICE_COUNT; ++i)
            {
                HRESULT hr = g_xaudio->CreateSourceVoice(
                    &entry->voices[i],
                    &entry->data.format
                );

                if (FAILED(hr))
                {
                    entry->voices[i] = nullptr;
                }
            }
        }

        SeEntry* raw = entry.get();

        // 失敗した場合も登録する。次から読みに行かないため。
        g_seCache[path] = std::move(entry);

        return raw;
    }
}

//============================================================
// 公開関数
//============================================================

bool Audio::Initialize()
{
    if (g_ready)
    {
        return true;
    }

    //--------------------------------------------------------
    // Media Foundation
    //--------------------------------------------------------
    HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);

    if (FAILED(hr))
    {
        return false;
    }

    g_mediaFoundation = true;

    //--------------------------------------------------------
    // XAudio2
    //--------------------------------------------------------
    hr = XAudio2Create(g_xaudio.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);

    if (FAILED(hr))
    {
        MFShutdown();
        g_mediaFoundation = false;

        return false;
    }

    hr = g_xaudio->CreateMasteringVoice(&g_masteringVoice);

    if (FAILED(hr))
    {
        g_xaudio.Reset();

        MFShutdown();
        g_mediaFoundation = false;

        return false;
    }

    g_ready = true;

    return true;
}

void Audio::Shutdown()
{
    if (!g_ready)
    {
        return;
    }

    StopBgm();

    for (auto& pair : g_seCache)
    {
        SeEntry* entry = pair.second.get();

        if (!entry)
        {
            continue;
        }

        for (int i = 0; i < SE_VOICE_COUNT; ++i)
        {
            DestroyVoice(entry->voices[i]);
        }
    }

    g_seCache.clear();

    if (g_masteringVoice)
    {
        g_masteringVoice->DestroyVoice();
        g_masteringVoice = nullptr;
    }

    g_xaudio.Reset();

    if (g_mediaFoundation)
    {
        MFShutdown();
        g_mediaFoundation = false;
    }

    g_ready = false;
}

void Audio::SetVolume(int master, int bgm, int se)
{
    auto normalize = [](int value)
    {
        if (value < 0)
        {
            value = 0;
        }

        if (value > 100)
        {
            value = 100;
        }

        return value / 100.0f;
    };

    g_master = normalize(master);
    g_bgm = normalize(bgm);
    g_se = normalize(se);

    if (g_bgmVoice)
    {
        g_bgmVoice->SetVolume(BgmVolume());
    }
}

namespace
{
    //--------------------------------------------------------
    // BGM の枠で1曲鳴らす。
    //   loop が false なら最後まで鳴って止まる。
    //--------------------------------------------------------
    void StartBgmVoice(const std::wstring& path, bool loop)
    {
        Audio::StopBgm();

        SoundData data;

        if (!DecodeToPcm(path, data))
        {
            // 曲が置かれていないだけなので、無音のまま続行する
            return;
        }

        g_bgmData = std::move(data);

        IXAudio2SourceVoice* voice = nullptr;

        if (FAILED(g_xaudio->CreateSourceVoice(&voice, &g_bgmData.format)))
        {
            g_bgmData.samples.clear();
            return;
        }

        XAUDIO2_BUFFER buffer{};

        buffer.AudioBytes = static_cast<UINT32>(g_bgmData.samples.size());
        buffer.pAudioData = g_bgmData.samples.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM;

        if (loop)
        {
            buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        }

        if (FAILED(voice->SubmitSourceBuffer(&buffer)))
        {
            voice->DestroyVoice();
            g_bgmData.samples.clear();
            return;
        }

        voice->SetVolume(BgmVolume());
        voice->Start(0);

        g_bgmVoice = voice;
        g_bgmPath = path;
        g_bgmLooping = loop;
    }
}

void Audio::PlayBgm(const std::wstring& path)
{
    if (!g_ready || path.empty())
    {
        return;
    }

    //--------------------------------------------------------
    // 同じ曲なら鳴らし直さない。
    // ステージをやり直しても曲が頭に戻らないようにするため。
    //--------------------------------------------------------
    if (path == g_bgmPath && g_bgmVoice)
    {
        return;
    }

    StartBgmVoice(path, true);
}

void Audio::PlayJingle(const std::wstring& path)
{
    if (!g_ready || path.empty())
    {
        return;
    }

    //--------------------------------------------------------
    // ファンファーレは繰り返さない。
    //
    // 同じパスでも鳴らし直す。
    // 「同じ曲なら流し続ける」を効かせてしまうと、
    // 2回目のゲームオーバーで鳴らないことがあるため。
    //--------------------------------------------------------
    StartBgmVoice(path, false);
}

bool Audio::IsJinglePlaying()
{
    if (!g_ready || !g_bgmVoice || g_bgmLooping)
    {
        return false;
    }

    XAUDIO2_VOICE_STATE state{};
    g_bgmVoice->GetState(&state);

    // 積んだバッファを鳴らし切ったら 0 になる
    return state.BuffersQueued > 0;
}

void Audio::StopBgm()
{
    DestroyVoice(g_bgmVoice);

    g_bgmData.samples.clear();
    g_bgmPath.clear();

    g_bgmLooping = true;
}

void Audio::PlaySe(SeId id)
{
    const int index = static_cast<int>(id);

    if (index < 0 || index >= static_cast<int>(SeId::Count))
    {
        return;
    }

    PlaySe(SE_PATH[index]);
}

void Audio::PlaySe(const std::wstring& path)
{
    if (!g_ready || path.empty())
    {
        return;
    }

    SeEntry* entry = AcquireSe(path);

    if (!entry || !entry->valid)
    {
        return;
    }

    //--------------------------------------------------------
    // 空いている枠を探す。全部塞がっていたら一番古い枠を奪う。
    //--------------------------------------------------------
    IXAudio2SourceVoice* target = nullptr;

    for (int i = 0; i < SE_VOICE_COUNT; ++i)
    {
        IXAudio2SourceVoice* voice = entry->voices[i];

        if (!voice)
        {
            continue;
        }

        XAUDIO2_VOICE_STATE state{};
        voice->GetState(&state);

        if (state.BuffersQueued == 0)
        {
            target = voice;
            break;
        }
    }

    if (!target)
    {
        target = entry->voices[entry->next];

        entry->next = (entry->next + 1) % SE_VOICE_COUNT;

        if (!target)
        {
            return;
        }

        target->Stop(0);
        target->FlushSourceBuffers();
    }

    XAUDIO2_BUFFER buffer{};

    buffer.AudioBytes = static_cast<UINT32>(entry->data.samples.size());
    buffer.pAudioData = entry->data.samples.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    if (FAILED(target->SubmitSourceBuffer(&buffer)))
    {
        return;
    }

    target->SetVolume(SeVolume());
    target->Start(0);
}

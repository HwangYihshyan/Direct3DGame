/*--------------------------------------------------------------------

ファイル名：LevelSelectScene.cpp
お仕事　　：ステージと難易度の選択

----------------------------------------------------------------------*/

#include "LevelSelectScene.h"
#include "AudioManager.h"

#include <algorithm>

LevelSelectScene::LevelSelectScene()
{
    m_menu.SetLayout(316.0f, 210.0f, 440.0f, 52.0f, 14.0f);
    RebuildMenu();
}

void LevelSelectScene::UnlockStage(int stageIndex)
{
    if (stageIndex >= 0 && stageIndex < 3)
    {
        m_stageUnlocked[stageIndex] = true;
    }

    RebuildMenu();
}

void LevelSelectScene::RebuildMenu()
{
    std::vector<MenuItem> items;

    for (int i = 0; i < 3; ++i)
    {
        MenuItem item;

        item.label = GetLevelSetting(i).displayName;
        item.next = SceneType::Game;
        item.enabled = m_stageUnlocked[i];

        if (!item.enabled)
        {
            item.label += L"   （未解放）";
        }

        items.push_back(item);
    }

    items.push_back({ L"戻る", SceneType::Start, true });

    m_menu.SetItems(items);
}

void LevelSelectScene::Reset()
{
    m_menu.Reset();
}

SceneType LevelSelectScene::Update(
    const InputTracker& input,
    float mouseX,
    float mouseY,
    float deltaTime
)
{
    //--------------------------------------------------------
    // 難易度は左右で切り替える
    //--------------------------------------------------------
    const DifficultyType beforeDifficulty = m_difficulty;
    const PlayerType beforePlayer = m_player;

    if (input.IsPressed(VK_LEFT) || input.IsPressed('A'))
    {
        const int value = static_cast<int>(m_difficulty);
        m_difficulty = static_cast<DifficultyType>((value + 2) % 3);
    }

    if (input.IsPressed(VK_RIGHT) || input.IsPressed('D'))
    {
        const int value = static_cast<int>(m_difficulty);
        m_difficulty = static_cast<DifficultyType>((value + 1) % 3);
    }

    //--------------------------------------------------------
    // キャラクターは Q / E で切り替える。
    // 未実装の枠（Player3 / 4）は飛ばす。
    //--------------------------------------------------------
    const auto stepPlayer = [this](int direction)
        {
            int index = static_cast<int>(m_player);

            for (int i = 0; i < PLAYER_TYPE_COUNT; ++i)
            {
                index = (index + direction + PLAYER_TYPE_COUNT)
                    % PLAYER_TYPE_COUNT;

                const PlayerType candidate = static_cast<PlayerType>(index);

                if (GetPlayerCharacter(candidate).available)
                {
                    m_player = candidate;
                    return;
                }
            }
        };

    if (input.IsPressed('Q'))
    {
        stepPlayer(-1);
    }

    if (input.IsPressed('E'))
    {
        stepPlayer(1);
    }

    //--------------------------------------------------------
    // 難易度やキャラクターが変わったときも、
    // カーソルを動かしたときと同じ音を出す。
    //--------------------------------------------------------
    if (m_difficulty != beforeDifficulty || m_player != beforePlayer)
    {
        Audio::PlaySe(SeId::MenuMove);
    }

    if (input.IsPressed(VK_ESCAPE))
    {
        Audio::PlaySe(SeId::MenuBack);
        return SceneType::Start;
    }

    // 選択中のステージ番号は常に反映しておく
    const int selected = m_menu.GetSelected();

    if (selected >= 0 && selected < 3)
    {
        m_selectedStage = selected;
    }

    if (!m_menu.Update(input, mouseX, mouseY, deltaTime))
    {
        return SceneType::None;
    }

    const MenuItem* item = m_menu.GetSelectedItem();

    if (!item || !item->enabled)
    {
        return SceneType::None;
    }

    if (item->next == SceneType::Game)
    {
        m_selectedStage = m_menu.GetSelected();
    }

    return item->next;
}

void LevelSelectScene::Draw(const SceneContext& context)
{
    //--------------------------------------------------------
    // 選んでいるステージの背景を出す
    //--------------------------------------------------------
    const int backdrop = std::clamp(m_selectedStage, 0, 2);

    if (context.scenery[backdrop] && context.renderTarget)
    {
        const D2D1_SIZE_F size = context.scenery[backdrop]->GetSize();

        const float height = context.screenHeight;

        const float width = (size.height > 0.0f)
            ? size.width * (height / size.height)
            : context.screenWidth;

        // 中央に寄せて描く
        const float left = (context.screenWidth - width) * 0.5f;

        context.renderTarget->DrawBitmap(
            context.scenery[backdrop],
            D2D1::RectF(left, 0.0f, left + width, height),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
        );

        // 文字が読めるよう暗く重ねる
        context.SetColor(0.02f, 0.04f, 0.07f, 0.62f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }
    else
    {
        context.SetColor(0.02f, 0.04f, 0.07f, 1.0f);
        context.FillRect(0.0f, 0.0f, context.screenWidth, context.screenHeight);
    }

    context.SetColor(0.90f, 0.95f, 1.0f, 1.0f);

    context.DrawText(
        L"ステージ選択",
        0.0f, 60.0f, context.screenWidth, 130.0f,
        context.headerFormat
    );

    m_menu.Draw(context);

    //--------------------------------------------------------
    // 右側にステージの説明を出す
    //
    // 枠の中へ収まるよう、上から順に高さを割り振っている。
    //--------------------------------------------------------
    const int stage = std::clamp(m_selectedStage, 0, 2);

    const LevelSetting& setting = GetLevelSetting(stage);

    constexpr float PANEL_LEFT = 646.0f;
    constexpr float PANEL_RIGHT = 1244.0f;
    constexpr float PANEL_TOP = 176.0f;
    constexpr float PANEL_BOTTOM = 588.0f;

    constexpr float INNER_LEFT = PANEL_LEFT + 16.0f;
    constexpr float INNER_RIGHT = PANEL_RIGHT - 16.0f;

    context.SetColor(0.04f, 0.06f, 0.10f, 0.92f);
    context.FillRoundRect(
        PANEL_LEFT, PANEL_TOP, PANEL_RIGHT, PANEL_BOTTOM, 10.0f
    );

    context.SetColor(0.25f, 0.55f, 0.85f, 0.8f);
    context.DrawRoundRect(
        PANEL_LEFT, PANEL_TOP, PANEL_RIGHT, PANEL_BOTTOM, 10.0f, 1.5f
    );

    //--------------------------------------------------------
    // ステージ名
    //--------------------------------------------------------
    context.SetColor(0.85f, 0.92f, 1.0f, 1.0f);

    context.DrawText(
        setting.displayName,
        INNER_LEFT, PANEL_TOP + 8.0f, INNER_RIGHT, PANEL_TOP + 44.0f,
        context.bodyFormat
    );

    //--------------------------------------------------------
    // 目標と目安時間
    //--------------------------------------------------------
    std::wstring body= L"目標　　　： ";
    body += setting.objective;
    body += L"\n制限時間　： ";
    body += std::to_wstring(setting.targetSeconds / 60);
    body += L" 分 ";
    body += std::to_wstring(setting.targetSeconds % 60);
    body += L" 秒\n出現レアリティ： ";
    body += setting.recommendGear;

    context.SetColor(0.78f, 0.84f, 0.90f, 1.0f);

    context.DrawText(
        body,
        INNER_LEFT, PANEL_TOP + 46.0f, INNER_RIGHT, PANEL_TOP + 126.0f,
        context.bodyFormat
    );

    //--------------------------------------------------------
    // キャラクター
    //--------------------------------------------------------
    const PlayerCharacterData& character = GetPlayerCharacter(m_player);

    context.SetColor(0.55f, 0.90f, 0.70f, 1.0f);

    std::wstring player = character.name;
    player += L"   ";
    player += character.role;

    context.DrawText(
        player,
        INNER_LEFT, PANEL_TOP + 130.0f, INNER_RIGHT, PANEL_TOP + 162.0f,
        context.bodyFormat
    );

    //--------------------------------------------------------
    // スキルは K と L を左右に並べる
    //--------------------------------------------------------
    const float skillTop = PANEL_TOP + 166.0f;
    const float skillBottom = PANEL_BOTTOM - 62.0f;
    const float skillMid = (INNER_LEFT + INNER_RIGHT) * 0.5f;

    struct SkillColumn
    {
        const wchar_t* key;
        const wchar_t* name;
        const wchar_t* text;
        float left;
        float right;
    };

    const SkillColumn columns[2] =
    {
        { L"K", character.skillName,  character.skillDescription,
          INNER_LEFT, skillMid - 6.0f },
        { L"L", character.skillName2, character.skillDescription2,
          skillMid + 6.0f, INNER_RIGHT }
    };

    for (const SkillColumn& column : columns)
    {
        if (column.name == nullptr || column.name[0] == L'\0')
        {
            continue;
        }

        context.SetColor(0.05f, 0.08f, 0.13f, 0.92f);
        context.FillRoundRect(
            column.left, skillTop, column.right, skillBottom, 8.0f
        );

        context.SetColor(0.30f, 0.62f, 0.90f, 0.75f);
        context.DrawRoundRect(
            column.left, skillTop, column.right, skillBottom, 8.0f, 1.2f
        );

        context.SetColor(1.0f, 0.86f, 0.40f, 1.0f);

        std::wstring head = L"[";
        head += column.key;
        head += L"] ";
        head += column.name;

        context.DrawText(
            head,
            column.left + 10.0f, skillTop + 6.0f,
            column.right - 8.0f, skillTop + 32.0f,
            context.bodyFormat
        );

        context.SetColor(0.80f, 0.86f, 0.92f, 1.0f);

        context.DrawText(
            column.text,
            column.left + 10.0f, skillTop + 34.0f,
            column.right - 8.0f, skillBottom - 6.0f,
            context.bodyFormat
        );
    }

    //--------------------------------------------------------
    // 難易度
    //--------------------------------------------------------
    context.SetColor(1.0f, 0.82f, 0.35f, 1.0f);

    std::wstring difficulty = L"難易度　←  ";
    difficulty += ToString(m_difficulty);
    difficulty += L"  →";

    context.DrawText(
        difficulty,
        INNER_LEFT, skillBottom + 8.0f, INNER_RIGHT, PANEL_BOTTOM - 8.0f,
        context.bodyFormat
    );

    context.SetColor(0.40f, 0.45f, 0.52f, 1.0f);

    context.DrawText(
        L"Q / E ：キャラクター選択 | ← / → ：難易度 | Enter：開始 |　Esc ：戻る",
        400.0f, 640.0f, context.screenWidth, 668.0f,
        context.bodyFormat
    );
}

/*--------------------------------------------------------------------

ファイル名：EffectSystem.h
お仕事　　：画面エフェクトの生成・更新・描画

設計書 Ver.3 19-5
    19-5.3 BulletEffect  … Muzzle Flash / Impact / Explosion
    19-5.6 PlayerEffect  … Jump Dust / Heal
    19-5.7 EnemyEffect   … Death
    19-5.9 SynergyEffect … Demolition の爆発

種類ごとにファイルを分けると中身がほぼ同じになるため、
1つのプールにまとめ、種類を EffectType で切り替えている。

共通要件（設計書 19-5）
    ・一定時間で自動的に消える
    ・数が増えすぎないよう上限を設ける
    ・ゲームロジック本体へは影響しない

----------------------------------------------------------------------*/

#ifndef EFFECTSYSTEM_H
#define EFFECTSYSTEM_H

#include <d2d1.h>
#include <wrl/client.h>

#include <vector>

class Camera;
class ResourceManager;

enum class EffectType
{
    Muzzle,      // 銃口の閃光
    Impact,      // 着弾
    Explosion,   // 爆発（Demolition / Blood Bombardment）
    Death,       // 撃破
    Dust         // 着地・ダッシュの土煙
};

struct EffectInstance
{
    EffectType type = EffectType::Impact;

    float worldX = 0.0f;
    float worldY = 0.0f;

    float scale = 1.0f;
    float rotation = 0.0f;   // 度

    float timer = 0.0f;
    float duration = 0.3f;

    // 火花の散り方を1発ごとに変えるための種
    int seed = 0;
};

class EffectSystem
{
public:
    void Initialize(ResourceManager* resources);

    // bullets.png の行（0:青 1:緑）
    void SetBulletRow(int row) { m_bulletRow = static_cast<float>(row); }

    void Reset();

    void Spawn(
        EffectType type,
        float worldX,
        float worldY,
        float scale = 1.0f,
        float rotation = 0.0f
    );

    void Update(float deltaTime);

    void Draw(
        ID2D1RenderTarget* renderTarget,
        const Camera& camera
    ) const;

    int GetCount() const { return static_cast<int>(m_effects.size()); }

private:
    //--------------------------------------------------------
    // 着弾の火花を図形で描く
    //   画像に頼らないので、effects.png が読めなくても
    //   当たったことは必ず分かる。
    //--------------------------------------------------------
    void DrawImpactBurst(
        ID2D1RenderTarget* renderTarget,
        const EffectInstance& effect,
        float screenX,
        float t
    ) const;

    // 図形描画に使う共有ブラシ（初回の Draw で作る）
    ID2D1SolidColorBrush* AcquireBrush(
        ID2D1RenderTarget* renderTarget
    ) const;

private:
    // 画面が埋まらないよう上限を設ける
    static constexpr int MAX_EFFECTS = 96;

    ResourceManager* m_resources = nullptr;
    float m_bulletRow = 0.0f;

    std::vector<EffectInstance> m_effects;

    // Spawn のたびに増やして seed に使う
    int m_seedCounter = 0;

    mutable Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
    mutable ID2D1RenderTarget* m_brushOwner = nullptr;
};

#endif

#include "EffectManager.h"

EffectManager::EffectManager()
{
}
EffectManager::~EffectManager()
{
    Clear();
}

void EffectManager::AddEffect(Effect *effect)
{
    effects.push_back(effect);
}

void EffectManager::Update()
{
    auto it = effects.begin();
    while (it != effects.end())
    {
        (*it)->Update();
        if ((*it)->IsDead())
        {
            delete *it;
            it = effects.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void EffectManager::Draw()
{
    for (auto eff : effects)
    {
        eff->Draw();
    }
}

void EffectManager::Clear()
{
    for (auto eff : effects)
    {
        delete eff;
    }
    effects.clear();
}

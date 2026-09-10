import re

def process_file(path):
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()

    if '#include "Camera.h"' not in content:
        content = '#include "Camera.h"\n' + content

    content = re.sub(r'float screenX = 960\.0f \+ \([a-zA-Z0-9_\.]+\.x - pPos\.x\) \* zoomScale;', 'float screenX = Camera::WorldToScreenX(position.x);', content)
    content = re.sub(r'float screenY = 540\.0f \+ \([a-zA-Z0-9_\.]+\.y - pPos\.y\) \* zoomScale;', 'float screenY = Camera::WorldToScreenY(position.y);', content)
    
    # for EffectManager
    content = re.sub(r'float screenX = 960\.0f \+ \(x - pPos\.x\) \* zoomScale;', 'float screenX = Camera::WorldToScreenX(x);', content)
    content = re.sub(r'float screenY = 540\.0f \+ \(y - pPos\.y\) \* zoomScale;', 'float screenY = Camera::WorldToScreenY(y);', content)
    content = re.sub(r'float screenX2 = 960\.0f \+ \(effect\.x2 - pPos\.x\) \* zoomScale;', 'float screenX2 = Camera::WorldToScreenX(effect.x2);', content)
    content = re.sub(r'float screenY2 = 540\.0f \+ \(effect\.y2 - pPos\.y\) \* zoomScale;', 'float screenY2 = Camera::WorldToScreenY(effect.y2);', content)
    
    # Enemy hitScreen
    content = re.sub(r'float hitScreenX = 960\.0f \+ \(hitPos\.x - pPos\.x\) \* zoomScale;', 'float hitScreenX = Camera::WorldToScreenX(hitPos.x);', content)
    content = re.sub(r'float hitScreenY = 540\.0f \+ \(hitPos\.y - pPos\.y\) \* zoomScale;', 'float hitScreenY = Camera::WorldToScreenY(hitPos.y);', content)

    # Clean up unused pPos stuff
    if 'Vector2 pPos = targetPlayer->GetPosition();' in content:
        content = re.sub(r'float zoomScale = 75\.0f / cellSize;\n\s*Vector2 pPos = targetPlayer->GetPosition\(\);\n', '', content)
        
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

process_file('TeamGame/Source/Characters/Enemy.cpp')
process_file('TeamGame/Source/Weapons/Bullet.cpp')
process_file('TeamGame/Source/Weapons/EnemyBullet.cpp')
process_file('TeamGame/Source/Managers/EffectManager.cpp')

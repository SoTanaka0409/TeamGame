import re

with open('TeamGame/Source/Characters/Player.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

content = '#include "Camera.h"\n' + content

# Mouse logic
content = content.replace('float dx = mouseX - 960.0f;', 'float dx = mouseX - Camera::WorldToScreenX(position.x);')
content = content.replace('float dy = mouseY - 540.0f;', 'float dy = mouseY - Camera::WorldToScreenY(position.y);')

# Draw
content = content.replace('    float screenX = 960.0f;\n    float screenY = 540.0f;', '    float screenX = Camera::WorldToScreenX(position.x);\n    float screenY = Camera::WorldToScreenY(position.y);')

# hitScreenX / eScreenX
content = re.sub(r'float hitScreenX = 960\.0f \+ \(hitPos\.x \- position\.x\) \* zoomScale;', 'float hitScreenX = Camera::WorldToScreenX(hitPos.x);', content)
content = re.sub(r'float hitScreenY = 540\.0f \+ \(hitPos\.y \- position\.y\) \* zoomScale;', 'float hitScreenY = Camera::WorldToScreenY(hitPos.y);', content)

content = re.sub(r'float eScreenX = 960\.0f \+ \(ePos\.x \- position\.x\) \* zoomScale;', 'float eScreenX = Camera::WorldToScreenX(ePos.x);', content)
content = re.sub(r'float eScreenY = 540\.0f \+ \(ePos\.y \- position\.y\) \* zoomScale;', 'float eScreenY = Camera::WorldToScreenY(ePos.y);', content)

# playerPixelX
content = content.replace('    float playerPixelX = 960.0f;\n    float playerPixelY = 540.0f;', '    float playerPixelX = Camera::WorldToScreenX(position.x);\n    float playerPixelY = Camera::WorldToScreenY(position.y);')

# worldX raycast
content = content.replace('float worldX = position.x + (currPx - 960.0f) / zoomScale;', 'float worldX = Camera::ScreenToWorldX(currPx);')
content = content.replace('float worldY = position.y + (currPy - 540.0f) / zoomScale;', 'float worldY = Camera::ScreenToWorldY(currPy);')

with open('TeamGame/Source/Characters/Player.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

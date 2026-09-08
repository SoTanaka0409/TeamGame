import re

with open('TeamGame/TeamGame.vcxproj', 'r', encoding='utf-8') as f:
    content = f.read()

# Add ClCompile
if 'Source\\Stage\\StageManager.cpp' not in content:
    content = content.replace(
        '<ClCompile Include="Source\\Weapons\\Shotgun.cpp">',
        '<ClCompile Include="Source\\Weapons\\EnemyBullet.cpp"><AdditionalOptions>/utf-8 /wd4828 /wd4010 %(AdditionalOptions)</AdditionalOptions></ClCompile><ClCompile Include="Source\\Stage\\StageManager.cpp"><AdditionalOptions>/utf-8 /wd4828 /wd4010 %(AdditionalOptions)</AdditionalOptions></ClCompile><ClCompile Include="Source\\Weapons\\Shotgun.cpp">'
    )

# Add ClInclude
if 'Source\\Stage\\StageManager.h' not in content:
    content = content.replace(
        '<ClInclude Include="Source\\Weapons\\Shotgun.h" />',
        '<ClInclude Include="Source\\Weapons\\EnemyBullet.h" /><ClInclude Include="Source\\Stage\\StageManager.h" /><ClInclude Include="Source\\Weapons\\Shotgun.h" />'
    )

with open('TeamGame/TeamGame.vcxproj', 'w', encoding='utf-8') as f:
    f.write(content)

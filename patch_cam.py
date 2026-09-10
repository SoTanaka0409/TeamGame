import re

with open('TeamGame/TeamGame.vcxproj', 'r', encoding='utf-8') as f:
    content = f.read()

if 'Source\\Core\\Camera.cpp' not in content:
    content = content.replace(
        '<ClCompile Include="Source\\Core\\main.cpp">',
        '<ClCompile Include="Source\\Core\\Camera.cpp"><AdditionalOptions>/utf-8 /wd4828 /wd4010 %(AdditionalOptions)</AdditionalOptions></ClCompile><ClCompile Include="Source\\Core\\main.cpp">'
    )

if 'Source\\Core\\Camera.h' not in content:
    content = content.replace(
        '<ClInclude Include="Source\\Core\\Vector2.h" />',
        '<ClInclude Include="Source\\Core\\Camera.h" /><ClInclude Include="Source\\Core\\Vector2.h" />'
    )

with open('TeamGame/TeamGame.vcxproj', 'w', encoding='utf-8') as f:
    f.write(content)

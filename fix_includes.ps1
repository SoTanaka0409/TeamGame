$content = Get-Content TeamGame\Source\Scenes\GameScene.cpp -Raw
$includes = "
#include "GameScene.h"
#include "DxLib.h"
#include "Enemy.h"
#include "InputManager.h"
#include "Player.h"
#include "ResultScene.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "NetworkManager.h"
#include "PacketTypes.h"
#include "GameSettings.h"
#include <algorithm>
#include <cmath>
#include <random>
"
$content = $content -replace '(?s)#include "GameScene\.h".*?<random>', $includes
Set-Content TeamGame\Source\Scenes\GameScene.cpp $content

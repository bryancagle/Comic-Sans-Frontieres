/// \file main.cpp 
/// \main file for Comic Sans Frontieres

#include <windows.h>
#include <windowsx.h>

#include "defines.h"
#include "abort.h"
#include "gamerenderer.h"
#include "imagefilenamelist.h"
#include "debug.h"
#include "timer.h"
#include "sprite.h"
#include "object.h"
#include "spriteman.h"
#include "objman.h"
#include "Random.h"
#include "sound.h"

//globals
GameStateType g_nGameState; ///< game states to be used for menus/gameplay/etc.
LevelStateType g_nLevelState; ///< tells the game what stage the player is currently in
BOOL g_bActiveApp;  ///< TRUE if this is the active application
HWND g_HwndApp; ///< Application window handle.
HINSTANCE g_hInstance; ///< Application instance handle.
char g_szGameName[256]; ///< Name of this game.
char g_szShaderModel[256]; ///< The shader model version used.
CImageFileNameList g_cImageFileName; ///< List of image file names.
CTimer g_cTimer; ///< The game timer.
CSpriteManager g_cSpriteManager; ///< The sprite manager.
CObjectManager g_cObjectManager; ///< The object manager.
CRandom g_cRandom; ///< The random number generator.
CSoundManager* g_pSoundManager; ///< The sound manager.

//graphics settings
int g_nScreenWidth; ///< Screen width.
int g_nScreenHeight; ///< Screen height.
BOOL g_bWireFrame = FALSE; ///< TRUE for wireframe rendering.
ShaderType g_nPixelShader = NULL_SHADER; ///< Pixel shader in use.

BOOL g_bDarkenScreen = FALSE; ///< If the screen is darkened, AKA shift is held down when player has all letters
BOOL g_bShieldActive = FALSE; ///< Tell when shield is activated
BOOL g_bAssistActive = FALSE; ///< Tell when assist is activated
BOOL g_bPlayerTyped[4]; ///< Checks if each subsequent letter in a special attack is typed
BOOL g_bSpecialAttackReleased = FALSE; ///< Used to check if special attack has been used
BOOL g_bSpecialActivate = FALSE; ///< Used to tell if player is able to activate special attack
float g_fScreenScroll = g_nScreenWidth / 2.0f; ///< Finds the current screen scroll value
CGameObject* player; ///< Pointer used to keep track of player
BOOL g_bPlayerIsInvulnerable = FALSE; ///< TRUE if player is invulnerable to collisions.

int cursorPos = 0; ///< Current position on the menu for cursor menus.
int cursorPosMax = 4; ///< Maximum number of menu spaces that can be moved on a menu.
int charSelect = 1; ///< Used to determine which character player has selected.

//XML settings
tinyxml2::XMLDocument g_xmlDocument; ///< TinyXML document for settings.
XMLElement* g_xmlSettings = nullptr; ///< TinyXML element for settings tag.

//debug variables
#ifdef DEBUG_ON
  CDebugManager g_cDebugManager; ///< The debug manager.
#endif //DEBUG_ON

CGameRenderer GameRenderer; ///< The game renderer.

//functions in Window.cpp
void InitGraphics();
HWND CreateDefaultWindow(char* name, HINSTANCE hInstance, int nCmdShow);

//function prototypes
void InitGame();
void BeginGame();
int DefaultWinMain(HINSTANCE, HINSTANCE, LPSTR, int); ///< Default WinMain.

/// \brief Initialize XML settings.
///
/// Open an XML file and prepare to read settings from it. Settings
/// tag is loaded to XML element g_xmlSettings for later pfrocessing. Abort if it
/// cannot load the file or cannot find settings tag in loaded file.

void InitXMLSettings(){
  //open and load XML file
  const char* xmlFileName = "gamesettings.xml"; //Settings file name.
  if(g_xmlDocument.LoadFile(xmlFileName) != 0)
    ABORT("Cannot load settings file %s.", xmlFileName);

  //get settings tag
  g_xmlSettings = g_xmlDocument.FirstChildElement("settings"); //settings tag
  if(g_xmlSettings == nullptr) //abort if tag not found
    ABORT("Cannot find <settings> tag in %s.", xmlFileName);
} //InitXMLSettings

/// \brief Load game settings.
///
/// Load and parse game settings from a TinyXML element g_xmlSettings.

void LoadGameSettings(){
  if(!g_xmlSettings)return; //bail and fail

  //get game name
  XMLElement* ist = g_xmlSettings->FirstChildElement("game"); 
  if(ist){
    size_t len = strlen(ist->Attribute("name")); //length of name string
    strncpy_s(g_szGameName, len+1, ist->Attribute("name"), len); 
  } //if

  //get renderer settings
  XMLElement* renderSettings = 
    g_xmlSettings->FirstChildElement("renderer"); //renderer tag
  if(renderSettings){ //read renderer tag attributes
    g_nScreenWidth = renderSettings->IntAttribute("width");
    g_nScreenHeight = renderSettings->IntAttribute("height");

    size_t len = strlen(renderSettings->Attribute("shadermodel")); //length shader model string
    strncpy_s(g_szShaderModel, len + 1, renderSettings->Attribute("shadermodel"), len);
  } //if

  //get image file names
  g_cImageFileName.GetImageFileNames(g_xmlSettings);

  //get debug settings
  #ifdef DEBUG_ON
    g_cDebugManager.GetDebugSettings(g_xmlSettings);
  #endif //DEBUG_ON
} //LoadGameSettings

int MessageLoop() {
	MSG msg; //current message

	while (TRUE)
		if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) { //if message waiting
			if (!GetMessage(&msg, nullptr, 0, 0))return (int)msg.wParam; //get it
			TranslateMessage(&msg); DispatchMessage(&msg); //translate it
		} //if
		else GameRenderer.ProcessFrame();

		return 0;
} //MessageLoop


void InitGame() {
	InitGraphics(); //initialize graphics
	GameRenderer.LoadTextures(); //load images
	g_nGameState = TITLE_GAMESTATE;
	g_nLevelState = NONE_STATE;
	g_pSoundManager = new CSoundManager();
	g_pSoundManager->Load(); //load game sounds
	g_pSoundManager->loop(ORGANDONOR_SOUND);
} //InitGame

///Starts the music for the respective level and creates the object
///Also resets basic stats like health and score

void BeginGame() {
	g_cObjectManager.ResetPlayerStats();
	g_nGameState = PLAYING_GAMESTATE;
	if(g_nLevelState == COMICWORLD_STATE) g_pSoundManager->loop(UNLEASH_SOUND);
	else if (g_nLevelState == FANTASY_STATE) g_pSoundManager->loop(MOOSEHEADHONK_SOUND);
	else if (g_nLevelState == CITY_STATE) g_pSoundManager->loop(HUBBUB_SOUND);
	g_cTimer.StartLevelTimer();
	g_cObjectManager.clear();
	CreateObjects();
} //BeginGame

/// \brief Create game objects. 
///
/// Create a player and basic stats

void CreateObjects(){
  //the plane Z - maybe unused after Z-axis changes?
  const int nPlaneZ = 375;

  ///Creates and sets the player character depending on what the player has chosen
  if (charSelect == 1) {
	  player = g_cObjectManager.createObject(FREDIDLE_OBJECT, "fredIdle",
		  Vector3(790 / 2.0f, 612 / 2.0f, (float)nPlaneZ), Vector3(0, 0, 0));
	  g_cObjectManager.setPlayer(6, 1.0);
  }

  if (charSelect == 2) {
	  player = g_cObjectManager.createObject(SWAZIDLE_OBJECT, "swazIdle",
		  Vector3(790 / 2.0f, 612 / 2.0f, (float)nPlaneZ), Vector3(0, 0, 0));
	  g_cObjectManager.setPlayer(8, 0.75);
  }

  if (charSelect == 3) {
	  player = g_cObjectManager.createObject(POLKIDLE_OBJECT, "polkIdle",
		  Vector3(790 / 2.0f, 612 / 2.0f, (float)nPlaneZ), Vector3(0, 0, 0));
	  g_cObjectManager.setPlayer(4, 1.25);
  }

  ///Inserting objects into the object list
  g_cObjectManager.InsertObjectType("fredIdle", FREDIDLE_OBJECT);
	g_cObjectManager.InsertObjectType("fredAttack", FREDATTACK_OBJECT);
	g_cObjectManager.InsertObjectType("fredHurt", FREDHURT_OBJECT);
	g_cObjectManager.InsertObjectType("fredLost", FREDLOST_OBJECT);

  g_cObjectManager.InsertObjectType("swazIdle", SWAZIDLE_OBJECT);
  g_cObjectManager.InsertObjectType("swazAttack", SWAZATTACK_OBJECT);
  g_cObjectManager.InsertObjectType("swazHurt", SWAZHURT_OBJECT);
  g_cObjectManager.InsertObjectType("swazLost", SWAZLOST_OBJECT);

  g_cObjectManager.InsertObjectType("polkIdle", POLKIDLE_OBJECT);
  g_cObjectManager.InsertObjectType("polkAttack", POLKATTACK_OBJECT);
  g_cObjectManager.InsertObjectType("polkHurt", POLKHURT_OBJECT);
  g_cObjectManager.InsertObjectType("polkLost", POLKLOST_OBJECT);

	g_cObjectManager.InsertObjectType("enemyEntry", ENEMYENTRY_OBJECT);
	g_cObjectManager.InsertObjectType("enemyExit", ENEMYEXIT_OBJECT);
	g_cObjectManager.InsertObjectType("enemy1Idle", ENEMY1IDLE_OBJECT);
	g_cObjectManager.InsertObjectType("enemy1IdleAfter", ENEMY1AFTER_OBJECT);
	g_cObjectManager.InsertObjectType("enemy1Hurt", ENEMY1HURT_OBJECT);

	g_cObjectManager.InsertObjectType("enemyInvaderIdle", ENEMYINVADERIDLE_OBJECT);
	g_cObjectManager.InsertObjectType("enemyInvaderAttack", ENEMYINVADERATTACK_OBJECT);
	g_cObjectManager.InsertObjectType("enemyInvaderHurt", ENEMYINVADERHURT_OBJECT);

	g_cObjectManager.InsertObjectType("enemyZoomerIdle", ENEMYZOOMERIDLE_OBJECT);
	g_cObjectManager.InsertObjectType("enemyZoomerBounce", ENEMYZOOMERBOUNCE_OBJECT);
	g_cObjectManager.InsertObjectType("enemyZoomerIdleFlip", ENEMYZOOMERIDLEFLIP_OBJECT);
	g_cObjectManager.InsertObjectType("enemyZoomerBounceFlip", ENEMYZOOMERBOUNCEFLIP_OBJECT);
	g_cObjectManager.InsertObjectType("enemyZoomerHurt", ENEMYZOOMERHURT_OBJECT);

	g_cObjectManager.InsertObjectType("enemyThiefIdle", ENEMYTHIEFIDLE_OBJECT);
	g_cObjectManager.InsertObjectType("enemyThiefAttack", ENEMYTHIEFATTACK_OBJECT);
	g_cObjectManager.InsertObjectType("enemyThiefHurt", ENEMYTHIEFHURT_OBJECT);

	g_cObjectManager.InsertObjectType("projectileF", PROJECTILEF_OBJECT);
	g_cObjectManager.InsertObjectType("projectileS", PROJECTILES_OBJECT);
	g_cObjectManager.InsertObjectType("projectileP", PROJECTILEP_OBJECT);
	g_cObjectManager.InsertObjectType("projectileD", PROJECTILED_OBJECT);
	g_cObjectManager.InsertObjectType("projectileZ", PROJECTILEZ_OBJECT);
	g_cObjectManager.InsertObjectType("projectileK", PROJECTILEK_OBJECT);
	g_cObjectManager.InsertObjectType("shield", SHIELD_OBJECT);
	g_cObjectManager.InsertObjectType("assistFred", ASSISTFRED_OBJECT);
	g_cObjectManager.InsertObjectType("assistSwaz", ASSISTSWAZ_OBJECT);
	g_cObjectManager.InsertObjectType("assistPolk", ASSISTPOLK_OBJECT);

	g_cObjectManager.InsertObjectType("projectileEnemy1", PROJECTILEENEMY1_OBJECT);
	g_cObjectManager.InsertObjectType("projectileInvader", PROJECTILEINVADER_OBJECT);
	g_cObjectManager.InsertObjectType("projectileThief", PROJECTILETHIEF_OBJECT);

	g_cObjectManager.InsertObjectType("life full", LIFEFULL_OBJECT);
	g_cObjectManager.InsertObjectType("life half", LIFEHALF_OBJECT);
	g_cObjectManager.InsertObjectType("life empty", LIFEEMPTY_OBJECT);

  g_cObjectManager.InsertObjectType("itemHealth", ITEMHEART_OBJECT);
  g_cObjectManager.InsertObjectType("itemLetterR", ITEMR_OBJECT);
  g_cObjectManager.InsertObjectType("itemLetterE", ITEME_OBJECT);
  g_cObjectManager.InsertObjectType("itemLetterD", ITEMD_OBJECT);

  g_cObjectManager.InsertObjectType("itemLetterW", ITEMW_OBJECT);
  g_cObjectManager.InsertObjectType("itemLetterA", ITEMA_OBJECT);
  g_cObjectManager.InsertObjectType("itemLetterZ", ITEMZ_OBJECT);

  g_cObjectManager.InsertObjectType("itemLetterO", ITEMO_OBJECT);
  g_cObjectManager.InsertObjectType("itemLetterL", ITEML_OBJECT);
  g_cObjectManager.InsertObjectType("itemLetterK", ITEMK_OBJECT);
} //CreateObjects

/// \brief Keyboard handler.
///
/// Handler for keyboard messages from the Windows API. Takes the appropriate
/// action when the user presses a key on the keyboard.
/// \param keystroke Virtual key code for the key pressed
/// \return TRUE if the game is to exit

BOOL KeyboardHandler(WPARAM keystroke){
	int *ammoCount = g_cObjectManager.getAmmoCount();
	switch (keystroke) {
	case VK_ESCAPE: //exit game
		return TRUE; //exit keyboard handler
		break;

	case VK_UP:
		if(g_nGameState == PLAYING_GAMESTATE){	//When in the playing state, this is used to move the player
			if(player)
				player->m_vVelocity.y = 5.0f * g_cObjectManager.getPlayerSpeed();
		}
		else if(g_nGameState == MENU_GAMESTATE || g_nGameState == CHARSELECT_GAMESTATE){	//When in a menu state, this is used to move menu
			if(cursorPos == 0)
				cursorPos = cursorPosMax;
			else
				cursorPos--;
			g_pSoundManager->play(MOVECURSOR_SOUND);
		}
		break;

	//when in playing state, this moves the player. otherwise, it moves the menu
	case VK_DOWN:
		if(g_nGameState == PLAYING_GAMESTATE){
			if(player)
				player->m_vVelocity.y = -5.0f * g_cObjectManager.getPlayerSpeed();
		}
		else if(g_nGameState == MENU_GAMESTATE || g_nGameState == CHARSELECT_GAMESTATE){
			if(cursorPos == cursorPosMax)
				cursorPos = 0;
			else
				cursorPos++;
			g_pSoundManager->play(MOVECURSOR_SOUND);
		}
		break;

	//when in playing state, this moves the player. otherwise, it moves the menu
	case VK_LEFT:
		if(g_nGameState == PLAYING_GAMESTATE){
			if(player)
				player->m_vVelocity.x = -5.0f * g_cObjectManager.getPlayerSpeed();
		}
		else if(g_nGameState == MENU_GAMESTATE || g_nGameState == CHARSELECT_GAMESTATE){
			if(cursorPos == 0)
				cursorPos = cursorPosMax;
			else
				cursorPos--;
			g_pSoundManager->play(MOVECURSOR_SOUND);
		}
		break;
	
	//when in playing state, this moves the player. otherwise, it moves the menu
	case VK_RIGHT:
		if(g_nGameState == PLAYING_GAMESTATE){
			if(player)
				player->m_vVelocity.x = 6.0f * g_cObjectManager.getPlayerSpeed();
		}
		else if(g_nGameState == MENU_GAMESTATE || g_nGameState == CHARSELECT_GAMESTATE){
			if(cursorPos == cursorPosMax)
				cursorPos = 0;
			else
				cursorPos++;
			g_pSoundManager->play(MOVECURSOR_SOUND);
		}
		break;

	//Used to start special attack
	case VK_SHIFT:
		if(g_nGameState == PLAYING_GAMESTATE) g_bDarkenScreen = TRUE;
		break;

	//fires shot if player is fred, also used for special attack
	case 'F':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (!g_bDarkenScreen && g_nGameState == PLAYING_GAMESTATE && (player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT)) {
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(FREDATTACK_OBJECT, "fredAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.FireGun();
			}
			else if (g_bDarkenScreen && g_nGameState == PLAYING_GAMESTATE && player->m_nObjectType != FREDHURT_OBJECT) {
				g_bPlayerTyped[0] = TRUE;
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//activates shield if player is fred, also used for special attack
	case 'R':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (g_bDarkenScreen && (player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT)) {
				g_bPlayerTyped[1] = TRUE;
			}
			if (!g_bDarkenScreen && ammoCount[0] > 0 && g_bShieldActive == FALSE && (player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT)) {
				g_bShieldActive = true;
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(FREDATTACK_OBJECT, "fredAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.CreateShield();
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//activate assist if player is fred, also used for special attack
	case 'E':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (g_bDarkenScreen && (player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT)) {
				g_bPlayerTyped[2] = TRUE;
			}
			if (!g_bDarkenScreen && ammoCount[1] > 0 && g_bAssistActive == FALSE && (player->m_nObjectType == FREDIDLE_OBJECT)) {
				g_bAssistActive = true;
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(FREDATTACK_OBJECT, "fredAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.CreateAssist();
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//fires pierce shot if player is fred, also used for special attack
	case 'D':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (g_bDarkenScreen && (player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT)) {
				g_bPlayerTyped[3] = TRUE;
			}
			if (!g_bDarkenScreen && ammoCount[2] > 0 && (player->m_nObjectType == FREDIDLE_OBJECT)) {
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(FREDATTACK_OBJECT, "fredAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.FirePierce();
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//fires shot if player is swaz, also used for special attack
	case 'S':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (!g_bDarkenScreen && g_nGameState == PLAYING_GAMESTATE && (player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT)) {
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(SWAZATTACK_OBJECT, "swazAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.FireGun();
			}
			else if (g_bDarkenScreen && g_nGameState == PLAYING_GAMESTATE && player->m_nObjectType != SWAZHURT_OBJECT) {
				g_bPlayerTyped[0] = TRUE;
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//activates shield if player is swaz, also used for special attack
	case 'W':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (g_bDarkenScreen && player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT) {
				g_bPlayerTyped[1] = TRUE;
			}
			if (!g_bDarkenScreen && ammoCount[0] > 0 && g_bShieldActive == FALSE && (player->m_nObjectType == SWAZIDLE_OBJECT)) {
				g_bShieldActive = true;
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(SWAZATTACK_OBJECT, "swazAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.CreateShield();
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//activates assist if player is swaz, also used for special attack
	case 'A':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (g_bDarkenScreen && player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT) {
				g_bPlayerTyped[2] = TRUE;
			}
			if (!g_bDarkenScreen && ammoCount[1] > 0 && g_bAssistActive == FALSE && (player->m_nObjectType == SWAZIDLE_OBJECT)) {
				g_bAssistActive = true;
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(SWAZATTACK_OBJECT, "swazAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.CreateAssist();
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//fires pierce shot if player is swaz, also used for special attack
	case 'Z':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (g_bDarkenScreen && player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT) {
				g_bPlayerTyped[3] = TRUE;
			}
			if (!g_bDarkenScreen && ammoCount[2] > 0 && (player->m_nObjectType == SWAZIDLE_OBJECT)) {
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(SWAZATTACK_OBJECT, "swazAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.FirePierce();
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//fires shot if player is polk, also used for special attack
	case 'P':
		if (g_nGameState == PLAYING_GAMESTATE) {
			if (!g_bDarkenScreen && g_nGameState == PLAYING_GAMESTATE && (player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT)) {
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(POLKATTACK_OBJECT, "polkAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.FireGun();
			}
			else if (g_bDarkenScreen && g_nGameState == PLAYING_GAMESTATE && player->m_nObjectType != POLKHURT_OBJECT) {
				g_bPlayerTyped[0] = TRUE;
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//activates shield if player is polk, also used for special attack
	case 'O':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (g_bDarkenScreen && player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT) {
				g_bPlayerTyped[1] = TRUE;
			}
			if (!g_bDarkenScreen && ammoCount[0] > 0 && g_bShieldActive == FALSE && (player->m_nObjectType == POLKIDLE_OBJECT)) {
				g_bShieldActive = true;
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(POLKATTACK_OBJECT, "polkAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.CreateShield();
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//activates assist if player is polk, also used for special attack
	case 'L':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (g_bDarkenScreen && player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT) {
				g_bPlayerTyped[2] = TRUE;
			}
			if (!g_bDarkenScreen && ammoCount[1] > 0 && g_bAssistActive == FALSE && (player->m_nObjectType == POLKIDLE_OBJECT)) {
				g_bAssistActive = true;
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(POLKATTACK_OBJECT, "polkAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.CreateAssist();
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//fires pierce shot if player is polk, also used for special attack
	case 'K':
		if(g_nGameState == PLAYING_GAMESTATE){
			if (g_bDarkenScreen && player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT) {
				g_bPlayerTyped[3] = TRUE;
			}
			if (!g_bDarkenScreen && ammoCount[2] > 0 && (player->m_nObjectType == POLKIDLE_OBJECT)) {
				player->kill();
				g_cObjectManager.DeleteNameToObject();
				player = g_cObjectManager.createObject(POLKATTACK_OBJECT, "polkAttack", player->m_vPos, player->m_vVelocity);
				g_cObjectManager.FirePierce();
			}
			g_cObjectManager.GarbageCollect();
		}
		break;

	//return to main menu
	case VK_BACK:
		if(g_nGameState == CHARSELECT_GAMESTATE){
			g_pSoundManager->play(MENUSELECT_SOUND);
			g_nGameState = MENU_GAMESTATE;
			cursorPosMax = 3;
			cursorPos = 0;
		}
		else if(g_nGameState == CREDITS_GAMESTATE){
			g_pSoundManager->play(MENUSELECT_SOUND);
			g_nGameState = MENU_GAMESTATE;
		}
		else if(g_nGameState == HOWTOPLAY_GAMESTATE){
			g_pSoundManager->play(MENUSELECT_SOUND);
			g_nGameState = MENU_GAMESTATE;
		}
		break;

	//activates special attack; advances screens and menus
	case VK_SPACE:
		if(g_bSpecialActivate && g_bDarkenScreen){
			g_cObjectManager.SpecialAttack();
			g_bSpecialAttackReleased = TRUE;
		}
		if(g_nGameState == ENDING_GAMESTATE){
			g_nGameState = TITLE_GAMESTATE;
			g_pSoundManager->stop(ENDING_SOUND);
			g_pSoundManager->loop(ORGANDONOR_SOUND);
			g_fScreenScroll = g_nScreenWidth / 2.0f;
			break;
		}
		if(g_nGameState == GAMEOVER_GAMESTATE){
			g_fScreenScroll = g_nScreenWidth / 2.0f;
			BeginGame();
		}
		if(g_nGameState == TITLE_GAMESTATE){
			g_pSoundManager->play(GAMESTART_SOUND);
			g_nGameState = MENU_GAMESTATE;
			cursorPos = 0;
			cursorPosMax = 3;
			break;
		}
		if(g_nGameState == MENU_GAMESTATE){
			g_pSoundManager->play(MENUSELECT_SOUND);
			if (cursorPos == 0){
				g_nGameState = CHARSELECT_GAMESTATE;
				cursorPos = 0;
				cursorPosMax = 2;
				break;
			}
			if(cursorPos == 1){ //how to play
				g_nGameState = HOWTOPLAY_GAMESTATE;
				break;
			}
			if(cursorPos == 2){ //credits
				g_nGameState = CREDITS_GAMESTATE;
				break;
			}
			if (cursorPos == 3){ //exit game
				return TRUE; //exit keyboard handler
				break;
			}		
		}
			
		if(g_nGameState == CHARSELECT_GAMESTATE){
			g_pSoundManager->play(MENUSELECT_SOUND);
			g_pSoundManager->stop(ORGANDONOR_SOUND);
			g_nLevelState = COMICWORLD_STATE;
			if(cursorPos == 0){
				charSelect = 1;
				BeginGame();
				break;
			}
			if (cursorPos == 1){
				charSelect = 2;
				BeginGame();
				break;
			}
			if (cursorPos == 2){
				charSelect = 3;
				BeginGame();
				break;
			}
		}

		if(g_nGameState == WON_GAMESTATE){
			g_bAssistActive = FALSE;
			g_bShieldActive = FALSE;
			if(g_nLevelState == COMICWORLD_STATE){
					g_nLevelState = FANTASY_STATE;
			}
			else if(g_nLevelState == FANTASY_STATE){
					g_nLevelState = CITY_STATE;
			}
			else if(g_nLevelState == CITY_STATE){
				g_nGameState = ENDING_GAMESTATE;
				g_pSoundManager->stop(STEPPINOUT_SOUND);
				g_pSoundManager->play(ENDING_SOUND);
				g_fScreenScroll = g_nScreenWidth / 2.0f;
				break;
			}
			g_pSoundManager->stop(STEPPINOUT_SOUND);
			g_fScreenScroll = g_nScreenWidth / 2.0f;
			BeginGame();
		}			
		break;
  } //switch

  return FALSE; //normal exit
} //KeyboardHandler

/// \brief Keyboard handler.
///
/// Handler for keyboard messages from the Windows API. Takes the appropriate
/// action when the user presses a key on the keyboard.
/// \param keystroke Virtual key code for the key pressed
/// \return TRUE if the game is to exit

BOOL KeyboardHandlerReleased(WPARAM keystroke){
	switch(keystroke) {
		case VK_UP:
			if (player && player->m_vVelocity.y > 0)
				player->m_vVelocity.y = 0;
			break;
		case VK_DOWN:
			if (player && player->m_vVelocity.y < 0)
				player->m_vVelocity.y = 0;
			break;
		case VK_LEFT:
			if (player && player->m_vVelocity.x < 0)
				player->m_vVelocity.x = 0;
			break;
		case VK_RIGHT:
			if (player && player->m_vVelocity.x > 0)
				player->m_vVelocity.x = 0;
			break;
		case VK_SHIFT:
			g_bDarkenScreen = FALSE;
			g_bPlayerTyped[0] = FALSE;
			g_bPlayerTyped[1] = FALSE;
			g_bPlayerTyped[2] = FALSE;
			g_bPlayerTyped[3] = FALSE;
			break;
	} //switch

	return FALSE; //normal exit
} //KeyboardHandlerReleased

/// \brief Window procedure.
///
/// Handler for messages from the Windows API. 
/// \param hwnd Window handle
/// \param message Message code
/// \param wParam Parameter for message 
/// \param lParam Second parameter for message
/// \return 0 if message is handled

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
  switch(message){ //handle message
    case WM_ACTIVATEAPP: g_bActiveApp = (BOOL)wParam; break; //iconize

    case WM_KEYDOWN: //keyboard hit
      if(KeyboardHandler(wParam))DestroyWindow(hwnd);
      break;

		case WM_KEYUP:
			if(KeyboardHandlerReleased(wParam))DestroyWindow(hwnd);
			break;

    case WM_DESTROY: //on exit
      GameRenderer.Release(); //release textures
      SAFE_DELETE(g_pSoundManager); //delete sound manager
      PostQuitMessage(0); //this is the last thing to do on exit
      break;

    default: //default window procedure
      return DefWindowProc(hwnd, message, wParam, lParam);
  } //switch(message)

  return 0;
} //WindowProc

/// \brief Default WinMain.  
/// The main entry point for this application should call this function first. 
///  \param h handle to the current instance of this application
///  \param ph unused - deprecated by Microsoft
///  \param s unused - command line string
///  \param show specifies how the window is to be shown
///  \return TRUE if application terminates correctly

BOOL DefaultWinMain(HINSTANCE h, HINSTANCE ph, LPSTR s, int show) {
#ifdef DEBUG_ON
	g_cDebugManager.open(); //open debug streams, settings came from XML file
#endif //DEBUG_ON

	InitXMLSettings(); //initialize XML settings reader
	LoadGameSettings(); //load game settings from xml file

	HWND hwnd = CreateDefaultWindow(g_szGameName, h, show); //create fullscreen window
	if (!hwnd)return FALSE; //bail if problem creating window
	g_HwndApp = hwnd; //save window handle

	return TRUE;
} //DefaultWinMain
                         
/// \brief Winmain. 
///
/// Main entry point for this application. 
/// \param hInst Handle to the current instance of this application.
/// \param hPrevInst Handle to previous instance, deprecated.
/// \param lpCmdLine Command line string, unused. 
/// \param nShow Specifies how the window is to be shown.
/// \return TRUE if application terminates correctly.

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShow) {
	g_hInstance = hInst;
	if (!DefaultWinMain(hInst, hPrevInst, lpCmdLine, nShow)) return 1;

	g_cTimer.start(); //start game timer
	InitGame();

	return MessageLoop();
} //WinMain

/// \file defines.h
/// \brief Essential defines.

#pragma once

#include <d3d11_2.h>
#include <dxgi1_3.h>
#include <DirectXMath.h>

#include "SimpleMath.h"
#include "tinyxml2.h"

using namespace DirectX;
using namespace SimpleMath;
using namespace tinyxml2;
using namespace std;

/// Safe release of a pointer to a Windows COM object. If
/// the pointer is not null, then release what it is pointing
/// to and set it to null.

//Beware the extra parentheses in this #define. They are there
//for a reason. 

#define SAFE_RELEASE(p) if(p){(p)->Release(); (p) = nullptr;}

/// Safe delete of a pointer. Note that it is OK to delete a null
/// pointer. Note the brackets and braces.

#define SAFE_DELETE(p) {delete (p); (p) = nullptr;}

/// \brief Billboard vertex structure. 
///
/// Custom vertex format for representing a vanilla billboard object. Consists 
/// of position, and texture coordinates.

struct BILLBOARDVERTEX{ 
  Vector3 p; ///< Position.
  float tu; ///< Texture U coordinate.
  float tv; ///< Texture V coordinate.
}; //BILLBOARDVERTEX

/// Constant buffer for use by shaders.

struct ConstantBuffer{
  XMFLOAT4X4 wvp; ///< World View Projection matrix
  float u0, u1, v0, v1;
}; //ConstantBuffer

/// Game state types.
/// Types of game states that the game can be in.

enum GameStateType{
	TITLE_GAMESTATE, MENU_GAMESTATE, CHARSELECT_GAMESTATE, HOWTOPLAY_GAMESTATE, CREDITS_GAMESTATE, PLAYING_GAMESTATE, WON_GAMESTATE, GAMEOVER_GAMESTATE, ENDING_GAMESTATE
};

enum LevelStateType {
	NONE_STATE, COMICWORLD_STATE, FANTASY_STATE, CITY_STATE
};

enum EnemyStateType {
	MOVING_STATE, ATTACKING_STATE
};

/// Game object types.
/// Types of game object that can appear in the game. Note: NUM_OBJECT_TYPES 
/// must be last.

enum ObjectType{
  FREDIDLE_OBJECT, FREDATTACK_OBJECT, FREDHURT_OBJECT, FREDLOST_OBJECT, 
  SWAZIDLE_OBJECT, SWAZATTACK_OBJECT, SWAZHURT_OBJECT, SWAZLOST_OBJECT,
  POLKIDLE_OBJECT, POLKATTACK_OBJECT, POLKHURT_OBJECT, POLKLOST_OBJECT,
  ENEMY1IDLE_OBJECT, ENEMY1AFTER_OBJECT, ENEMY1HURT_OBJECT, ENEMYENTRY_OBJECT, ENEMYEXIT_OBJECT,
  ENEMYINVADERIDLE_OBJECT, ENEMYINVADERATTACK_OBJECT, ENEMYINVADERHURT_OBJECT,
  ENEMYZOOMERIDLE_OBJECT, ENEMYZOOMERBOUNCE_OBJECT, ENEMYZOOMERHURT_OBJECT, ENEMYZOOMERIDLEFLIP_OBJECT, ENEMYZOOMERBOUNCEFLIP_OBJECT,
	ENEMYTHIEFIDLE_OBJECT, ENEMYTHIEFATTACK_OBJECT, ENEMYTHIEFHURT_OBJECT,
  LIFEFULL_OBJECT, LIFEHALF_OBJECT, LIFEEMPTY_OBJECT,
  ITEMHEART_OBJECT, ITEMR_OBJECT, ITEME_OBJECT, ITEMD_OBJECT,
  ITEMW_OBJECT, ITEMA_OBJECT, ITEMZ_OBJECT,
  ITEMO_OBJECT, ITEML_OBJECT, ITEMK_OBJECT,
  HUD_OBJECT, HUD2A_OBJECT, HUD2B_OBJECT, HUD3_OBJECT, KEYF_OBJECT, KEYS_OBJECT, KEYP_OBJECT,
  F_OBJECT_NORM, R_OBJECT_NORM, E_OBJECT_NORM, D_OBJECT_NORM,
  F_OBJECT_SPEC, R_OBJECT_SPEC, E_OBJECT_SPEC, D_OBJECT_SPEC,
  F_OBJECT_BIG, R_OBJECT_BIG, E_OBJECT_BIG, D_OBJECT_BIG,
  R_OBJECT_EMPTY, E_OBJECT_EMPTY, D_OBJECT_EMPTY,
  S_OBJECT_NORM, W_OBJECT_NORM, A_OBJECT_NORM, Z_OBJECT_NORM,
  S_OBJECT_SPEC, W_OBJECT_SPEC, A_OBJECT_SPEC, Z_OBJECT_SPEC,
  S_OBJECT_BIG, W_OBJECT_BIG, A_OBJECT_BIG, Z_OBJECT_BIG,
  W_OBJECT_EMPTY, A_OBJECT_EMPTY, Z_OBJECT_EMPTY,
  P_OBJECT_NORM, O_OBJECT_NORM, L_OBJECT_NORM, K_OBJECT_NORM,
  P_OBJECT_SPEC, O_OBJECT_SPEC, L_OBJECT_SPEC, K_OBJECT_SPEC,
  P_OBJECT_BIG, O_OBJECT_BIG, L_OBJECT_BIG, K_OBJECT_BIG,
  O_OBJECT_EMPTY, L_OBJECT_EMPTY, K_OBJECT_EMPTY,
  COUNTINF_OBJECT, COUNT0_OBJECT, COUNT1_OBJECT, COUNT2_OBJECT, COUNT3_OBJECT, COUNT4_OBJECT, COUNT5_OBJECT,
  FREDLIFE_OBJECT, SWAZLIFE_OBJECT, POLKLIFE_OBJECT,
  SHIELD_OBJECT, ASSISTFRED_OBJECT, ASSISTSWAZ_OBJECT, ASSISTPOLK_OBJECT,
  PROJECTILEF_OBJECT, PROJECTILES_OBJECT, PROJECTILEP_OBJECT,
  PROJECTILED_OBJECT, PROJECTILEZ_OBJECT, PROJECTILEK_OBJECT,
  PROJECTILEENEMY1_OBJECT, PROJECTILEINVADER_OBJECT, PROJECTILETHIEF_OBJECT, EXPLOSION_OBJECT,
  MAINCURS_OBJECT, CHARCURS_OBJECT, TNEXT_OBJECT, TBACK_OBJECT,
  MENUSTART_OBJECT, MENUOPTIONS_OBJECT, MENUHTP_OBJECT, MENUCREDITS_OBJECT, 
  TITLESCREEN_OBJECT, MENUSCREEN_OBJECT, CHARSCREEN_OBJECT, CREDSCREEN_OBJECT, HTPSCREEN_OBJECT,
  ENDINGFRED_OBJECT, ENDINGSWAZ_OBJECT, ENDINGPOLK_OBJECT,
  STAGE0TITLE_OBJECT, STAGE1TITLE_OBJECT, STAGE2TITLE_OBJECT,
  DARKSCREEN_OBJECT, ENDSCREEN_OBJECT, GAMEOVERSCREEN_OBJECT,
  FREDGOOD_OBJECT, FREDNEUTRAL_OBJECT, FREDBAD_OBJECT,
  SWAZGOOD_OBJECT, SWAZNEUTRAL_OBJECT, SWAZBAD_OBJECT,
  POLKGOOD_OBJECT, POLKNEUTRAL_OBJECT, POLKBAD_OBJECT,
  RANKS_OBJECT, RANKA_OBJECT, RANKB_OBJECT, RANKC_OBJECT, RANKD_OBJECT,
  RANKSSMALL_OBJECT, RANKASMALL_OBJECT, RANKBSMALL_OBJECT, RANKCSMALL_OBJECT, RANKDSMALL_OBJECT,
  NUM_OBJECT_TYPES //MUST be the last one
}; //ObjectType

/// Shader type for pixel shaders.

enum ShaderType {
	NULL_SHADER, GHOST_SHADER,
	NUM_SHADERS
}; //ShaderType
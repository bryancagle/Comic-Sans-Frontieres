/// \file gamerenderer.cpp
/// \brief Direct3D rendering tasks for the game.
/// DirectX stuff that won't change much is hidden away in this file
/// so you won't have to keep looking at it.

#include <algorithm>

#include "gamerenderer.h"
#include "defines.h" 
#include "abort.h"
#include "imagefilenamelist.h"
#include "debug.h"
#include "sprite.h"
#include "object.h"
#include "objman.h"
#include "Sound.h"
#include "spriteman.h"
#include "SpriteSheet.h"
#include "Timer.h"
#include "Random.h"

extern int g_nScreenWidth;
extern int g_nScreenHeight;
extern BOOL g_bWireFrame;
extern CImageFileNameList g_cImageFileName;
extern CObjectManager g_cObjectManager;
extern CSpriteManager g_cSpriteManager;
extern CSoundManager* g_pSoundManager;
extern CTimer g_cTimer;
extern ShaderType g_nPixelShader;
extern float g_fScreenScroll;
extern BOOL g_bDarkenScreen;
extern BOOL g_bPlayerTyped[4];
extern BOOL g_bSpecialAttackReleased;
extern BOOL g_bSpecialActivate;
extern GameStateType g_nGameState;
extern LevelStateType g_nLevelState;
extern CGameObject* player;
extern CRandom g_cRandom;

extern int cursorPos;
extern int cursorPosMax;

extern int m_nEnemyCount;

CGameObject* g_cObject;

CGameRenderer::CGameRenderer(): m_bCameraDefaultMode(TRUE){
  m_cScreenText = nullptr;    
  m_nFrameCount = m_nLastFrameCountTime = m_nLastSpawnTime = 0;
	darken = bigF = bigR = bigE = bigD = nullptr;
	m_bFirstScreenPos = TRUE;
} //constructor

CGameRenderer::~CGameRenderer(){
  delete m_cScreenText;
} //constructor

/// Initialize the vertex and constant buffers for the background, that is, the
/// ground and the sky.

void CGameRenderer::InitBackground(){
  HRESULT hr;

  //load vertex buffer
  float w = 1584.0; //2.0f*g_nScreenWidth;
	  float h = 612.0; //2.0f*g_nScreenHeight;
  
  //vertex information, first triangle in clockwise order
  BILLBOARDVERTEX pVertexBufferData[6]; 
  pVertexBufferData[0].p = Vector3(w, 0, 0);
  pVertexBufferData[0].tu = 1.0f; pVertexBufferData[0].tv = 0.0f;

  pVertexBufferData[1].p = Vector3(0, 0, 0);
  pVertexBufferData[1].tu = 0.0f; pVertexBufferData[1].tv = 0.0f;

  pVertexBufferData[2].p = Vector3(w, 0, 750);
  pVertexBufferData[2].tu = 1.0f; pVertexBufferData[2].tv = 1.0f;

  pVertexBufferData[3].p = Vector3(0, 0, 750);
  pVertexBufferData[3].tu = 0.0f; pVertexBufferData[3].tv = 1.0f;

  pVertexBufferData[4].p = Vector3(w, h, 750);
  pVertexBufferData[4].tu = 1.0f; pVertexBufferData[4].tv = 0.0f;

  pVertexBufferData[5].p = Vector3(0, h, 750);
  pVertexBufferData[5].tu = 0.0f; pVertexBufferData[5].tv = 0.0f;
  
  //create vertex buffer for background
  m_pShader = new CShader(2, NUM_SHADERS);
    
  m_pShader->AddInputElementDesc(0, DXGI_FORMAT_R32G32B32_FLOAT, "POSITION");
  m_pShader->AddInputElementDesc(12, DXGI_FORMAT_R32G32_FLOAT,  "TEXCOORD");
  m_pShader->VSCreateAndCompile(L"VertexShader.hlsl", "main");
	m_pShader->PSCreateAndCompile(L"DoNothing.hlsl", "main", NULL_SHADER);
	m_pShader->PSCreateAndCompile(L"Ghost.hlsl", "main", GHOST_SHADER);
    
  // Create constant buffers.
  D3D11_BUFFER_DESC constantBufferDesc = { 0 };
  constantBufferDesc.ByteWidth = sizeof(ConstantBuffer); 
  constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
  constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  constantBufferDesc.CPUAccessFlags = 0;
  constantBufferDesc.MiscFlags = 0;
  constantBufferDesc.StructureByteStride = 0;
    
  m_pDev2->CreateBuffer(&constantBufferDesc, nullptr, &m_pConstantBuffer);
    
  D3D11_BUFFER_DESC VertexBufferDesc;
  VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
  VertexBufferDesc.ByteWidth = sizeof(BILLBOARDVERTEX)* 6;
  VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  VertexBufferDesc.CPUAccessFlags = 0;
  VertexBufferDesc.MiscFlags = 0;
  VertexBufferDesc.StructureByteStride = 0;
    
  D3D11_SUBRESOURCE_DATA subresourceData;
  subresourceData.pSysMem = pVertexBufferData;
  subresourceData.SysMemPitch = 0;
  subresourceData.SysMemSlicePitch = 0;
    
  hr = m_pDev2->CreateBuffer(&VertexBufferDesc, &subresourceData, &m_pBackgroundVB);
} //InitBackground

/// Draw the game background.
/// \param x Camera x offset

void CGameRenderer::DrawBackground(float x){
  const float delta = 2.0f * g_nScreenWidth;
  float fQuantizeX = delta * (int)(x / delta - 1.0f) + g_nScreenWidth; //Quantized x coordinate

  UINT nVertexBufferOffset = 0;
  
  UINT nVertexBufferStride = sizeof(BILLBOARDVERTEX);
  m_pDC2->IASetVertexBuffers(0, 1, &m_pBackgroundVB, &nVertexBufferStride, &nVertexBufferOffset);
  m_pDC2->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  m_pShader->SetShaders(g_nPixelShader);

  //draw floor
  if(g_bWireFrame)
    m_pDC2->PSSetShaderResources(0, 1, &m_pWireframeTexture); //set wireframe texture
  else
    m_pDC2->PSSetShaderResources(0, 1, &m_pFloorTexture); //set floor texture
  
  SetWorldMatrix(Vector3(fQuantizeX, 0, 0));
  
  ConstantBuffer constantBufferData; ///< Constant buffer data for shader.

  constantBufferData.wvp = CalculateWorldViewProjectionMatrix();
  constantBufferData.u0 = 0.0f;
  constantBufferData.u1 = 1.0f;
  constantBufferData.v0 = 0.0f;
  constantBufferData.v1 = 1.0f;
  m_pDC2->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &constantBufferData, 0, 0);
  m_pDC2->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
  //m_pDC2->Draw(4, 2);

  SetWorldMatrix(Vector3(fQuantizeX - delta, 0, 0));

  constantBufferData.wvp = CalculateWorldViewProjectionMatrix();
  m_pDC2->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &constantBufferData, 0, 0);
  m_pDC2->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
  //m_pDC2->Draw(4, 2);

  //draw backdrop
	if (!g_bWireFrame) {
		if (g_nLevelState == COMICWORLD_STATE)
			m_pDC2->PSSetShaderResources(0, 1, &m_pWallTexture);
		else if(g_nLevelState == FANTASY_STATE)
			m_pDC2->PSSetShaderResources(0, 1, &m_pFantasyTexture);
		else
			m_pDC2->PSSetShaderResources(0, 1, &m_pCityTexture);
	}

  SetWorldMatrix(Vector3(fQuantizeX - 375, 0, -350));

  constantBufferData.wvp = CalculateWorldViewProjectionMatrix();
  m_pDC2->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &constantBufferData, 0, 0);
  m_pDC2->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
  m_pDC2->Draw(4, 2);

  /*SetWorldMatrix(Vector3(fQuantizeX - delta, 0, -800));

  constantBufferData.wvp = CalculateWorldViewProjectionMatrix();
  m_pDC2->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &constantBufferData, 0, 0);
  m_pDC2->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
  m_pDC2->Draw(4, 2);
	*/
} //DrawBackground
 
/// Load the background and sprite textures.

void CGameRenderer::LoadTextures(){ 
  //background
  LoadTexture(m_pWallTexture, g_cImageFileName[0]);
  LoadTexture(m_pFantasyTexture, g_cImageFileName[1]);
  LoadTexture(m_pCityTexture, g_cImageFileName[2]);

  //sprites
  g_cSpriteManager.Load(ENEMYENTRY_OBJECT, "enemyEntry");
  g_cSpriteManager.Load(ENEMYEXIT_OBJECT, "enemyExit");
  g_cSpriteManager.Load(ENEMY1IDLE_OBJECT, "enemy1Idle");
  g_cSpriteManager.Load(ENEMY1AFTER_OBJECT, "enemy1IdleAfter");
  g_cSpriteManager.Load(ENEMY1HURT_OBJECT, "enemy1Hurt");

  g_cSpriteManager.Load(ENEMYINVADERIDLE_OBJECT, "enemyInvaderIdle");
  g_cSpriteManager.Load(ENEMYINVADERATTACK_OBJECT, "enemyInvaderAttack");
  g_cSpriteManager.Load(ENEMYINVADERHURT_OBJECT, "enemyInvaderHurt");

  g_cSpriteManager.Load(ENEMYZOOMERIDLE_OBJECT, "enemyZoomerIdle");
  g_cSpriteManager.Load(ENEMYZOOMERBOUNCE_OBJECT, "enemyZoomerBounce");
  g_cSpriteManager.Load(ENEMYZOOMERHURT_OBJECT, "enemyZoomerHurt");
  g_cSpriteManager.Load(ENEMYZOOMERIDLEFLIP_OBJECT, "enemyZoomerIdleFlip");
  g_cSpriteManager.Load(ENEMYZOOMERBOUNCEFLIP_OBJECT, "enemyZoomerBounceFlip");

	g_cSpriteManager.Load(ENEMYTHIEFIDLE_OBJECT, "enemyThiefIdle");
	g_cSpriteManager.Load(ENEMYTHIEFATTACK_OBJECT, "enemyThiefAttack");
	g_cSpriteManager.Load(ENEMYTHIEFHURT_OBJECT, "enemyThiefHurt");

  g_cSpriteManager.Load(PROJECTILEF_OBJECT, "projectileF");
  g_cSpriteManager.Load(PROJECTILES_OBJECT, "projectileS");
  g_cSpriteManager.Load(PROJECTILEP_OBJECT, "projectileP");
  g_cSpriteManager.Load(PROJECTILED_OBJECT, "projectileD");
  g_cSpriteManager.Load(PROJECTILEZ_OBJECT, "projectileZ");
  g_cSpriteManager.Load(PROJECTILEK_OBJECT, "projectileK");
  g_cSpriteManager.Load(PROJECTILEENEMY1_OBJECT, "projectileEnemy1");
  g_cSpriteManager.Load(PROJECTILEINVADER_OBJECT, "projectileInvader");
	g_cSpriteManager.Load(PROJECTILETHIEF_OBJECT, "projectileThief");

  g_cSpriteManager.Load(SHIELD_OBJECT, "shield");
  g_cSpriteManager.Load(ASSISTFRED_OBJECT, "assistFred");
  g_cSpriteManager.Load(ASSISTSWAZ_OBJECT, "assistSwaz");
  g_cSpriteManager.Load(ASSISTPOLK_OBJECT, "assistPolk");
  g_cSpriteManager.Load(EXPLOSION_OBJECT, "explosion");


  g_cSpriteManager.Load(FREDIDLE_OBJECT, "fredIdle");
  g_cSpriteManager.Load(FREDATTACK_OBJECT, "fredAttack");
  g_cSpriteManager.Load(FREDHURT_OBJECT, "fredHurt");
  g_cSpriteManager.Load(FREDLOST_OBJECT, "fredLost");

  g_cSpriteManager.Load(SWAZIDLE_OBJECT, "swazIdle");
  g_cSpriteManager.Load(SWAZATTACK_OBJECT, "swazAttack");
  g_cSpriteManager.Load(SWAZHURT_OBJECT, "swazHurt");
  g_cSpriteManager.Load(SWAZLOST_OBJECT, "swazLost");

  g_cSpriteManager.Load(POLKIDLE_OBJECT, "polkIdle");
  g_cSpriteManager.Load(POLKATTACK_OBJECT, "polkAttack");
  g_cSpriteManager.Load(POLKHURT_OBJECT, "polkHurt");
  g_cSpriteManager.Load(POLKLOST_OBJECT, "polkLost");

  g_cSpriteManager.Load(LIFEFULL_OBJECT, "lifeFull");
  g_cSpriteManager.Load(LIFEHALF_OBJECT, "lifeHalf");
  g_cSpriteManager.Load(LIFEEMPTY_OBJECT, "lifeEmpty");
  g_cSpriteManager.Load(FREDLIFE_OBJECT, "fredLife");
  g_cSpriteManager.Load(SWAZLIFE_OBJECT, "swazLife");
  g_cSpriteManager.Load(POLKLIFE_OBJECT, "polkLife");
  g_cSpriteManager.Load(HUD_OBJECT, "UIBase");
  g_cSpriteManager.Load(HUD2A_OBJECT, "UIBase2a");
  g_cSpriteManager.Load(HUD2B_OBJECT, "UIBase2b");
  g_cSpriteManager.Load(HUD3_OBJECT, "UIBase3");

  g_cSpriteManager.Load(KEYF_OBJECT, "keyF");
  g_cSpriteManager.Load(KEYS_OBJECT, "keyS");
  g_cSpriteManager.Load(KEYP_OBJECT, "keyP");

  g_cSpriteManager.Load(ITEMR_OBJECT, "itemLetterR");
  g_cSpriteManager.Load(ITEME_OBJECT, "itemLetterE");
  g_cSpriteManager.Load(ITEMD_OBJECT, "itemLetterD");

  g_cSpriteManager.Load(ITEMW_OBJECT, "itemLetterW");
  g_cSpriteManager.Load(ITEMA_OBJECT, "itemLetterA");
  g_cSpriteManager.Load(ITEMZ_OBJECT, "itemLetterZ");

  g_cSpriteManager.Load(ITEMO_OBJECT, "itemLetterO");
  g_cSpriteManager.Load(ITEML_OBJECT, "itemLetterL");
  g_cSpriteManager.Load(ITEMK_OBJECT, "itemLetterK");
  g_cSpriteManager.Load(ITEMHEART_OBJECT, "itemHealth");

  g_cSpriteManager.Load(F_OBJECT_NORM, "letterNormalF");
  g_cSpriteManager.Load(R_OBJECT_NORM, "letterNormalR");
  g_cSpriteManager.Load(E_OBJECT_NORM, "letterNormalE");
  g_cSpriteManager.Load(D_OBJECT_NORM, "letterNormalD");
  g_cSpriteManager.Load(F_OBJECT_SPEC, "letterSpecialF");
  g_cSpriteManager.Load(R_OBJECT_SPEC, "letterSpecialR");
  g_cSpriteManager.Load(E_OBJECT_SPEC, "letterSpecialE");
  g_cSpriteManager.Load(D_OBJECT_SPEC, "letterSpecialD");
  g_cSpriteManager.Load(R_OBJECT_EMPTY, "letterEmptyR");
  g_cSpriteManager.Load(E_OBJECT_EMPTY, "letterEmptyE");
  g_cSpriteManager.Load(D_OBJECT_EMPTY, "letterEmptyD");
  g_cSpriteManager.Load(F_OBJECT_BIG, "letterBigF");
  g_cSpriteManager.Load(R_OBJECT_BIG, "letterBigR");
  g_cSpriteManager.Load(E_OBJECT_BIG, "letterBigE");
  g_cSpriteManager.Load(D_OBJECT_BIG, "letterBigD");

  g_cSpriteManager.Load(S_OBJECT_NORM, "letterNormalS");
  g_cSpriteManager.Load(W_OBJECT_NORM, "letterNormalW");
  g_cSpriteManager.Load(A_OBJECT_NORM, "letterNormalA");
  g_cSpriteManager.Load(Z_OBJECT_NORM, "letterNormalZ");
  g_cSpriteManager.Load(S_OBJECT_SPEC, "letterSpecialS");
  g_cSpriteManager.Load(W_OBJECT_SPEC, "letterSpecialW");
  g_cSpriteManager.Load(A_OBJECT_SPEC, "letterSpecialA");
  g_cSpriteManager.Load(Z_OBJECT_SPEC, "letterSpecialZ");
  g_cSpriteManager.Load(W_OBJECT_EMPTY, "letterEmptyW");
  g_cSpriteManager.Load(A_OBJECT_EMPTY, "letterEmptyA");
  g_cSpriteManager.Load(Z_OBJECT_EMPTY, "letterEmptyZ");
  g_cSpriteManager.Load(S_OBJECT_BIG, "letterBigS");
  g_cSpriteManager.Load(W_OBJECT_BIG, "letterBigW");
  g_cSpriteManager.Load(A_OBJECT_BIG, "letterBigA");
  g_cSpriteManager.Load(Z_OBJECT_BIG, "letterBigZ");

  g_cSpriteManager.Load(P_OBJECT_NORM, "letterNormalP");
  g_cSpriteManager.Load(O_OBJECT_NORM, "letterNormalO");
  g_cSpriteManager.Load(L_OBJECT_NORM, "letterNormalL");
  g_cSpriteManager.Load(K_OBJECT_NORM, "letterNormalK");
  g_cSpriteManager.Load(P_OBJECT_SPEC, "letterSpecialP");
  g_cSpriteManager.Load(O_OBJECT_SPEC, "letterSpecialO");
  g_cSpriteManager.Load(L_OBJECT_SPEC, "letterSpecialL");
  g_cSpriteManager.Load(K_OBJECT_SPEC, "letterSpecialK");
  g_cSpriteManager.Load(O_OBJECT_EMPTY, "letterEmptyO");
  g_cSpriteManager.Load(L_OBJECT_EMPTY, "letterEmptyL");
  g_cSpriteManager.Load(K_OBJECT_EMPTY, "letterEmptyK");
  g_cSpriteManager.Load(P_OBJECT_BIG, "letterBigP");
  g_cSpriteManager.Load(O_OBJECT_BIG, "letterBigO");
  g_cSpriteManager.Load(L_OBJECT_BIG, "letterBigL");
  g_cSpriteManager.Load(K_OBJECT_BIG, "letterBigK");

  g_cSpriteManager.Load(COUNTINF_OBJECT, "ammoCountInf");
  g_cSpriteManager.Load(COUNT0_OBJECT, "ammoCount0");
  g_cSpriteManager.Load(COUNT1_OBJECT, "ammoCount1");
  g_cSpriteManager.Load(COUNT2_OBJECT, "ammoCount2");
  g_cSpriteManager.Load(COUNT3_OBJECT, "ammoCount3");
  g_cSpriteManager.Load(COUNT4_OBJECT, "ammoCount4");
  g_cSpriteManager.Load(COUNT5_OBJECT, "ammoCount5");

  g_cSpriteManager.Load(DARKSCREEN_OBJECT, "screendark");
  g_cSpriteManager.Load(TITLESCREEN_OBJECT, "titlescreen");
  g_cSpriteManager.Load(MENUSCREEN_OBJECT, "menuscreen");
  g_cSpriteManager.Load(GAMEOVERSCREEN_OBJECT, "gameover");
  g_cSpriteManager.Load(CHARSCREEN_OBJECT, "charscreen");
  g_cSpriteManager.Load(CREDSCREEN_OBJECT, "credits");
  g_cSpriteManager.Load(HTPSCREEN_OBJECT, "howtoplay");

  g_cSpriteManager.Load(ENDINGFRED_OBJECT, "endingFred");
  g_cSpriteManager.Load(ENDINGSWAZ_OBJECT, "endingSwaz");
  g_cSpriteManager.Load(ENDINGPOLK_OBJECT, "endingPolk");

  g_cSpriteManager.Load(STAGE0TITLE_OBJECT, "stage0Title");
  g_cSpriteManager.Load(STAGE1TITLE_OBJECT, "stage1Title");
  g_cSpriteManager.Load(STAGE2TITLE_OBJECT, "stage2Title");

  g_cSpriteManager.Load(MENUSTART_OBJECT, "menuStart");
  g_cSpriteManager.Load(MENUOPTIONS_OBJECT, "menuOptions");
  g_cSpriteManager.Load(MENUHTP_OBJECT, "menuHTP");
  g_cSpriteManager.Load(MENUCREDITS_OBJECT, "menuCredits");

  g_cSpriteManager.Load(MAINCURS_OBJECT, "maincursor");
  g_cSpriteManager.Load(CHARCURS_OBJECT, "charcursor");

  g_cSpriteManager.Load(ENDSCREEN_OBJECT, "screenend");
  g_cSpriteManager.Load(FREDGOOD_OBJECT, "fredGood");
  g_cSpriteManager.Load(FREDNEUTRAL_OBJECT, "fredNeutral");
  g_cSpriteManager.Load(FREDBAD_OBJECT, "fredBad");

  g_cSpriteManager.Load(SWAZGOOD_OBJECT, "swazGood");
  g_cSpriteManager.Load(SWAZNEUTRAL_OBJECT, "swazNeutral");
  g_cSpriteManager.Load(SWAZBAD_OBJECT, "swazBad");

  g_cSpriteManager.Load(POLKGOOD_OBJECT, "polkGood");
  g_cSpriteManager.Load(POLKNEUTRAL_OBJECT, "polkNeutral");
  g_cSpriteManager.Load(POLKBAD_OBJECT, "polkBad");

  g_cSpriteManager.Load(RANKS_OBJECT, "rankS");
  g_cSpriteManager.Load(RANKA_OBJECT, "rankA");
  g_cSpriteManager.Load(RANKB_OBJECT, "rankB");
  g_cSpriteManager.Load(RANKC_OBJECT, "rankC");
  g_cSpriteManager.Load(RANKD_OBJECT, "rankD");

  g_cSpriteManager.Load(RANKSSMALL_OBJECT, "rankSsmall");
  g_cSpriteManager.Load(RANKASMALL_OBJECT, "rankAsmall");
  g_cSpriteManager.Load(RANKBSMALL_OBJECT, "rankBsmall");
  g_cSpriteManager.Load(RANKCSMALL_OBJECT, "rankCsmall");
  g_cSpriteManager.Load(RANKDSMALL_OBJECT, "rankDsmall");

  m_cScreenText = new CSpriteSheet(21, 37);
  m_cScreenText->Load("Images\\Text.png");
} //LoadTextures

/// All textures used in the game are released - the release function is kind
/// of like a destructor for DirectX entities, which are COM objects.

void CGameRenderer::Release(){ 
  g_cSpriteManager.Release();

  SAFE_RELEASE(m_pWallTexture);
  SAFE_RELEASE(m_pFloorTexture);
  SAFE_RELEASE(m_pWireframeTexture);
  SAFE_RELEASE(m_pBackgroundVB);
  SAFE_RELEASE(m_cScreenText);

  SAFE_DELETE(m_pShader);

  CRenderer::Release();
} //Release

/// Draw the heads-up display.
/// \param text0 Text to draw at left of HUD.
/// \param text1 Text to draw at right of HUD.

void CGameRenderer::DrawHUD(){
  CGameObject* player = g_cObjectManager.GetPlayerObjectPtr();
  float w = g_nScreenWidth/2.0f;
  float h = g_nScreenHeight/2.0f;
  Vector3 p;

  //switch to orthographic projection
  XMMATRIX tempProj = m_matProj;
  m_matProj = XMMatrixOrthographicOffCenterLH(-w, w, -h, h, 1.0f, 10000.0f);
  m_matWorld = XMMatrixIdentity();
  m_matView = XMMatrixLookAtLH(Vector3(w, h, 0), Vector3(w, h, 1000.0f), Vector3(0, 1, 0));
  
  ///clear the depth buffer
  m_pDC2->ClearDepthStencilView(m_pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

  //draws game over screen if player loses
	if(g_nGameState == GAMEOVER_GAMESTATE){
		if (g_bDarkenScreen) {
			g_bDarkenScreen = FALSE;
			darken = nullptr;
		}
 		gameOver = g_cSpriteManager.GetSprite(GAMEOVERSCREEN_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + title->m_nWidth / 2.0f, g_nScreenHeight - title->m_nHeight / 2.0f, 101.0f);
		gameOver->Draw(p, 0.0f, 0);
	}

	//This and the else ifs draws the title card of the stage
	if(g_nLevelState == COMICWORLD_STATE){
		stageTitle = g_cSpriteManager.GetSprite(STAGE0TITLE_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + stageTitle->m_nWidth / 2.0f + 818, stageTitle->m_nHeight, 101.0f);
		stageTitle->Draw(p, 0.0f, 0);
	}
	else if(g_nLevelState == FANTASY_STATE){
		stageTitle = g_cSpriteManager.GetSprite(STAGE1TITLE_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + stageTitle->m_nWidth / 2.0f + 817, stageTitle->m_nHeight, 101.0f);
		stageTitle->Draw(p, 0.0f, 0);
	}
	else if(g_nLevelState == CITY_STATE){
		stageTitle = g_cSpriteManager.GetSprite(STAGE2TITLE_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + stageTitle->m_nWidth / 2.0f + 817, stageTitle->m_nHeight, 101.0f);
		stageTitle->Draw(p, 0.0f, 0);
	}

	//Sprites for each of the letters
	int *ammoCount = g_cObjectManager.getAmmoCount();
	if(g_bDarkenScreen && ((bigF != nullptr || ammoCount[0] > 0) && (bigR != nullptr || ammoCount[1] > 0) && (bigE != nullptr || ammoCount[2] > 0))){
		//darken the screen for special attack
		if(darken == nullptr) darken = g_cSpriteManager.GetSprite(DARKSCREEN_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + darken->m_nWidth / 2.0f, g_nScreenHeight - darken->m_nHeight / 2.0f, 100.0f);
		darken->Draw(p, 0.0f, 0);

		if(player->m_nObjectType == FREDIDLE_OBJECT){
			if (g_bPlayerTyped[0] && bigF == nullptr) bigF = g_cSpriteManager.GetSprite(F_OBJECT_BIG);
			if (g_bPlayerTyped[1] && bigR == nullptr) bigR = g_cSpriteManager.GetSprite(R_OBJECT_BIG);
			if (g_bPlayerTyped[2] && bigE == nullptr) bigE = g_cSpriteManager.GetSprite(E_OBJECT_BIG);
			if (g_bPlayerTyped[3] && bigD == nullptr) bigD = g_cSpriteManager.GetSprite(D_OBJECT_BIG);
		}
		else if(player->m_nObjectType == SWAZIDLE_OBJECT){
			if (g_bPlayerTyped[0] && bigF == nullptr) bigF = g_cSpriteManager.GetSprite(S_OBJECT_BIG);
			if (g_bPlayerTyped[1] && bigR == nullptr) bigR = g_cSpriteManager.GetSprite(W_OBJECT_BIG);
			if (g_bPlayerTyped[2] && bigE == nullptr) bigE = g_cSpriteManager.GetSprite(A_OBJECT_BIG);
			if (g_bPlayerTyped[3] && bigD == nullptr) bigD = g_cSpriteManager.GetSprite(Z_OBJECT_BIG);
		}
		else if(player->m_nObjectType == POLKIDLE_OBJECT){
			if (g_bPlayerTyped[0] && bigF == nullptr) bigF = g_cSpriteManager.GetSprite(P_OBJECT_BIG);
			if (g_bPlayerTyped[1] && bigR == nullptr) bigR = g_cSpriteManager.GetSprite(O_OBJECT_BIG);
			if (g_bPlayerTyped[2] && bigE == nullptr) bigE = g_cSpriteManager.GetSprite(L_OBJECT_BIG);
			if (g_bPlayerTyped[3] && bigD == nullptr) bigD = g_cSpriteManager.GetSprite(K_OBJECT_BIG);
		}

		if((bigF == nullptr && bigR != nullptr) || (bigF == nullptr && bigE != nullptr) || (bigF == nullptr && bigD != nullptr) ||
			(bigR == nullptr && bigE != nullptr) || (bigR == nullptr && bigD != nullptr) || (bigE == nullptr && bigD != nullptr)){
			g_bDarkenScreen = FALSE;
		}
		if(bigF != nullptr){
			p = Vector3(-g_nScreenWidth / 4.0f + bigF->m_nWidth-200, g_nScreenHeight / 2.0f - bigF->m_nHeight + 300, -10.0f);
			bigF->Draw(p, 0.0f, 0);
		}
		if(bigR != nullptr){
			p = Vector3(-g_nScreenWidth / 4.0f + bigR->m_nWidth + 0, g_nScreenHeight / 2.0f - bigR->m_nHeight + 300, -10.0f);
			bigR->Draw(p, 0.0f, 0);
		}
		if(bigE != nullptr){
			p = Vector3(-g_nScreenWidth / 4.0f + bigE->m_nWidth + 200, g_nScreenHeight / 2.0f - bigE->m_nHeight + 300, -10.0f);
			bigE->Draw(p, 0.0f, 0);
		}
		if(bigD != nullptr){
			p = Vector3(-g_nScreenWidth / 4.0f + bigD->m_nWidth + 400, g_nScreenHeight / 2.0f - bigD->m_nHeight + 300, -10.0f);
			bigD->Draw(p, 0.0f, 0);
		}
		if(bigF != nullptr && bigR != nullptr && bigE != nullptr && bigD != nullptr){
			g_bSpecialActivate = TRUE;
			
		}
		if(g_bSpecialAttackReleased){
			bigF = bigR = bigE = bigD = nullptr;
			g_bPlayerTyped[0] = g_bPlayerTyped[1] = g_bPlayerTyped[2] = g_bPlayerTyped[3] = FALSE;
			g_bDarkenScreen = FALSE;
			g_bSpecialActivate = FALSE;
		}
	}
	else{
		bigF = bigR = bigE = bigD = nullptr;
		g_bDarkenScreen = FALSE;
		g_bSpecialActivate = FALSE;
	}

  //draw the HUD background sprite
  C3DSprite* hud = g_cSpriteManager.GetSprite(HUD_OBJECT);
  p = Vector3(-g_nScreenWidth / 2.0f + hud->m_nWidth / 2.0f + 5, g_nScreenHeight - hud->m_nHeight / 2.0f - 5, 1001.0f);
  hud->Draw(p, 0.0f, 0);

  C3DSprite* hud2;
  if(g_cObjectManager.getPlayerLives()>9) hud2 = g_cSpriteManager.GetSprite(HUD2B_OBJECT);
  else hud2 = g_cSpriteManager.GetSprite(HUD2A_OBJECT);
  p = Vector3(-g_nScreenWidth / 2.0f + hud2->m_nWidth / 2.0f + 5, g_nScreenHeight - hud2->m_nHeight / 2.0f - 685, 1001.0f);
  hud2->Draw(p, 0.0f, 0);

  C3DSprite* hud3 = g_cSpriteManager.GetSprite(HUD3_OBJECT);
  p = Vector3(-g_nScreenWidth / 2.0f + hud3->m_nWidth / 2.0f + 695, g_nScreenHeight - hud3->m_nHeight / 2.0f - 5, 1001.0f);
  hud3->Draw(p, 0.0f, 0);
  
  //Find the amount of hearts to draw, and draw them
  int numFullHearts = 0;
  int numHalfHearts = 0;
  int numEmptyHearts = 0;
  int heartPosition = 0;
	if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT || player->m_nObjectType == FREDLOST_OBJECT){

		 switch(g_cObjectManager.getPlayerHealth()){
		 case 0:
			 numFullHearts = 0;
			 numHalfHearts = 0;
			 numEmptyHearts = 3;
			 break;
		 case 1:
			 numFullHearts = 0;
			 numHalfHearts = 1;
			 numEmptyHearts = 2;
			 break;
		 case 2:
			 numFullHearts = 1;
			 numHalfHearts = 0;
			 numEmptyHearts = 2;
			 break;
		 case 3:
			 numFullHearts = 1;
			 numHalfHearts = 1;
			 numEmptyHearts = 1;
			 break;
		 case 4:
			 numFullHearts = 2;
			 numHalfHearts = 0;
			 numEmptyHearts = 1;
			 break;
		 case 5:
			 numFullHearts = 2;
			 numHalfHearts = 1;
			 numEmptyHearts = 0;
			 break;
		 case 6:
			 numFullHearts = 3;
			 numHalfHearts = 0;
			 numEmptyHearts = 0;
			 break;
		 }
	}
	if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT || player->m_nObjectType == SWAZLOST_OBJECT){

		switch(g_cObjectManager.getPlayerHealth()){
		case 0:
			numFullHearts = 0;
			numHalfHearts = 0;
			numEmptyHearts = 4;
			break;
		case 1:
			numFullHearts = 0;
			numHalfHearts = 1;
			numEmptyHearts = 3;
			break;
		case 2:
			numFullHearts = 1;
			numHalfHearts = 0;
			numEmptyHearts = 3;
			break;
		case 3:
			numFullHearts = 1;
			numHalfHearts = 1;
			numEmptyHearts = 2;
			break;
		case 4:
			numFullHearts = 2;
			numHalfHearts = 0;
			numEmptyHearts = 2;
			break;
		case 5:
			numFullHearts = 2;
			numHalfHearts = 1;
			numEmptyHearts = 1;
			break;
		case 6:
			numFullHearts = 3;
			numHalfHearts = 0;
			numEmptyHearts = 1;
			break;
		case 7:
			numFullHearts = 3;
			numHalfHearts = 1;
			numEmptyHearts = 0;
			break;
		case 8:
			numFullHearts = 4;
			numHalfHearts = 0;
			numEmptyHearts = 0;
			break;
		}
	}
	if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT || player->m_nObjectType == POLKLOST_OBJECT){

		switch(g_cObjectManager.getPlayerHealth()){
		case 0:
			numFullHearts = 0;
			numHalfHearts = 0;
			numEmptyHearts = 2;
			break;
		case 1:
			numFullHearts = 0;
			numHalfHearts = 1;
			numEmptyHearts = 1;
			break;
		case 2:
			numFullHearts = 1;
			numHalfHearts = 0;
			numEmptyHearts = 1;
			break;
		case 3:
			numFullHearts = 1;
			numHalfHearts = 1;
			numEmptyHearts = 0;
			break;
		case 4:
			numFullHearts = 2;
			numHalfHearts = 0;
			numEmptyHearts = 0;
			break;
		}
	}
	for(int j = 0; j < numFullHearts; j++){
		C3DSprite* heartF = g_cSpriteManager.GetSprite(LIFEFULL_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + heartF->m_nWidth / 2.0f + 22 + (40 * heartPosition), g_nScreenHeight - heartF->m_nHeight / 2.0f - 15, 1000.0f);
		heartF->Draw(p, 0.0f, 0);
		heartPosition += 1;
	}
	for(int j = 0; j < numHalfHearts; j++){
		C3DSprite* heartH = g_cSpriteManager.GetSprite(LIFEHALF_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + heartH->m_nWidth / 2.0f + 22 + (40 * heartPosition), g_nScreenHeight - heartH->m_nHeight / 2.0f - 15, 1000.0f);
		heartH->Draw(p, 0.0f, 0);
		heartPosition += 1;
	}
	for(int j = 0; j < numEmptyHearts; j++){
		C3DSprite* heartE = g_cSpriteManager.GetSprite(LIFEEMPTY_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + heartE->m_nWidth / 2.0f + 22 + (40 * heartPosition), g_nScreenHeight - heartE->m_nHeight / 2.0f - 15, 1000.0f);
		heartE->Draw(p, 0.0f, 0);
		heartPosition += 1;
	}

  //Draw the first letter in the UI
	if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT || player->m_nObjectType == FREDLOST_OBJECT){
	  if((ammoCount[0] > 0) && (ammoCount[1] > 0)&&(ammoCount[2] > 0))
			letter1 = g_cSpriteManager.GetSprite(F_OBJECT_SPEC);
	  else
			letter1 = g_cSpriteManager.GetSprite(F_OBJECT_NORM);
  }
  else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT || player->m_nObjectType == SWAZLOST_OBJECT){
	  if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
		  letter1 = g_cSpriteManager.GetSprite(S_OBJECT_SPEC);
	  else
		  letter1 = g_cSpriteManager.GetSprite(S_OBJECT_NORM);
  }
  else if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT || player->m_nObjectType == POLKLOST_OBJECT){
	  if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
		  letter1 = g_cSpriteManager.GetSprite(P_OBJECT_SPEC);
	  else
		  letter1 = g_cSpriteManager.GetSprite(P_OBJECT_NORM);
  }
  p = Vector3(-g_nScreenWidth / 2.0f + letter1->m_nWidth / 2.0f + 22, g_nScreenHeight - letter1->m_nHeight / 2.0f - 55, 1000.0f);
  letter1->Draw(p, 0.0f, 0);
 

  //Draw the second letter in the UI
	if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT || player->m_nObjectType == FREDLOST_OBJECT){
	  if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
			letter2 = g_cSpriteManager.GetSprite(R_OBJECT_SPEC);
	  else if(ammoCount[0] > 0)
			letter2 = g_cSpriteManager.GetSprite(R_OBJECT_NORM);
	  else
			letter2 = g_cSpriteManager.GetSprite(R_OBJECT_EMPTY);
  } 
  else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT || player->m_nObjectType == SWAZLOST_OBJECT){
	  if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
		  letter2 = g_cSpriteManager.GetSprite(W_OBJECT_SPEC);
	  else if(ammoCount[0] > 0)
		  letter2 = g_cSpriteManager.GetSprite(W_OBJECT_NORM);
	  else
		  letter2 = g_cSpriteManager.GetSprite(W_OBJECT_EMPTY);
  }
  else if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT || player->m_nObjectType == POLKLOST_OBJECT){
	  if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
		  letter2 = g_cSpriteManager.GetSprite(O_OBJECT_SPEC);
	  else if(ammoCount[0] > 0)
		  letter2 = g_cSpriteManager.GetSprite(O_OBJECT_NORM);
	  else
		  letter2 = g_cSpriteManager.GetSprite(O_OBJECT_EMPTY);
  }
  p = Vector3(-g_nScreenWidth / 2.0f + letter2->m_nWidth / 2.0f + 72, g_nScreenHeight - letter2->m_nHeight / 2.0f - 55, 1000.0f);
  letter2->Draw(p, 0.0f, 0);
  
  //Draw the third letter in the UI
	if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT || player->m_nObjectType == FREDLOST_OBJECT){
	  if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
		  letter3 = g_cSpriteManager.GetSprite(E_OBJECT_SPEC);
	  else if(ammoCount[1] > 0)
		  letter3 = g_cSpriteManager.GetSprite(E_OBJECT_NORM);
	  else
		  letter3 = g_cSpriteManager.GetSprite(E_OBJECT_EMPTY);
  }
  else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT || player->m_nObjectType == SWAZLOST_OBJECT){
	  if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
		  letter3 = g_cSpriteManager.GetSprite(A_OBJECT_SPEC);
	  else if(ammoCount[1] > 0)
		  letter3 = g_cSpriteManager.GetSprite(A_OBJECT_NORM);
	  else
		  letter3 = g_cSpriteManager.GetSprite(A_OBJECT_EMPTY);
  }
  else if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT || player->m_nObjectType == POLKLOST_OBJECT){
	  if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
		  letter3 = g_cSpriteManager.GetSprite(L_OBJECT_SPEC);
	  else if(ammoCount[1] > 0)
		  letter3 = g_cSpriteManager.GetSprite(L_OBJECT_NORM);
	  else
		  letter3 = g_cSpriteManager.GetSprite(L_OBJECT_EMPTY);
  }
  p = Vector3(-g_nScreenWidth / 2.0f + letter3->m_nWidth / 2.0f + 122, g_nScreenHeight - letter3->m_nHeight / 2.0f - 55, 1000.0f);
  letter3->Draw(p, 0.0f, 0);

  //Draw the fourth letter in the UI
	if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT || player->m_nObjectType == FREDLOST_OBJECT){
		if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
			letter4 = g_cSpriteManager.GetSprite(D_OBJECT_SPEC);
		else if(ammoCount[2] > 0)
			letter4 = g_cSpriteManager.GetSprite(D_OBJECT_NORM);
		else
			letter4 = g_cSpriteManager.GetSprite(D_OBJECT_EMPTY);
  }
	else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT || player->m_nObjectType == SWAZLOST_OBJECT){
		if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
			letter4 = g_cSpriteManager.GetSprite(Z_OBJECT_SPEC);
		else if(ammoCount[2] > 0)
			letter4 = g_cSpriteManager.GetSprite(Z_OBJECT_NORM);
		else
			letter4 = g_cSpriteManager.GetSprite(Z_OBJECT_EMPTY);
	}
	else if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT || player->m_nObjectType == POLKLOST_OBJECT){
		if((ammoCount[0] > 0) && (ammoCount[1] > 0) && (ammoCount[2] > 0))
			letter4 = g_cSpriteManager.GetSprite(K_OBJECT_SPEC);
		else if(ammoCount[2] > 0)
			letter4 = g_cSpriteManager.GetSprite(K_OBJECT_NORM);
		else
			letter4 = g_cSpriteManager.GetSprite(K_OBJECT_EMPTY);
	}
	p = Vector3(-g_nScreenWidth / 2.0f + letter4->m_nWidth / 2.0f + 172, g_nScreenHeight - letter4->m_nHeight / 2.0f - 55, 1000.0f);
	letter4->Draw(p, 0.0f, 0);
  
  //Ammo count for each letter, first letter is always infinite
  C3DSprite* ammoInf = g_cSpriteManager.GetSprite(COUNTINF_OBJECT);
  p = Vector3(-g_nScreenWidth / 2.0f + ammoInf->m_nWidth / 2.0f + 45, g_nScreenHeight - ammoInf->m_nHeight / 2.0f - 80, 1000.0f);
  ammoInf->Draw(p, 0.0f, 0);

  //Draws the ammo count for each letter
  C3DSprite* ammoLetter;
  for(int i = 0; i < 3; i++){
	  switch(ammoCount[i]){
	  case 0:
		  ammoLetter = g_cSpriteManager.GetSprite(COUNT0_OBJECT);
		  p = Vector3(-g_nScreenWidth / 2.0f + ammoLetter->m_nWidth / 2.0f + 95 + (50 * i), g_nScreenHeight - ammoLetter->m_nHeight / 2.0f - 80, 1000.0f);
		  ammoLetter->Draw(p, 0.0f, 0);
		  break;
	  case 1:
		  ammoLetter = g_cSpriteManager.GetSprite(COUNT1_OBJECT);
		  p = Vector3(-g_nScreenWidth / 2.0f + ammoLetter->m_nWidth / 2.0f + 95 + (50 * i), g_nScreenHeight - ammoLetter->m_nHeight / 2.0f - 80, 1000.0f);
		  ammoLetter->Draw(p, 0.0f, 0);
		  break;
	  case 2:
		  ammoLetter = g_cSpriteManager.GetSprite(COUNT2_OBJECT);
		  p = Vector3(-g_nScreenWidth / 2.0f + ammoLetter->m_nWidth / 2.0f + 95 + (50 * i), g_nScreenHeight - ammoLetter->m_nHeight / 2.0f - 80, 1000.0f);
		  ammoLetter->Draw(p, 0.0f, 0);
		  break;
	  case 3:
		  ammoLetter = g_cSpriteManager.GetSprite(COUNT3_OBJECT);
		  p = Vector3(-g_nScreenWidth / 2.0f + ammoLetter->m_nWidth / 2.0f + 95 + (50 * i), g_nScreenHeight - ammoLetter->m_nHeight / 2.0f - 80, 1000.0f);
		  ammoLetter->Draw(p, 0.0f, 0);
		  break;
	  case 4:
		  ammoLetter = g_cSpriteManager.GetSprite(COUNT4_OBJECT);
		  p = Vector3(-g_nScreenWidth / 2.0f + ammoLetter->m_nWidth / 2.0f + 95 + (50 * i), g_nScreenHeight - ammoLetter->m_nHeight / 2.0f - 80, 1000.0f);
		  ammoLetter->Draw(p, 0.0f, 0);
		  break;
	  case 5:
		  ammoLetter = g_cSpriteManager.GetSprite(COUNT5_OBJECT);
		  p = Vector3(-g_nScreenWidth / 2.0f + ammoLetter->m_nWidth / 2.0f + 95 + (50 * i), g_nScreenHeight - ammoLetter->m_nHeight / 2.0f - 80, 1000.0f);
		  ammoLetter->Draw(p, 0.0f, 0);
		  break;
	  }
  }

	//Draw the key icon, telling the player which key to press to shoot
  if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT || player->m_nObjectType == FREDLOST_OBJECT){
	  key = g_cSpriteManager.GetSprite(KEYF_OBJECT); // Key F icon for Fred
	  p = Vector3(-g_nScreenWidth / 2.0f + key->m_nWidth / 2.0f + 15, g_nScreenHeight - key->m_nHeight / 2.0f - 115, 1000.0f);
	  key->Draw(p, 0.0f, 0);
  }
  else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT || player->m_nObjectType == SWAZLOST_OBJECT){
	  key = g_cSpriteManager.GetSprite(KEYS_OBJECT); // Key S icon for Swaz
	  p = Vector3(-g_nScreenWidth / 2.0f + key->m_nWidth / 2.0f + 15, g_nScreenHeight - key->m_nHeight / 2.0f - 115, 1000.0f);
	  key->Draw(p, 0.0f, 0);
  }
  else if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT || player->m_nObjectType == POLKLOST_OBJECT){
	  key = g_cSpriteManager.GetSprite(KEYP_OBJECT); // Key P icon for Polk
	  p = Vector3(-g_nScreenWidth / 2.0f + key->m_nWidth / 2.0f + 15, g_nScreenHeight - key->m_nHeight / 2.0f - 115, 1000.0f);
	  key->Draw(p, 0.0f, 0);
  }

	DrawHUDText("Shoot", Vector3(80.0f, 630.0f, 1000.0f));

  //Draws the life icon of the player character
  C3DSprite* lifeIcon;

	if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT || player->m_nObjectType == FREDLOST_OBJECT)
	  lifeIcon = g_cSpriteManager.GetSprite(FREDLIFE_OBJECT);
  else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT || player->m_nObjectType == SWAZLOST_OBJECT)
	  lifeIcon = g_cSpriteManager.GetSprite(SWAZLIFE_OBJECT);
  else
	  lifeIcon = g_cSpriteManager.GetSprite(POLKLIFE_OBJECT);

  p = Vector3(-g_nScreenWidth / 2.0f + lifeIcon->m_nWidth / 2.0f + 15, g_nScreenHeight - lifeIcon->m_nHeight / 2.0f - 700, 1000.0f);
  lifeIcon->Draw(p, 0.0f, 0);

  //Creates a character string for drawing the life count
  char lifeCount[3] = "x";
  sprintf_s(lifeCount, "%d", g_cObjectManager.getPlayerLives());
  
  //Draw the life count
  DrawHUDText("x", p + Vector3(545.0f, -10.0f, -10.0f));
  DrawHUDText(lifeCount, p + Vector3(565.0f, -10.0f, -10.0f));

  //Creates a character string for drawing the score count
  char scoreCount[7];
  sprintf_s(scoreCount, "%d", g_cObjectManager.getScore());
  
  //Draw the score count
  DrawHUDText("Score:", p + Vector3(1217.0f, 683.0f, -10.0f));
  DrawHUDText(scoreCount, p + Vector3(1350.0f, 683.0f, -10.0f));

  //back to perspective projection 
  m_matProj = tempProj;
} //DrawHUD

/// Used to draw menu screens

void CGameRenderer::DrawMenu(){
	if(g_nGameState == PLAYING_GAMESTATE || g_nGameState == GAMEOVER_GAMESTATE) return;
	float w = g_nScreenWidth / 2.0f;
	float h = g_nScreenHeight / 2.0f;
	Vector3 p;
	Vector3 d;

	//switch to orthographic projection
	XMMATRIX tempProj = m_matProj;
	m_matProj = XMMatrixOrthographicOffCenterLH(-w, w, -h, h, 1.0f, 10000.0f);
	m_matWorld = XMMatrixIdentity();
	m_matView = XMMatrixLookAtLH(Vector3(w, h, 0), Vector3(w, h, 1000.0f), Vector3(0, 1, 0));

	///clear the depth buffer
	m_pDC2->ClearDepthStencilView(m_pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	switch (g_nGameState){
		case TITLE_GAMESTATE: //draws title screen
			title = g_cSpriteManager.GetSprite(TITLESCREEN_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + title->m_nWidth / 2.0f, g_nScreenHeight - title->m_nHeight / 2.0f, 101.0f);
			title->Draw(p, 0.0f, 0);
			break;

		case MENU_GAMESTATE: //draws main menu, the cursor
			menu = g_cSpriteManager.GetSprite(MENUSCREEN_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + menu->m_nWidth / 2.0f, g_nScreenHeight - menu->m_nHeight / 2.0f, 101.0f);
			menu->Draw(p, 0.0f, 0);

			mainCursor = g_cSpriteManager.GetSprite(MAINCURS_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + mainCursor->m_nWidth / 2.0f + 110, g_nScreenHeight - mainCursor->m_nHeight / 2.0f - 165 - (130 * cursorPos), 100.0f);
			mainCursor->Draw(p, 0.0f, 0);

			if(cursorPos == 0){
				menuIcon = g_cSpriteManager.GetSprite(MENUSTART_OBJECT);
				p = Vector3(-g_nScreenWidth / 2.0f + menuIcon->m_nWidth / 2.0f + 600, g_nScreenHeight - menuIcon->m_nHeight / 2.0f - 250, 101.0f);
				menuIcon->Draw(p, 0.0f, 0);
			} else if(cursorPos == 1){
				menuIcon = g_cSpriteManager.GetSprite(MENUHTP_OBJECT);
				p = Vector3(-g_nScreenWidth / 2.0f + menuIcon->m_nWidth / 2.0f + 600, g_nScreenHeight - menuIcon->m_nHeight / 2.0f - 250, 101.0f);
				menuIcon->Draw(p, 0.0f, 0);
			} else if(cursorPos == 2){
				menuIcon = g_cSpriteManager.GetSprite(MENUCREDITS_OBJECT);
				p = Vector3(-g_nScreenWidth / 2.0f + menuIcon->m_nWidth / 2.0f + 600, g_nScreenHeight - menuIcon->m_nHeight / 2.0f - 250, 101.0f);
				menuIcon->Draw(p, 0.0f, 0);
			} else if(cursorPos == 3){
				menuIcon = g_cSpriteManager.GetSprite(MENUOPTIONS_OBJECT);
				p = Vector3(-g_nScreenWidth / 2.0f + menuIcon->m_nWidth / 2.0f + 600, g_nScreenHeight - menuIcon->m_nHeight / 2.0f - 250, 101.0f);
				menuIcon->Draw(p, 0.0f, 0);
			}
			break;

		case CHARSELECT_GAMESTATE: //draws character select
			charSel = g_cSpriteManager.GetSprite(CHARSCREEN_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSel->m_nWidth / 2.0f, g_nScreenHeight - charSel->m_nHeight / 2.0f - 1.1f, 101.0f);
			charSel->Draw(p, 0.0f, 0);

			charCursor = g_cSpriteManager.GetSprite(CHARCURS_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charCursor->m_nWidth / 2.0f + 38 + (332 * cursorPos), g_nScreenHeight - charCursor->m_nHeight / 2.0f - 620, 100.0f);
			charCursor->Draw(p, 0.0f, 0);

			character = g_cSpriteManager.GetSprite(FREDNEUTRAL_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + character->m_nWidth / 2.0f + 70, g_nScreenHeight - character->m_nHeight / 2.0f - 185, 100.0f);
			character->Draw(p, 0.0f, 0);

			character = g_cSpriteManager.GetSprite(SWAZNEUTRAL_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + character->m_nWidth / 2.0f + 402, g_nScreenHeight - character->m_nHeight / 2.0f - 185, 100.0f);
			character->Draw(p, 0.0f, 0);

			character = g_cSpriteManager.GetSprite(POLKNEUTRAL_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + character->m_nWidth / 2.0f + 734, g_nScreenHeight - character->m_nHeight / 2.0f - 185, 100.0f);
			character->Draw(p, 0.0f, 0);
			break;

		case CREDITS_GAMESTATE: //draws credits screen
			credScreen = g_cSpriteManager.GetSprite(CREDSCREEN_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + credScreen->m_nWidth / 2.0f, g_nScreenHeight - credScreen->m_nHeight / 2.0f, 101.0f);
			credScreen->Draw(p, 0.0f, 0);
			break;

		case HOWTOPLAY_GAMESTATE: //draws how to play screen
			credScreen = g_cSpriteManager.GetSprite(HTPSCREEN_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + credScreen->m_nWidth / 2.0f, g_nScreenHeight - credScreen->m_nHeight / 2.0f, 101.0f);
			credScreen->Draw(p, 0.0f, 0);
			break;
	} //switch

	//back to perspective projection 
	m_matProj = tempProj;
} //DrawMenu

/// Draws the hud text

void CGameRenderer::DrawHUDText(char* text, Vector3 p){
	for(unsigned int i = 0; i<strlen(text); i++){
		char c = text[i];
		if(c >= 'A' && c <= 'Z')
			p = m_cScreenText->Draw(p, 1, 48, c - 'A');
		else if(c >= 'a' && c <= 'z')
			p = m_cScreenText->Draw(p, 1, 95, c - 'a');
		else if(c >= '0' && c <= '9')
			p = m_cScreenText->Draw(p, 1, 1, c - '0');
		else if(c == ':')
			p = m_cScreenText->Draw(p, 1, 1, c - '0');
		else p = m_cScreenText->Draw(p, 1, 1, 10); //blank
	} //for
} //DrawHUDText

/// Draws the ending screen, which  changes depending on the character. also shows the previous stage ranks, and calculates the final rank based off those

void CGameRenderer::Ending(){
	float w = g_nScreenWidth / 2.0f;
	float h = g_nScreenHeight / 2.0f;
	Vector3 p;
	Vector3 d;
	CGameObject* player = g_cObjectManager.GetPlayerObjectPtr();

	//switch to orthographic projection
	XMMATRIX tempProj = m_matProj;
	m_matProj = XMMatrixOrthographicOffCenterLH(-w, w, -h, h, 1.0f, 10000.0f);
	m_matWorld = XMMatrixIdentity();
	m_matView = XMMatrixLookAtLH(Vector3(w, h, 0), Vector3(w, h, 1000.0f), Vector3(0, 1, 0));

	///clear the depth buffer
	m_pDC2->ClearDepthStencilView(m_pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	C3DSprite* end;
	C3DSprite* rank;
	if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT)
	end = g_cSpriteManager.GetSprite(ENDINGFRED_OBJECT);
	else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT)
		end = g_cSpriteManager.GetSprite(ENDINGSWAZ_OBJECT);
	else 		
		end = g_cSpriteManager.GetSprite(ENDINGPOLK_OBJECT);

	p = Vector3(-g_nScreenWidth / 2.0f + end->m_nWidth / 2.0f, g_nScreenHeight - end->m_nHeight / 2.0f, 1000.0f);
	end->Draw(p, 0.0f, 0);

	if(m_nFinalScore1>=10000)
		rank = g_cSpriteManager.GetSprite(RANKSSMALL_OBJECT);
	else if(m_nFinalScore1 >= 8000)
		rank = g_cSpriteManager.GetSprite(RANKASMALL_OBJECT);
	else if(m_nFinalScore1 >= 7000)
		rank = g_cSpriteManager.GetSprite(RANKBSMALL_OBJECT);
	else if(m_nFinalScore1 >= 6000)
		rank = g_cSpriteManager.GetSprite(RANKCSMALL_OBJECT);
	else
		rank = g_cSpriteManager.GetSprite(RANKDSMALL_OBJECT);
	
	
	p = Vector3(-g_nScreenWidth / 2.0f + rank->m_nWidth / 2.0f + 215, g_nScreenHeight - rank->m_nHeight / 2.0f - 203, 999.0f);
	rank->Draw(p, 0.0f, 0);

	if(m_nFinalScore2 >= 10000)
		rank = g_cSpriteManager.GetSprite(RANKSSMALL_OBJECT);
	else if(m_nFinalScore2 >= 8000)
		rank = g_cSpriteManager.GetSprite(RANKASMALL_OBJECT);
	else if(m_nFinalScore2 >= 7000)
		rank = g_cSpriteManager.GetSprite(RANKBSMALL_OBJECT);
	else if(m_nFinalScore2 >= 6000)
		rank = g_cSpriteManager.GetSprite(RANKCSMALL_OBJECT);
	else
		rank = g_cSpriteManager.GetSprite(RANKDSMALL_OBJECT);

	p = Vector3(-g_nScreenWidth / 2.0f + rank->m_nWidth / 2.0f + 215, g_nScreenHeight - rank->m_nHeight / 2.0f - 300, 999.0f);
	rank->Draw(p, 0.0f, 0);

	if(m_nFinalScore3 >= 10000)
		rank = g_cSpriteManager.GetSprite(RANKSSMALL_OBJECT);
	else if(m_nFinalScore3 >= 8000)
		rank = g_cSpriteManager.GetSprite(RANKASMALL_OBJECT);
	else if(m_nFinalScore3 >= 7000)
		rank = g_cSpriteManager.GetSprite(RANKBSMALL_OBJECT);
	else if(m_nFinalScore3 >= 6000)
		rank = g_cSpriteManager.GetSprite(RANKCSMALL_OBJECT);
	else
		rank = g_cSpriteManager.GetSprite(RANKDSMALL_OBJECT);

	p = Vector3(-g_nScreenWidth / 2.0f + rank->m_nWidth / 2.0f + 215, g_nScreenHeight - rank->m_nHeight / 2.0f - 397, 999.0f);
	rank->Draw(p, 0.0f, 0);

	int finalRankScore = (m_nFinalScore1 + m_nFinalScore2 + m_nFinalScore3) / 3;

	if(finalRankScore >= 10000)
		rank = g_cSpriteManager.GetSprite(RANKS_OBJECT);
	else if(finalRankScore >= 8000)
		rank = g_cSpriteManager.GetSprite(RANKA_OBJECT);
	else if(finalRankScore >= 7000)
		rank = g_cSpriteManager.GetSprite(RANKB_OBJECT);
	else if(finalRankScore >= 6000)
		rank = g_cSpriteManager.GetSprite(RANKC_OBJECT);
	else
		rank = g_cSpriteManager.GetSprite(RANKD_OBJECT);

	p = Vector3(-g_nScreenWidth / 2.0f + rank->m_nWidth / 2.0f + 270, g_nScreenHeight - rank->m_nHeight / 2.0f - 469, 999.0f);
	rank->Draw(p, 0.0f, 0);

	m_matProj = tempProj;
} //Ending

/// Draw the end screen at the end of a level with stats and ranking

void CGameRenderer::EndScreen(){
	float w = g_nScreenWidth / 2.0f;
	float h = g_nScreenHeight / 2.0f;
	Vector3 p;
	Vector3 d;
	CGameObject* player = g_cObjectManager.GetPlayerObjectPtr();

	//switch to orthographic projection
	XMMATRIX tempProj = m_matProj;
	m_matProj = XMMatrixOrthographicOffCenterLH(-w, w, -h, h, 1.0f, 10000.0f);
	m_matWorld = XMMatrixIdentity();
	m_matView = XMMatrixLookAtLH(Vector3(w, h, 0), Vector3(w, h, 1000.0f), Vector3(0, 1, 0));

	///clear the depth buffer
	m_pDC2->ClearDepthStencilView(m_pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	C3DSprite* end = g_cSpriteManager.GetSprite(ENDSCREEN_OBJECT);
	p = Vector3(-g_nScreenWidth / 2.0f + end->m_nWidth / 2.0f, g_nScreenHeight - end->m_nHeight / 2.0f, 1000.0f);
	end->Draw(p, 0.0f, 0);

	C3DSprite* charSpr = g_cSpriteManager.GetSprite(FREDNEUTRAL_OBJECT);
	p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
	
	d = p;
	int score = g_cObjectManager.getScore();
	BOOL gotHit = g_cObjectManager.gotHit();
	int fireShots = g_cObjectManager.firedShots();
	int hitShots = g_cObjectManager.hitShots();
	int accuracy = 0;
	if(fireShots == 0)
		accuracy = 0;
	else
		accuracy = (100 * hitShots) / fireShots;

	if(accuracy >= 100) accuracy = 100;

	//Creates a character string for drawing the score count
	char scoreCount[10];
	sprintf_s(scoreCount, "%d", g_cObjectManager.getScore());

	//Draw the score count
	DrawHUDText(scoreCount, p + Vector3(890.0f, 327.0f, -10.0f));

	//Draw the perfect bonus count
	if(g_cObjectManager.gotHit() == FALSE){
	DrawHUDText("5000", p + Vector3(1015.0f, 238.0f, -10.0f));
	score += 5000;
	}
	else DrawHUDText("0", p + Vector3(1015.0f, 238.0f, -10.0f));

	//Draw the accuracy count
	char accuracyPerc[10];
	sprintf_s(accuracyPerc, "%d", accuracy);
	DrawHUDText(accuracyPerc, p + Vector3(783.0f, 106.0f, -10.0f));
		
	//Draw the accuracy score count
	char accuracyScore[10];
	accuracy *= 50;
	sprintf_s(accuracyScore, "%d", accuracy);
	DrawHUDText(accuracyScore, p + Vector3(1050.0f, 106.0f, -10.0f));
	score += accuracy;

	//Draw the final score count
	sprintf_s(scoreCount, "%d", score);
	DrawHUDText(scoreCount, p + Vector3(970.0f, 18.0f, -10.0f));

	//Draw the ranking
	C3DSprite* rank;
		
	if(score >= 10000){
		rank = g_cSpriteManager.GetSprite(RANKS_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + rank->m_nWidth / 2.0f + 600, g_nScreenHeight - rank->m_nHeight / 2.0f - 510, 999.0f);
		rank->Draw(p, 0.0f, 0);

		if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(FREDGOOD_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
		else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(SWAZGOOD_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
		else if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(POLKGOOD_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
	} //if
	else if(score >= 8000){
		rank = g_cSpriteManager.GetSprite(RANKA_OBJECT);
		p= Vector3(-g_nScreenWidth / 2.0f + rank->m_nWidth / 2.0f + 600, g_nScreenHeight - rank->m_nHeight / 2.0f - 510, 999.0f);
		rank->Draw(p, 0.0f, 0);

		if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(FREDGOOD_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
		else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(SWAZGOOD_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
		else if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(POLKGOOD_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
	} //else if
	else if(score >= 7000){
		rank = g_cSpriteManager.GetSprite(RANKB_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + rank->m_nWidth / 2.0f + 600, g_nScreenHeight - rank->m_nHeight / 2.0f - 510, 999.0f);
		rank->Draw(p, 0.0f, 0);

		if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(FREDNEUTRAL_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
		else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(SWAZNEUTRAL_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
		else if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(POLKNEUTRAL_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
	} //else if
	else if(score >= 6000){
		rank = g_cSpriteManager.GetSprite(RANKC_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + rank->m_nWidth / 2.0f + 600, g_nScreenHeight - rank->m_nHeight / 2.0f - 510, 999.0f);
		rank->Draw(p, 0.0f, 0);

		if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(FREDNEUTRAL_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
		else if (player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(SWAZNEUTRAL_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
		else if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(POLKNEUTRAL_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
	} //else if
	else{
		rank = g_cSpriteManager.GetSprite(RANKD_OBJECT);
		p = Vector3(-g_nScreenWidth / 2.0f + rank->m_nWidth / 2.0f + 600, g_nScreenHeight - rank->m_nHeight / 2.0f - 510, 999.0f);
		rank->Draw(p, 0.0f, 0);

		if(player->m_nObjectType == FREDIDLE_OBJECT || player->m_nObjectType == FREDATTACK_OBJECT || player->m_nObjectType == FREDHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(FREDBAD_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
		else if(player->m_nObjectType == SWAZIDLE_OBJECT || player->m_nObjectType == SWAZATTACK_OBJECT || player->m_nObjectType == SWAZHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(SWAZBAD_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
		else if(player->m_nObjectType == POLKIDLE_OBJECT || player->m_nObjectType == POLKATTACK_OBJECT || player->m_nObjectType == POLKHURT_OBJECT){
			charSpr = g_cSpriteManager.GetSprite(POLKBAD_OBJECT);
			p = Vector3(-g_nScreenWidth / 2.0f + charSpr->m_nWidth / 2.0f + 140, g_nScreenHeight - charSpr->m_nHeight / 2.0f - 400, 999.0f);
			charSpr->Draw(p, 0.0f, 0);
		}
	} //else

	if(g_nLevelState == COMICWORLD_STATE)
		m_nFinalScore1 = score;
	else if(g_nLevelState == FANTASY_STATE)
		m_nFinalScore2 = score;
	else if(g_nLevelState == CITY_STATE)
		m_nFinalScore3 = score;
	
	//back to perspective projection 
  m_matProj = tempProj;
} //EndScreen

/// Move all objects, then draw them.
/// \return TRUE if it succeeded

void CGameRenderer::ComposeFrame(){
  //set camera location
  CGameObject* p = g_cObjectManager.GetPlayerObjectPtr();
	if (p == nullptr)
		ABORT("No player object found.");
	else if (m_bFirstScreenPos) {
		g_fScreenScroll = g_nScreenWidth / 2.0f;
		m_bFirstScreenPos = FALSE;
	}

  float x = p->m_vPos.x, y = p->m_vPos.y; //player's current location
  y = min(y, g_nScreenHeight/2.0f);
  y = max(y, g_nScreenHeight/4.0f);

  Vector3 pos, lookatpt;

  if(m_bCameraDefaultMode){
	  pos = Vector3(g_fScreenScroll + g_nScreenWidth / 2, 300, -350);
	  lookatpt = Vector3(g_fScreenScroll + g_nScreenWidth / 2, 300, 1000);
  }
  else{
	  pos = Vector3(x - 2.5f*g_nScreenWidth, 1000, -3000);
	  lookatpt = Vector3(x - g_nScreenWidth, 700, 0);
  } //else
  
  SetViewMatrix(pos, lookatpt);

  //prepare to draw
  m_pDC2->OMSetRenderTargets(1, &m_pRTV, m_pDSV);
  float clearColor[] = { 1.0f, 1.0f, 1.0f, 0.0f };
  m_pDC2->ClearRenderTargetView(m_pRTV, clearColor);
  m_pDC2->ClearDepthStencilView(m_pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

  //draw
  DrawBackground(g_fScreenScroll + g_nScreenWidth / 2.0f); //draw background
  g_cObjectManager.draw(); //draw objects

  //draw text
  m_nFrameCount++;
  if(g_cTimer.elapsed(m_nLastFrameCountTime, 1000)){
    m_nDisplayedFrameCount = m_nFrameCount;
    m_nFrameCount = 0;
  } //if

	//End of level, show results screen
  //once player crosses the end of the screen, then procede to the end screen state
  if(p->m_vPos.x > 1700){
	  g_nGameState = WON_GAMESTATE;
	  p->m_vPos.x = 1705;
	  p->m_bCanFire = FALSE;
	  if(g_nLevelState == COMICWORLD_STATE)
		  g_pSoundManager->stop(UNLEASH_SOUND);
	  else if(g_nLevelState == FANTASY_STATE)
		  g_pSoundManager->stop(MOOSEHEADHONK_SOUND);
	  else if(g_nLevelState == CITY_STATE)
		  g_pSoundManager->stop(HUBBUB_SOUND);
	  g_pSoundManager->play(STEPPINOUT_SOUND);
	  EndScreen();
  }
  else 
  DrawHUD();

  //scrolls the screen, and then spawn an enemy based off the timer
	if(g_nGameState == PLAYING_GAMESTATE)
  if(g_fScreenScroll < 1.3*g_nScreenWidth && g_nGameState != GAMEOVER_GAMESTATE){
	  if(g_cTimer.elapsed(m_nLastScrollTime, 17)) g_fScreenScroll += 0.4f;
	  if(g_cTimer.elapsed(m_nLastSpawnTime, 10000)){ //Spawn a number of enemies after an amount of time
		  float px = g_cObject->GetScreenFrameRight() - 200.0f;
		  float py = g_nScreenHeight / 2 - 100;
		  float radius = g_cRandom.number(50, 150);
		  float rotation = g_cRandom.number(0, 359);

		  if(g_cRandom.number(0, 1) == 0)
			  EnemyFormationSquare(px, py, radius, 4, rotation);
		  else
			  EnemyFormationLine(px, py, 70, 4, rotation);
	  } //if
  } //if
} //ComposeFrame

/// Draws enemy in line formation.
/// \param px X position.
/// \param py Y position.
/// \param r Spacing between enemies.
/// \param n Number of enemies.
/// rotate = rotation applied on to the line.

void CGameRenderer::EnemyFormationLine(float px, float py, float r, int n, float rotate){
	Vector3 s;
	for(int i = 0; i < n; i++){
		s.x = px + r * (i*(cos(rotate * (float)3.14 / 180)));
		s.y = py + r * (i*(sin(rotate * (float)3.14 / 180)));
		s.z = 375 - 5.0f*(4 / 2 + 0.5f);
		if(m_nEnemyCount < 10){
			g_cObjectManager.createObject(ENEMYENTRY_OBJECT, "enemyEntry", Vector3(s.x, s.y - 5, s.z - 1), Vector3(-0.0f, 0, 0));
			m_nEnemyCount++;
		}
	}
} //EnemyFormationLine

/// Draws enemy in square formation.
/// \param px X position.
/// \param py Y position.
/// \param r Spacing between enemies.
/// \param n Number of enemies.
/// \param rotate Rotation applied.

void CGameRenderer::EnemyFormationSquare(float px, float py, float r, int n, float rotate){
	Vector3 s;
	for(int i = 0; i < n; i++){
		s.x = px + r * sin(2.0f * i * (float)3.14 / n + (rotate * (float)3.14 / 180));
		s.y = py + r * cos(2.0f * i * (float)3.14 / n + (rotate * (float)3.14 / 180));
		s.z = 375 - 5.0f*(4 / 2 + 0.5f);
		if(m_nEnemyCount < 10000){
			g_cObjectManager.createObject(ENEMYENTRY_OBJECT, "enemyEntry", Vector3(s.x, s.y - 5, s.z - 1), Vector3(-0.0f, 0, 0));
			m_nEnemyCount++;
		}
	}
} //EnemyFormationSquare

/// Compose a frame of animation and present it to the video card.

void CGameRenderer::ProcessFrame(){
	if(g_nGameState == PLAYING_GAMESTATE){
		g_cTimer.beginframe();
		g_cObjectManager.move(); //move objects
		ComposeFrame(); //compose a frame of animation
	}
	else if(g_nGameState == ENDING_GAMESTATE){
		Ending();
	}
	else DrawMenu();
	m_pSwapChain2->Present(0, 0); //present it

  if(g_bSpecialAttackReleased){
	  g_bSpecialAttackReleased = FALSE;
  }
} //ProcessFrame

/// Toggle between eagle-eye camera (camera pulled back far enough to see
/// backdrop) and the normal game camera.
void CGameRenderer::FlipCameraMode(){
	m_bCameraDefaultMode = !m_bCameraDefaultMode;
} //FlipCameraMode

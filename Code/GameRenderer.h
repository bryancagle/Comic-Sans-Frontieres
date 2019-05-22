/// \file gamerenderer.h 
/// \brief Definition of the renderer class CGameRenderer.

#pragma once

#include "renderer.h"
#include "defines.h"
#include "Shader.h"

/// \brief The game renderer.
///
/// The game renderer class handles all the nasty Direct3D details associated
/// with game related rendering tasks, including drawing the game background.

class CGameRenderer: public CRenderer{
	friend class CObjectManager; 
	friend class CObject;
  private: 
    //Direct3D stuff for background floor and wall
    ID3D11Buffer* m_pBackgroundVB;  ///< Vertex buffer.
    ID3D11ShaderResourceView* m_pWallTexture; ///< Texture for wall.
		ID3D11ShaderResourceView* m_pFantasyTexture;
		ID3D11ShaderResourceView* m_pCityTexture;
    ID3D11ShaderResourceView* m_pFloorTexture; ///< Texture for floor.
    ID3D11ShaderResourceView* m_pWireframeTexture; ///< Texture for showing wireframe, all black.
    ID3D11Buffer* m_pConstantBuffer; ///< Constant buffer for shader.
    CShader* m_pShader; ///< Pointer to an instance of the shader class.

    BOOL m_bCameraDefaultMode; ///< Camera in default mode.
    
    CSpriteSheet* m_cScreenText; ///< Screen text sprite sheet.
		C3DSprite* title; //title screen
		C3DSprite* menu; //main menu
		C3DSprite* menuIcon; //the menu icon
		C3DSprite* nextback; //next/back icons - unused
		C3DSprite* charSel; //character select screen
		C3DSprite* credScreen; //credits screen
		C3DSprite* darken; //special attack screen
		C3DSprite* mainCursor; //main menu cursor
		C3DSprite* charCursor; //character select cursor
		C3DSprite* character; //character on the CSS
		C3DSprite* gameOver; //game over screen
		C3DSprite* stageTitle; //title card icon for stages
		
		//big letters used for special attack
		C3DSprite* bigF;
		C3DSprite* bigR;
		C3DSprite* bigE;
		C3DSprite* bigD;

		//small letters used for HUD
		C3DSprite* letter1;
		C3DSprite* letter2;
		C3DSprite* letter3;
		C3DSprite* letter4;

		C3DSprite* key; //current character's letter key

    int m_nFrameCount; ///< Frames rendered in current second.
    int m_nDisplayedFrameCount; ///< Frames rendered in the previous second.
    int m_nLastFrameCountTime; ///< Last time we changed displayed frame count.
		
		//used to store final score for final rank calculations
		int m_nFinalScore1;
		int m_nFinalScore2;
		int m_nFinalScore3;

		int m_nLastSpawnTime = 0;
		int m_nLastScrollTime = 0;
		BOOL m_bFirstScreenPos;

		void DrawHUD(); ///< Draw the heads-up display.
		void DrawMenu();
		void EndScreen();
		void Ending();
		void DrawHUDText(char * text, Vector3 p);
 
  public:
    CGameRenderer(); ///< Constructor.
    ~CGameRenderer(); ///< Destructor.

    void InitBackground(); ///< Initialize the background.
    void DrawBackground(float x); ///< Draw the background.
  
    void LoadTextures(); ///< Load textures for image storage.
    void Release(); ///< Release offscreen images.

    void ComposeFrame(); ///< Compose a frame of animation.
	
		void EnemyFormationLine(float px, float py, float r, int n, float rotate);
		void EnemyFormationSquare(float px, float py, float r, int n, float rotate);
    void ProcessFrame(); ///< Process a frame of animation.
		void FlipCameraMode(); ///< Flip the camera mode.
}; //CGameRenderer 

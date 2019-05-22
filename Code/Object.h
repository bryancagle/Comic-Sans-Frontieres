/// \file object.h
/// \brief Interface for the game object class CGameObject.

#pragma once

#include "sprite.h"
#include "defines.h"
#include "ObjMan.h"

/// \brief The game object. 
///
/// Game objects are responsible for remembering information about 
/// themselves - including their image, location, and speed - and
/// for moving and drawing themselves.

class CGameObject{ //class for a game object
  friend class CIntelligentObject;
  friend class CObjectManager;
  friend class CGameRenderer;
  friend class CSoundManager;
  friend BOOL KeyboardHandler(WPARAM keystroke); //for keyboard control of objects
	friend BOOL KeyboardHandlerReleased(WPARAM keystroke);
	friend bool ZCompare(const CGameObject* p0, const CGameObject* p1); //for depth sorting

  protected:
    ObjectType m_nObjectType; ///< Object type.

	BOOL m_bCanFire = TRUE;
    Vector3 m_vPos; ///< Current location.
    Vector3 m_vVelocity; ///< Current velocity.
    int m_nLastMoveTime; ///< Last time moved.
		float m_nAttackOrientation;
		int m_nDelayTime;
    float m_fOrientation; ///< Orientation, angle to rotate about the Z axis.
		int m_nHealth;

    C3DSprite* m_pSprite; ///< Pointer to sprite.
    int m_nCurrentFrame; ///< Frame to be displayed.
    int m_nFrameCount; ///< Number of frames in animation.
    int m_nLastFrameTime; ///< Last time the frame was changed.
    int m_nFrameInterval; ///< Interval between frames.
		int m_nLastFlyTime;
		int* m_pAnimation; ///< Sequence of frame numbers to be repeated
    int m_nAnimationFrameCount; ///< Number of entries in m_pAnimation
    
    int m_nWidth; ///< Width of object.
    int m_nHeight; ///< Height of object.
    int m_nBirthTime; ///< Time of creation.
    int m_nLifeTime; ///< Time that object lives.
		int m_nInvulnerableTime; ///< How long the object is invulnerable.
    BOOL m_bVulnerable; ///< Vulnerable to bullets.
    BOOL m_bIntelligent; ///< TRUE for an intelligent object.
    BOOL m_bCycleSprite; ///< TRUE to cycle sprite frames, otherwise play once.
    BOOL m_bIsDead; ///< TRUE if the object is dead.
    int m_nSoundInstance; ///< Sound instance played most recently.

    void LoadSettings(const char* name); //< Load object-dependent settings from XML element.

  public:
		CGameObject(ObjectType object, const char * name, const Vector3 & s, const Vector3 & v);
		CGameObject(ObjectType object, const char * name, const Vector3 & s, const Vector3 & v, int health); ///< Constructor.
    ~CGameObject(); //< Destructor.
    void draw(); ///< Draw at current location.
    virtual void move(); ///< Change location depending on time and speed
    void kill(); ///< Kill object.
		float GetScreenFrameLeft();
		float GetScreenFrameRight();
}; //CGameObject


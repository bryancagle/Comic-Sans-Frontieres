/// \file object.cpp
/// \brief Code for the game object class CGameObject.

#include "object.h"
#include "ObjMan.h"
#include "defines.h" 
#include "timer.h"
#include "debug.h" 
#include "spriteman.h" 
#include "sound.h"
#include "Random.h"
#include "GameRenderer.h"

extern CTimer g_cTimer;
extern int g_nScreenWidth;
extern int g_nScreenHeight;
extern int m_nEnemyCount;
extern float g_fScreenScroll;
extern XMLElement* g_xmlSettings;
extern CSpriteManager g_cSpriteManager;
extern CSoundManager* g_pSoundManager;
extern CObjectManager g_cObjectManager;
extern CRandom g_cRandom; ///< The random number generator.
extern BOOL g_bEndOfFirstLevel;
extern BOOL g_bPlayerIsInvulnerable;
extern BOOL g_bShieldActive;
extern BOOL g_bAssistActive;
CGameRenderer* g_cGameRenderer;

/// Initialize a game object. Gets object-dependent settings from g_xmlSettings
/// from the "object" tag that has the same "name" attribute as parameter name.
/// Assumes that the sprite manager has loaded the sprites already.
/// \param object Object type
/// \param name Object name in XML settings file object tag
/// \param s Initial location of object
/// \param v Initial velocity

CGameObject::CGameObject(ObjectType object, const char* name, const Vector3& s, const Vector3& v){ 
  //defaults
  m_nCurrentFrame = 0; 
  m_nLastFrameTime = g_cTimer.time();
  m_nFrameInterval = 30; 
  
	m_nDelayTime = 0;
  m_nLifeTime = -1; //negative means immortal
	m_nInvulnerableTime = 0;
  m_bVulnerable = FALSE; 
  m_bIntelligent = FALSE;
  m_bIsDead = FALSE;

	m_nAttackOrientation = 0.0f;
	m_nHealth = 3;

  m_pAnimation = nullptr;
  m_nAnimationFrameCount = 0;
  m_bCycleSprite = TRUE;

  //common values
  m_nObjectType = object; //type of object

  m_pSprite = g_cSpriteManager.GetSprite(object); //sprite pointer
  if(m_pSprite){
    m_nFrameCount = m_pSprite->m_nFrameCount; //get frame count
		m_nCurrentFrame = 0;
    m_nHeight = m_pSprite->m_nHeight; //get object height from sprite
    m_nWidth = m_pSprite->m_nWidth; //get object width from sprite
  } //if

  m_nLastMoveTime = g_cTimer.time(); //time
  m_nBirthTime = g_cTimer.time(); //time of creation
  m_vPos = s; //location
  m_vVelocity = v;  //velocity
  
  //object-dependent settings loaded from XML
  LoadSettings(name);
  m_nFrameInterval += g_cRandom.number(-m_nFrameInterval/10, m_nFrameInterval/10);

  //sound played at creation of object
  m_nSoundInstance = -1;

  switch(object){
    case PROJECTILEF_OBJECT:
      m_nSoundInstance = g_pSoundManager->play(SHOOT_SOUND);
      break;

		case PROJECTILES_OBJECT:
			m_nSoundInstance = g_pSoundManager->play(SHOOT_SOUND);
			break;

		case PROJECTILEP_OBJECT:
			m_nSoundInstance = g_pSoundManager->play(SHOOT_SOUND);
			break;
		case PROJECTILED_OBJECT:
			m_nSoundInstance = g_pSoundManager->play(SHOOT_SOUND);
			break;

		case PROJECTILEZ_OBJECT:
			m_nSoundInstance = g_pSoundManager->play(SHOOT_SOUND);
			break;

		case PROJECTILEK_OBJECT:
			m_nSoundInstance = g_pSoundManager->play(SHOOT_SOUND);
			break;
  } //switch
} //constructor


CGameObject::~CGameObject(){  
  delete[] m_pAnimation;
} //destructor

/// Draw the current sprite frame at the current position, then
/// compute which frame is to be drawn next time.

void CGameObject::draw(){
  if(m_pSprite == nullptr)return;
  if(m_bIsDead)return; //bail if already dead

  int t = m_nFrameInterval;
  if(m_bCycleSprite)
    t = (int)(t/(1.5f + fabs(m_vVelocity.x)));

   BOOL ghost = ((m_nObjectType == FREDHURT_OBJECT || m_nObjectType == SWAZHURT_OBJECT || m_nObjectType == POLKHURT_OBJECT));

  if(m_pAnimation != nullptr){ //if there's an animation sequence
    //draw current frame
    m_pSprite->Draw(m_vPos, m_fOrientation, m_pAnimation[m_nCurrentFrame], FALSE);
    //advance to next frame
    if(g_cTimer.elapsed(m_nLastFrameTime, t)) //if enough time passed
      //increment and loop if necessary
      if(++m_nCurrentFrame >= m_nAnimationFrameCount && m_bCycleSprite) 
        m_nCurrentFrame = 0;
  } //if
  else 
       m_pSprite->Draw(m_vPos, m_fOrientation, 0, ghost); //assume only one frame
} //draw

/// Load settings for object from g_xmlSettings.
/// \param name name of object as found in name tag of XML settings file

void CGameObject::LoadSettings(const char* name){
  if(g_xmlSettings){ //got "settings" tag
    //get "objects" tag
    XMLElement* objSettings =
      g_xmlSettings->FirstChildElement("objects"); //objects tag

    if(objSettings){ //got "objects" tag
      //set obj to the first "object" tag with the correct name
      XMLElement* obj = objSettings->FirstChildElement("object");
      while(obj && strcmp(name, obj->Attribute("name"))){
        obj = obj->NextSiblingElement("object");
      } //while

      if(obj){ //got "object" tag with right name
        //get object information from tag
				m_nFrameInterval = obj->IntAttribute("frameinterval");
        m_bVulnerable = obj->BoolAttribute("vulnerable");
        m_bCycleSprite = obj->BoolAttribute("cycle");
        m_nLifeTime = obj->IntAttribute("lifetime");
				m_nInvulnerableTime = obj->IntAttribute("invulnerabletime");

        //parse animation sequence
        if(obj->Attribute("animation")){ //sequence present

          //get sequence length
          size_t length = strlen(obj->Attribute("animation"));
          m_nAnimationFrameCount = 1; //one more than number of commas
          for(size_t i = 0; i<length; i++) //for each character
          if(obj->Attribute("animation")[i] == ',')
            m_nAnimationFrameCount++; //count commas

          m_pAnimation = new int[m_nAnimationFrameCount]; //memory for animation sequence

          size_t i = 0; //character index
          int count = 0; //number of frame numbers input
          int num; //frame number
          char c = obj->Attribute("animation")[i]; //character in sequence string
          while(i < length){
            //get next frame number
            num = 0;
            while(i<length && c >= '0' && c <= '9'){
              num = num * 10 + c - '0';
              c = obj->Attribute("animation")[++i];
            }
            //process frame number
            c = obj->Attribute("animation")[++i]; //skip over comma
            m_pAnimation[count++] = num; //record frame number
          } //while
        } //if
      } //if
    } //if
  } //if
} //LoadSettings
 
/// The distance that an object moves depends on its speed, 
/// and the amount of time since it last moved.

void CGameObject::move() { //move object 
	const float SCALE = 32.0f; //to scale back motion
	const float TOPMARGIN = -200.0f; //margin on top of page
	const float SIDEMARGIN = 140.0f;
	const float BOTTOMMARGIN = 40.0f;
	const float GRAVITY = 9.8f; //gravity

	const int time = g_cTimer.time(); //current time
	const int tdelta = time - m_nLastMoveTime; //time since last move
	const float tfactor = tdelta / SCALE; //scaled time factor

	m_vPos.x += m_vVelocity.x*tfactor; //motion
	m_vPos.y += m_vVelocity.y*tfactor; //motion

	//Handles wall collisions for each of these. Usually just removes them. Decreases enemy screen count if its an enemy
	if(m_nObjectType == FREDIDLE_OBJECT || m_nObjectType == FREDATTACK_OBJECT || m_nObjectType == FREDHURT_OBJECT
		|| m_nObjectType == SWAZIDLE_OBJECT || m_nObjectType == SWAZATTACK_OBJECT || m_nObjectType == SWAZHURT_OBJECT 
		|| m_nObjectType == POLKIDLE_OBJECT || m_nObjectType == POLKATTACK_OBJECT || m_nObjectType == POLKHURT_OBJECT
		|| m_nObjectType == PROJECTILEF_OBJECT || m_nObjectType == PROJECTILES_OBJECT || m_nObjectType == PROJECTILEP_OBJECT 
		|| m_nObjectType == PROJECTILEENEMY1_OBJECT || m_nObjectType == PROJECTILEINVADER_OBJECT || m_nObjectType == ENEMYZOOMERIDLE_OBJECT || m_nObjectType == ENEMYZOOMERIDLEFLIP_OBJECT
		|| m_nObjectType == ENEMYTHIEFIDLE_OBJECT){
		if(m_vPos.x <= g_fScreenScroll - (g_nScreenWidth/2.0f) + SIDEMARGIN){ //left collision
			m_vPos.x = g_fScreenScroll - (g_nScreenWidth / 2.0f) + SIDEMARGIN;
			if(m_nObjectType == ENEMYZOOMERIDLE_OBJECT){
				kill();
				g_cObjectManager.createObject(ENEMYZOOMERBOUNCE_OBJECT, "enemyZoomerBounce", Vector3(m_vPos.x + 10.0f, m_vPos.y, m_vPos.z), Vector3(0.0f, 0, 0), m_nHealth);
			}
			if(m_nObjectType == PROJECTILEF_OBJECT || m_nObjectType == PROJECTILEENEMY1_OBJECT || m_nObjectType == PROJECTILEINVADER_OBJECT || m_nObjectType == PROJECTILETHIEF_OBJECT)
				kill();
		}
		else if(m_vPos.x > g_fScreenScroll + (g_nScreenWidth/2.0f) - SIDEMARGIN){ //right collision
			m_vPos.x = g_fScreenScroll + g_nScreenWidth/2.0f - SIDEMARGIN;
			if(m_nObjectType == ENEMYZOOMERIDLEFLIP_OBJECT){
				kill();
				g_cObjectManager.createObject(ENEMYZOOMERBOUNCEFLIP_OBJECT, "enemyZoomerBounceFlip", Vector3(m_vPos.x - 10.0f, m_vPos.y, m_vPos.z), Vector3(0.0f, 0, 0), m_nHealth);
			}
			if(m_nObjectType == PROJECTILEF_OBJECT || m_nObjectType == PROJECTILES_OBJECT || m_nObjectType == PROJECTILEP_OBJECT 
				|| m_nObjectType == PROJECTILED_OBJECT ||  m_nObjectType == PROJECTILEZ_OBJECT || m_nObjectType == PROJECTILEK_OBJECT
				|| m_nObjectType == PROJECTILEENEMY1_OBJECT || m_nObjectType == PROJECTILEINVADER_OBJECT || m_nObjectType == PROJECTILEINVADER_OBJECT || m_nObjectType == PROJECTILETHIEF_OBJECT)
				kill();

			if (m_nObjectType == ENEMYTHIEFIDLE_OBJECT) m_vVelocity.x = -m_vVelocity.x;
		}
		else if(m_vPos.y <= BOTTOMMARGIN + 50.0f && m_nObjectType == ENEMYTHIEFIDLE_OBJECT){
			m_vVelocity.y = -m_vVelocity.y;
		}
		else if(m_vPos.y <= BOTTOMMARGIN) { //bottom collision
			m_vPos.y = BOTTOMMARGIN;
			if(m_nObjectType == PROJECTILEINVADER_OBJECT || m_nObjectType == PROJECTILEENEMY1_OBJECT || m_nObjectType == PROJECTILEINVADER_OBJECT || m_nObjectType == PROJECTILETHIEF_OBJECT)
				kill();
		}
		else if(m_vPos.y >= g_nScreenHeight + TOPMARGIN){ //top collision
			m_vPos.y = g_nScreenHeight + TOPMARGIN;
			if(m_nObjectType == PROJECTILEINVADER_OBJECT || m_nObjectType == PROJECTILEENEMY1_OBJECT || m_nObjectType == PROJECTILEINVADER_OBJECT || m_nObjectType == PROJECTILETHIEF_OBJECT)
				kill();
		}
		if(m_vPos.y >= g_nScreenHeight + TOPMARGIN - 75.0f && m_nObjectType == ENEMYTHIEFIDLE_OBJECT){ //top collision for thief
			m_vVelocity.y = -m_vVelocity.y;
		}
		if((m_vPos.x <= g_fScreenScroll - (g_nScreenWidth / 2.0f) + SIDEMARGIN) && (m_vPos.y <= BOTTOMMARGIN)){ //bottom left collision
			m_vPos.x = g_fScreenScroll - (g_nScreenWidth / 2.0f) + SIDEMARGIN;
			m_vPos.y = BOTTOMMARGIN;
		}
	}

	if(m_nObjectType == ENEMY1IDLE_OBJECT || m_nObjectType == ENEMYINVADERIDLE_OBJECT || m_nObjectType == ENEMYTHIEFIDLE_OBJECT){
		if (m_vPos.x <= g_fScreenScroll - (g_nScreenWidth / 2.0f) + 100) {
			kill();
			m_nEnemyCount--;
		}
	}
 
	if(m_nObjectType == PROJECTILEENEMY1_OBJECT){
		m_fOrientation = atan2f(m_vVelocity.y, m_vVelocity.x); //rotation around Z axis
	}

	if(g_cTimer.elapsed(m_nLastFlyTime, 17))
	if(m_nObjectType == ENEMY1HURT_OBJECT || m_nObjectType == ENEMYINVADERHURT_OBJECT || m_nObjectType == ENEMYZOOMERHURT_OBJECT || m_nObjectType == ENEMYTHIEFHURT_OBJECT){
		if(m_nObjectType == ENEMYZOOMERHURT_OBJECT)
			m_vPos.x += 16.0f;
		m_vPos.x += 4.0f;
		m_fOrientation -= 20.0f * 3.14f / 180;
	}

	if(m_nObjectType == SHIELD_OBJECT){
		CGameObject* player = g_cObjectManager.GetPlayerObjectPtr();
		m_vPos = player->m_vPos;
		if(!g_bShieldActive)
			kill();
	}

	if(m_nObjectType == ASSISTFRED_OBJECT || m_nObjectType == ASSISTSWAZ_OBJECT || m_nObjectType == ASSISTPOLK_OBJECT){
		CGameObject* player = g_cObjectManager.GetPlayerObjectPtr();
		m_vPos.x = player->m_vPos.x + 17;
		m_vPos.y = player->m_vPos.y - 35;
		if(!g_bAssistActive)
			kill();
	}

	if(m_nObjectType == PROJECTILEF_OBJECT || m_nObjectType == PROJECTILES_OBJECT || m_nObjectType == PROJECTILEP_OBJECT){
		m_fOrientation -= 0.20f * 3.14f / 180;
	}

	if(m_nObjectType == ENEMYINVADERIDLE_OBJECT || m_nObjectType == ENEMYINVADERATTACK_OBJECT || m_nObjectType == ENEMYINVADERHURT_OBJECT){
		CGameObject* player = g_cObjectManager.GetPlayerObjectPtr();
		Vector2 v = player->m_vPos - m_vPos;
		m_fOrientation = atan2(v.y, v.x);
	}

	m_nLastMoveTime = time;  //record time of move
} //move

/// Kill an object by flagging it as dead, and stopping any associated sound.

void CGameObject::kill(){
  m_bIsDead = TRUE;
} //kill

float CGameObject::GetScreenFrameLeft(){
	return g_fScreenScroll - (g_nScreenWidth / 2.0f) + 140.0f;
} //GetScreenFrameLeft

float CGameObject::GetScreenFrameRight(){
	return g_fScreenScroll + (g_nScreenWidth / 2.0f) - 140.0f;
} //GetScreenFrameRight


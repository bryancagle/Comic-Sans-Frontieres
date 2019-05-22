/// \file objman.cpp
/// \brief Code for the object manager class CObjectManager.

#include "objman.h"
#include "debug.h"
#include "defines.h"
#include "timer.h"
#include "Sound.h"
#include "EnemyOne.h"
#include "EnemyInvader.h"
#include "EnemyThief.h"
#include "Random.h"

extern int g_nScreenWidth;
extern int g_nScreenHeight;
extern GameStateType g_nGameState;
extern LevelStateType g_nLevelState;
extern BOOL g_bPlayerIsInvulnerable;
extern BOOL g_bShieldActive;
extern BOOL g_bAssistActive;
extern BOOL g_bSpecialActivate;
extern CGameObject* player;
extern CTimer g_cTimer; 
extern CRandom g_cRandom;
extern CSoundManager* g_pSoundManager;
int m_nEnemyCount;

/// Comparison for depth sorting game objects.
/// To compare two game objects, simply compare their Z coordinates.
/// \param p0 Pointer to game object 0.
/// \param p1 Pointer to game object 1.
/// \return true If object 0 is behind object 1.

bool ZCompare(const CGameObject* p0, const CGameObject* p1){
  return p0->m_vPos.z > p1->m_vPos.z;
} //ZCompare

//initilizes some stats
CObjectManager::CObjectManager(){ 
  m_stlObjectList.clear();
  m_stlNameToObject.clear();
  m_stlNameToObjectType.clear();
  m_nLastGunFireTime = 0;
  m_nStartInvulnerableTime = 0;
	m_bPlayerHit = FALSE;
	m_bGotHit = FALSE;
	m_bCollided = FALSE;
	m_bDiedOfAge = FALSE;
	m_bDiedOfDmg = FALSE;
	m_bDiedOfSpecial = FALSE;
	m_nFiredShots = 0;
	m_nHitShots = 0;
	m_nAmmoCount[0] = 0;
	m_nAmmoCount[1] = 0;
	m_nAmmoCount[2] = 0;
	m_nPlayerLives = 2;
} //constructor

CObjectManager::~CObjectManager(){ 
  for(auto i=m_stlObjectList.begin(); i!=m_stlObjectList.end(); i++)
    delete *i;
} //destructor

/// Insert a map from an object name string to an object type enumeration.
/// \param name Name of an object type
/// \param t Enumerated object type corresponding to that name.

void CObjectManager::InsertObjectType(const char* name, ObjectType t){
  m_stlNameToObjectType.insert(pair<string, ObjectType>(name, t)); 
} //InsertObjectType

/// Get the ObjectType corresponding to a type name string. Returns NUM_OBJECT_TYPES
/// if the name is not in m_stlNameToObjectType.
/// \param name Name of an object type
/// \return Enumerated object type corresponding to that name.

ObjectType CObjectManager::GetObjectType(const char* name){
  unordered_map<string, ObjectType>::iterator i = 
    m_stlNameToObjectType.find(name);
  if(i == m_stlNameToObjectType.end()) //if name not in map
    return NUM_OBJECT_TYPES; //error return
  else return i->second; //return object type
} //GetObjectType

/// Create a new instance of a game object.
/// \param obj The type of the new object
/// \param name The name of object as found in name tag of XML settings file
/// \param s Location.
/// \param v Velocity.
/// \return Pointer to object created.

CGameObject* CObjectManager::createObject(ObjectType obj, const char* name, const Vector3& s, const Vector3& v){
  CGameObject* p;
	
	if(obj == ENEMY1IDLE_OBJECT)
		p = new CEnemyOneObject(name, s, v);
	else if(obj == ENEMYINVADERIDLE_OBJECT)
		p = new CEnemyInvaderObject(name, s, v);
	else if(obj == ENEMYTHIEFIDLE_OBJECT)
		p = new CEnemyThiefObject(name, s, v);
  else p = new CGameObject(obj, name, s, v);

  m_stlObjectList.push_front(p); //insert in object list

  auto i = m_stlNameToObject.find(name);

  if(i == m_stlNameToObject.end()) //if name not in map
    m_stlNameToObject.insert(pair<string, CGameObject*>(name, p)); //put it there
  else{
    m_stlNameToObject.erase(i); //erase old one
    m_stlNameToObject.insert(pair<string, CGameObject*>(name, p)); //put new one there
  } //else

  return p;
} //createObject

CGameObject* CObjectManager::createObject(ObjectType obj, const char* name, const Vector3& s, const Vector3& v, int health) {
	CGameObject* p;

	if(obj == ENEMY1IDLE_OBJECT)
		p = new CEnemyOneObject(name, s, v);
	else if(obj == ENEMYINVADERIDLE_OBJECT)
		p = new CEnemyInvaderObject(name, s, v);
	else if(obj == ENEMYTHIEFIDLE_OBJECT)
		p = new CEnemyThiefObject(name, s, v);
	else p = new CGameObject(obj, name, s, v);
	p->m_nHealth = health;

	m_stlObjectList.push_front(p); //insert in object list

	auto i = m_stlNameToObject.find(name);

	if(i == m_stlNameToObject.end()) //if name not in map
		m_stlNameToObject.insert(pair<string, CGameObject*>(name, p)); //put it there
	else{
		m_stlNameToObject.erase(i); //erase old one
		m_stlNameToObject.insert(pair<string, CGameObject*>(name, p)); //put new one there
	} //else

	return p;
} //createObject

void CObjectManager::clear(){
	for(auto i = m_stlObjectList.begin(); i != m_stlObjectList.end(); i++){
		delete *i;
	}
	m_stlObjectList.clear();

	m_stlNameToObject.erase(m_stlNameToObject.begin(), m_stlNameToObject.end());
	m_stlNameToObject.clear();

	m_stlNameToObjectType.erase(m_stlNameToObjectType.begin(), m_stlNameToObjectType.end());
	m_stlNameToObjectType.clear();
} //clear

/// Move all game objects, while making sure that they wrap around the world correctly.

void CObjectManager::move(){
  const float dX = (float)g_nScreenWidth; // Wrap distance from player.

	///find the player
  auto fredIterator = GetPlayerObject();
  CGameObject* fredObject = GetPlayerObjectPtr();
 
  //move nonplayer objects
  for(auto i=m_stlObjectList.begin(); i!=m_stlObjectList.end();){ //for each object
    CGameObject* curObject = *i++; //current object
    curObject->move(); //move it
	
    //wrap objects a fixed distance from player
    if(curObject != fredObject){ //not the player
      float fredX=0.0f; //player's X coordinate
      if(fredIterator != m_stlNameToObject.end())
        fredX = fredObject->m_vPos.x;

     float& x = curObject->m_vPos.x; //X coordinate of current object
    } //if
  } //for 
  
  CollisionDetection(); //collision detection
  cull(); //cull old objects
  GarbageCollect(); //bring out yer dead!

  if(fredObject->m_nObjectType == SWAZIDLE_OBJECT){
	  g_bPlayerIsInvulnerable = (fredObject->m_nObjectType == SWAZHURT_OBJECT)
	  && (g_cTimer.time() < m_nStartInvulnerableTime + fredObject->m_nInvulnerableTime);
	  
  }
  else if(fredObject->m_nObjectType == POLKIDLE_OBJECT){
	  g_bPlayerIsInvulnerable = (fredObject->m_nObjectType == POLKHURT_OBJECT)
		  && (g_cTimer.time() < m_nStartInvulnerableTime + fredObject->m_nInvulnerableTime);
	}
	else{
		g_bPlayerIsInvulnerable = (fredObject->m_nObjectType == FREDHURT_OBJECT)
			&& (g_cTimer.time() < m_nStartInvulnerableTime + fredObject->m_nInvulnerableTime);
	}
} //move

/// Draw the objects from the object list and the player object. Care
/// must be taken to draw them from back to front.

void CObjectManager::draw(){
  m_stlObjectList.sort(ZCompare); //depth sort
  for(auto i = m_stlObjectList.begin(); i != m_stlObjectList.end(); i++) //for each object
    (*i)->draw();
} //draw

/// Get a pointer to an object by name, nullptr if it doesn't exist.
/// \param name Name of object.
/// \return Pointer to object created with that name, if it exists.

CGameObject* CObjectManager::GetObjectByName(const char* name){ 
  unordered_map<string, CGameObject*>::iterator 
    current = m_stlNameToObject.find((string)name);
  if(current != m_stlNameToObject.end())
    return current->second;
  else return nullptr;
} //GetObjectByName

/// Distance between objects.
/// \param pointer to first object 
/// \param pointer to second object
/// \return distance between the two objects

float CObjectManager::distance(CGameObject *g0, CGameObject *g1){ 
  if(g0 == nullptr || g1 == nullptr)return -1; //bail if bad pointer
  const float fWorldWidth = 2.0f * (float)g_nScreenWidth; //world width
  float x = (float)fabs(g0->m_vPos.x - g1->m_vPos.x); //x distance
  float y = (float)fabs(g0->m_vPos.y - g1->m_vPos.y); //y distance
  if(x > fWorldWidth) x -= (float)fWorldWidth; //compensate for wrap-around world
  return sqrtf(x*x + y*y);
} //distance

/// Fire a letter shot

void CObjectManager::FireGun(){   
  std::unordered_map<std::string, CGameObject*>::iterator fredIterator = GetPlayerObject();
  if(fredIterator == m_stlNameToObject.end())return; //this should of course never happen
  const CGameObject* fredObject = fredIterator->second;
	if(fredObject->m_nObjectType == FREDHURT_OBJECT || fredObject->m_nObjectType == SWAZHURT_OBJECT || fredObject->m_nObjectType == POLKHURT_OBJECT)return; //dead players can't fire letters
  if(fredObject->m_bCanFire == FALSE)return; //Can't fire if end of level reached
  if(g_cTimer.elapsed(m_nLastGunFireTime, 200)){ //slow down firing rate
    const float fAngle = fredObject->m_fOrientation;
    const float fSine = sin(fAngle);
    const float fCosine = cos(fAngle);

    //enter the number of pixels from center of character to gun
    const float fGunDx =  17.0f; 
    const float fGunDy =  5.0f;

    //initial bullet position
    const Vector3 s = fredObject->m_vPos +
      Vector3(fGunDx*fCosine - fGunDy*fSine, fGunDx*fSine - fGunDy*fCosine, 0); 

    //velocity of bullet
    const float BULLETSPEED = 20.0f;
		const Vector3 v = -BULLETSPEED * Vector3(-fCosine, -fSine, 0);
	  
		if(fredObject->m_nObjectType == FREDATTACK_OBJECT){
			createObject(PROJECTILEF_OBJECT, "projectileF", s, v); //create bullet
			if(g_bAssistActive)
			createObject(PROJECTILEF_OBJECT, "projectileF", Vector3(s.x, s.y - 35, s.z), v); //create bullet
		}
		else if(fredObject->m_nObjectType == SWAZATTACK_OBJECT){
			createObject(PROJECTILES_OBJECT, "projectileS", s, v); //create bullet
			if(g_bAssistActive)
				createObject(PROJECTILES_OBJECT, "projectileS", Vector3(s.x, s.y - 35, s.z), v); //create bullet
		}
		else if(fredObject->m_nObjectType == POLKATTACK_OBJECT){
			createObject(PROJECTILEP_OBJECT, "projectileP", s, v); //create bullet
			if(g_bAssistActive)
				createObject(PROJECTILEP_OBJECT, "projectileP", Vector3(s.x, s.y - 35, s.z), v); //create bullet
		}
		m_nFiredShots += 1;
	} //if
} //FireGun

/// Fires pierce shot that goes through all enemies

void CObjectManager::FirePierce(){
	std::unordered_map<std::string, CGameObject*>::iterator fredIterator = GetPlayerObject();
	if(fredIterator == m_stlNameToObject.end())return; //this should of course never happen
	const CGameObject* fredObject = fredIterator->second;
	if(fredObject->m_nObjectType == FREDHURT_OBJECT || fredObject->m_nObjectType == SWAZHURT_OBJECT || fredObject->m_nObjectType == POLKHURT_OBJECT)return; //hurt players can't fire pierce shots
	if(fredObject->m_bCanFire == FALSE)return; //Can't fire if end of level reached
	if(g_cTimer.elapsed(m_nLastGunFireTime, 200)){ //slow down firing rate
		const float fAngle = fredObject->m_fOrientation;
		const float fSine = sin(fAngle);
		const float fCosine = cos(fAngle);

		//enter the number of pixels from center of character to gun
		const float fGunDx = 17.0f;
		const float fGunDy = 5.0f;

		//initial bullet position
		const Vector3 s = fredObject->m_vPos +
			Vector3(fGunDx*fCosine - fGunDy*fSine, fGunDx*fSine - fGunDy*fCosine, 0);

		//velocity of bullet
		const float BULLETSPEED = 20.0f;
		const Vector3 v = -BULLETSPEED * Vector3(-fCosine, -fSine, 0);

		if(fredObject->m_nObjectType == FREDATTACK_OBJECT){
			createObject(PROJECTILED_OBJECT, "projectileD", s, v); //create bullet
			if(g_bAssistActive)
				createObject(PROJECTILED_OBJECT, "projectileD", Vector3(s.x, s.y - 35, s.z), v); //create bullet
		}
		else if(fredObject->m_nObjectType == SWAZATTACK_OBJECT){
			createObject(PROJECTILEZ_OBJECT, "projectileZ", s, v); //create bullet
			if(g_bAssistActive)
				createObject(PROJECTILEZ_OBJECT, "projectileZ", Vector3(s.x, s.y - 35, s.z), v); //create bullet
		}
		else if(fredObject->m_nObjectType == POLKATTACK_OBJECT){
			createObject(PROJECTILEK_OBJECT, "projectileK", s, v); //create bullet
			if(g_bAssistActive)
				createObject(PROJECTILEK_OBJECT, "projectileK", Vector3(s.x, s.y - 35, s.z), v); //create bullet
		}
		reduceAmmoCount(2);
		m_nFiredShots += 1;
	} //if
} //FirePierce

/// Activates shield, protecting player for one shot

void CObjectManager::CreateShield(){
	std::unordered_map<std::string, CGameObject*>::iterator fredIterator = GetPlayerObject();
	if(fredIterator == m_stlNameToObject.end())return; //this should of course never happen
	const CGameObject* fredObject = fredIterator->second;
	if(fredObject->m_nObjectType == FREDHURT_OBJECT || fredObject->m_nObjectType == SWAZHURT_OBJECT || fredObject->m_nObjectType == POLKHURT_OBJECT)return; //hurt players can't activate shields
	if(fredObject->m_bCanFire == FALSE)return; //Can't fire if end of level reached

	//initial shield position
	const Vector3 s = fredObject->m_vPos;

	createObject(SHIELD_OBJECT, "shield", s, Vector3(0,0,0)); //create bullet
	reduceAmmoCount(0);
} //CreateShield

/// Activates assist, giving player double shots

void CObjectManager::CreateAssist(){
	std::unordered_map<std::string, CGameObject*>::iterator fredIterator = GetPlayerObject();
	if(fredIterator == m_stlNameToObject.end())return; //this should of course never happen
	const CGameObject* fredObject = fredIterator->second;
	if(fredObject->m_nObjectType == FREDHURT_OBJECT || fredObject->m_nObjectType == SWAZHURT_OBJECT || fredObject->m_nObjectType == POLKHURT_OBJECT)return; //hurt characters can't summon assists
	if(fredObject->m_bCanFire == FALSE)return; //Can't fire if end of level reached
	const float fAngle = fredObject->m_fOrientation;
	const float fSine = sin(fAngle);
	const float fCosine = cos(fAngle);

	//enter the number of pixels from center of character to gun
	const float fGunDx = 17.0f;
	const float fGunDy = 17.0f;

	//initial bullet position
	const Vector3 s = fredObject->m_vPos +
		Vector3(fGunDx*fCosine - fGunDy*fSine, fGunDx*fSine - fGunDy*fCosine, 0);

	if(fredObject->m_nObjectType == FREDATTACK_OBJECT)
		createObject(ASSISTFRED_OBJECT, "assistFred", s, Vector3(0, 0, 0)); //create bullet
	else if(fredObject->m_nObjectType == SWAZATTACK_OBJECT)
		createObject(ASSISTSWAZ_OBJECT, "assistSwaz", s, Vector3(0, 0, 0)); //create bullet
	else if(fredObject->m_nObjectType == POLKATTACK_OBJECT)
		createObject(ASSISTPOLK_OBJECT, "assistPolk", s, Vector3(0, 0, 0)); //create bullet

	reduceAmmoCount(1);
} //CreateAssist

/// Cull old objects.
/// Run through the objects in the object list and compare their age to
/// their life span. Kill any that are too old. Immortal objects are
/// flagged with a negative life span, so ignore those.

void CObjectManager::cull(){ 
  for(auto i=m_stlObjectList.begin(); i!=m_stlObjectList.end();){
    CGameObject* object = *i++; //current object

    //died of old age
    if(object->m_nLifeTime > 0 && //if mortal and ...
    (g_cTimer.time() - object->m_nBirthTime > object->m_nLifeTime)){ //...old...
			m_bDiedOfAge = TRUE;
      object->kill(); //slay it
      CreateNextIncarnation(object); //create next in the animation sequence
    } //if

    //one shot animation 
    if(object->m_nFrameCount > 1 && !object->m_bCycleSprite && //if plays one time...
      object->m_nCurrentFrame >= object->m_nAnimationFrameCount){ //and played once already...
        object->kill(); //slay it
        CreateNextIncarnation(object); //create next in the animation sequence
    } //if
  } //for
} //cull

/// Get an iterator to the player object, which may be Fred, Swaz, or Polk.
/// \return Iterator to the player object.

unordered_map<string, CGameObject*>::iterator  CObjectManager::GetPlayerObject(){
  auto i = m_stlNameToObject.find("fredIdle");
  
  if(i != m_stlNameToObject.end())return i; 
  else{i = m_stlNameToObject.find("fredAttack");
    if(i != m_stlNameToObject.end())return i; 
	else{i = m_stlNameToObject.find("fredHurt");
      if(i != m_stlNameToObject.end())return i; 
	  else{i = m_stlNameToObject.find("fredLost");
		if (i != m_stlNameToObject.end())return i; 
		else{i = m_stlNameToObject.find("swazIdle");
		  if (i != m_stlNameToObject.end())return i;
		  else {i = m_stlNameToObject.find("swazAttack");
			if (i != m_stlNameToObject.end())return i;
			 else {i = m_stlNameToObject.find("swazHurt");
			  if (i != m_stlNameToObject.end())return i;
			  else {i = m_stlNameToObject.find("swazLost");
  			    if (i != m_stlNameToObject.end())return i;
			    else {i = m_stlNameToObject.find("polkIdle");
				  if (i != m_stlNameToObject.end())return i;
				  else {i = m_stlNameToObject.find("polkAttack");
					if (i != m_stlNameToObject.end())return i;
					else {i = m_stlNameToObject.find("polkHurt");
					  if (i != m_stlNameToObject.end())return i;
						else {return m_stlNameToObject.find("polkLost");
							  }
						  }
						}
					 }
				  }
			  }
		  }
		} //else
	  } //else
    } //else
  } //else
} //GetPlayerObject

/// Get a pointer to the player object

CGameObject* CObjectManager::GetPlayerObjectPtr(){
  auto i = GetPlayerObject();
  if(i != m_stlNameToObject.end())
    return (*i).second;
  else return nullptr;
} //GetPlayerObjectPtr

/// Create the object next in the appropriate series (object, exploding
/// object, dead object). If there's no "next" object, do nothing.
/// \param object Pointer to the object to be replaced

void CObjectManager::CreateNextIncarnation(CGameObject* object){ 
  Vector3 p = object->m_vPos, v = object->m_vVelocity;
  auto i = GetPlayerObject();

  switch(object->m_nObjectType){
    case FREDIDLE_OBJECT: 
			if(i != m_stlNameToObject.end()) //if name in map
				m_stlNameToObject.erase(i); //erase it
			if(!m_bDiedOfDmg)
				player = createObject(FREDHURT_OBJECT, "fredHurt", p, v);
			else
				player = createObject(FREDLOST_OBJECT, "fredLost", p, v);
			m_nStartInvulnerableTime = g_cTimer.time(); //make invulnerable
			break;

		case FREDATTACK_OBJECT:
			if(i != m_stlNameToObject.end()) //if name in map
				m_stlNameToObject.erase(i); //erase it
			if(m_bDiedOfDmg){
				player = createObject(FREDLOST_OBJECT, "fredLost", p, v);
			}
			else if(m_bPlayerHit){
				player = createObject(FREDHURT_OBJECT, "fredHurt", p, v);
				m_nStartInvulnerableTime = g_cTimer.time(); //make invulnerable
			}
			else if(m_bDiedOfAge){
				player = createObject(FREDIDLE_OBJECT, "fredIdle", p, v);
				m_bDiedOfAge = FALSE;
			}
			//m_nStartInvulnerableTime = g_cTimer.time(); //make invulnerable
			break;

		case FREDHURT_OBJECT:
			if(i != m_stlNameToObject.end()) //if name in map
				m_stlNameToObject.erase(i); //erase it
				player = createObject(FREDIDLE_OBJECT, "fredIdle", p, v);
			break;

		case SWAZIDLE_OBJECT:
			if(i != m_stlNameToObject.end()) //if name in map
				m_stlNameToObject.erase(i); //erase it
			if(!m_bDiedOfDmg)
				player = createObject(SWAZHURT_OBJECT, "swazHurt", p, v);
			else
				player = createObject(SWAZLOST_OBJECT, "swazLost", p, v);
			m_nStartInvulnerableTime = g_cTimer.time(); //make invulnerable
			break;

		case SWAZATTACK_OBJECT:
			if(i != m_stlNameToObject.end()){ //if name in map
				m_stlNameToObject.erase(i); //erase it
			}
			if(m_bDiedOfDmg){
				player = createObject(SWAZLOST_OBJECT, "swazLost", p, v);
				m_nStartInvulnerableTime = g_cTimer.time(); //make invulnerable
			}
			else if(m_bPlayerHit){
				player = createObject(SWAZHURT_OBJECT, "swazHurt", p, v);
				m_nStartInvulnerableTime = g_cTimer.time(); //make invulnerable
			}
			else if(m_bDiedOfAge){
				player = createObject(SWAZIDLE_OBJECT, "swazIdle", p, v);
				m_bDiedOfAge = FALSE;
			}
			break;

		case SWAZHURT_OBJECT:
			if(i != m_stlNameToObject.end()) //if name in map
				m_stlNameToObject.erase(i); //erase it
			player = createObject(SWAZIDLE_OBJECT, "swazIdle", p, v);
			break;

		case POLKIDLE_OBJECT:
			if(i != m_stlNameToObject.end()) //if name in map
				m_stlNameToObject.erase(i); //erase it
			if(!m_bDiedOfDmg)
				player = createObject(POLKHURT_OBJECT, "polkHurt", p, v);
			else
				player = createObject(POLKLOST_OBJECT, "polkLost", p, v);
			m_nStartInvulnerableTime = g_cTimer.time(); //make invulnerable
			break;

		case POLKATTACK_OBJECT:
			if(i != m_stlNameToObject.end()){ //if name in map
				m_stlNameToObject.erase(i); //erase it
			}
			if(m_bDiedOfDmg){
				player = createObject(POLKLOST_OBJECT, "polkLost", p, v);
				m_nStartInvulnerableTime = g_cTimer.time(); //make invulnerable
			}
			else if(m_bPlayerHit){
				player = createObject(POLKHURT_OBJECT, "polkHurt", p, v);
				m_nStartInvulnerableTime = g_cTimer.time(); //make invulnerable
			}
			else if(m_bDiedOfAge){
				player = createObject(POLKIDLE_OBJECT, "polkIdle", p, v);
				m_bDiedOfAge = FALSE;
			}
			break;

		case POLKHURT_OBJECT:
			if(i != m_stlNameToObject.end()) //if name in map
				m_stlNameToObject.erase(i); //erase it
			player = createObject(POLKIDLE_OBJECT, "polkIdle", p, v);
			break;

		case ENEMY1AFTER_OBJECT:
			createObject(ENEMY1HURT_OBJECT, "enemy1Hurt", p, v, object->m_nHealth);
			break;

		case ENEMY1IDLE_OBJECT:
			createObject(ENEMY1HURT_OBJECT, "enemy1Hurt", p, v, object->m_nHealth);
			break;

		case ENEMY1HURT_OBJECT:
			if(m_bDiedOfAge){
				if(object->m_nHealth >= 1) 
					createObject(ENEMY1AFTER_OBJECT, "enemy1Idle", p, v, object->m_nHealth);
				else
					createObject(ENEMYEXIT_OBJECT, "enemyExit", Vector3(p.x, p.y, 376), Vector3(0, 0, 0));
			}
			break;

		case ENEMYINVADERIDLE_OBJECT:
			createObject(ENEMYINVADERHURT_OBJECT, "enemyInvaderHurt", p, v, object->m_nHealth);
			break;

		case ENEMYINVADERATTACK_OBJECT:
			if(m_bDiedOfAge)
				createObject(ENEMYINVADERIDLE_OBJECT, "enemyInvaderIdle", p, v, object->m_nHealth);
			else
				createObject(ENEMYINVADERHURT_OBJECT, "enemyInvaderHurt", p, v, object->m_nHealth);
			break;

		case ENEMYINVADERHURT_OBJECT:
			if(m_bDiedOfAge){
				if (object->m_nHealth >= 1)
				createObject(ENEMYINVADERIDLE_OBJECT, "enemyInvaderIdle", p, v, object->m_nHealth);
				else
				createObject(ENEMYEXIT_OBJECT, "enemyExit", Vector3(p.x, p.y, 376), Vector3(0, 0, 0));
			}
			break;

		case ENEMYZOOMERIDLE_OBJECT:
			createObject(ENEMYZOOMERHURT_OBJECT, "enemyZoomerHurt", p, v, object->m_nHealth);
			break;

		case ENEMYZOOMERIDLEFLIP_OBJECT:
			createObject(ENEMYZOOMERHURT_OBJECT, "enemyZoomerHurt", p, v, object->m_nHealth);
			break;

		case ENEMYZOOMERBOUNCE_OBJECT:
			if(m_bDiedOfSpecial)
				createObject(ENEMYZOOMERHURT_OBJECT, "enemyZoomerHurt", p, v, object->m_nHealth);
			else
				createObject(ENEMYZOOMERIDLEFLIP_OBJECT, "enemyZoomerIdleFlip", p, Vector3(15.0f, 0, 0), object->m_nHealth);
			break;

		case ENEMYZOOMERBOUNCEFLIP_OBJECT:
			if(m_bDiedOfSpecial)
				createObject(ENEMYZOOMERHURT_OBJECT, "enemyZoomerHurt", p, v, object->m_nHealth);
			else
				createObject(ENEMYZOOMERIDLE_OBJECT, "enemyZoomerIdle", p, Vector3(-15.0f, 0, 0), object->m_nHealth);
			break;

		case ENEMYZOOMERHURT_OBJECT:
			if(m_bDiedOfAge){
				if (object->m_nHealth >= 1)
					createObject(ENEMYZOOMERIDLE_OBJECT, "enemyZoomerIdle", p, Vector3(-15.0f, 0, 0), object->m_nHealth);
				else
					createObject(ENEMYEXIT_OBJECT, "enemyExit", Vector3(p.x, p.y, 376), Vector3(0, 0, 0));
			}
			break;

		case ENEMYTHIEFIDLE_OBJECT:
			createObject(ENEMYTHIEFHURT_OBJECT, "enemyThiefHurt", p, v, object->m_nHealth);
			break;

		case ENEMYTHIEFATTACK_OBJECT:
			if(m_bDiedOfAge){
				int randDir = g_cRandom.number(-1, 1);
				createObject(ENEMYTHIEFIDLE_OBJECT, "enemyThiefIdle", p, Vector3(1.5f, 3.0f * randDir, 0.0f), object->m_nHealth);
			}
			else
				createObject(ENEMYTHIEFHURT_OBJECT, "enemyThiefHurt", p, v, object->m_nHealth);
			break;

		case ENEMYTHIEFHURT_OBJECT:
			if(m_bDiedOfAge){
				if (object->m_nHealth >= 1)
					createObject(ENEMYTHIEFIDLE_OBJECT, "enemyThiefIdle", p, v, object->m_nHealth);
				else
					createObject(ENEMYEXIT_OBJECT, "enemyExit", p, Vector3(0.0f, 0.0f, 0.0f), object->m_nHealth);
			}
			break;

		//depending on the stage, it will randomly spawn enemies from the  entrance clouds
		case ENEMYENTRY_OBJECT:
			if(m_bDiedOfAge){
				if(g_nLevelState == COMICWORLD_STATE){
					int randEnemy = g_cRandom.number(1, 2);
					switch (randEnemy){
					case 1:
						createObject(ENEMYZOOMERIDLE_OBJECT, "enemyZoomerIdle", p, Vector3(-15.0f, v.y, v.z));
						break;

					case 2:
						createObject(ENEMY1IDLE_OBJECT, "enemy1Idle", p, v);
						break;
					} //switch
				} //if
				else if(g_nLevelState == FANTASY_STATE){
					int randEnemy = g_cRandom.number(1, 3);
					switch(randEnemy){
					case 1:
						createObject(ENEMY1IDLE_OBJECT, "enemy1Idle", p, v);
						break;

					case 2:
						createObject(ENEMYZOOMERIDLE_OBJECT, "enemyZoomerIdle", p, Vector3(-15.0f, v.y, v.z));
						break;

					case 3:
						createObject(ENEMYINVADERIDLE_OBJECT, "enemyInvaderIdle", p, v);
						break;
					} //switch
				} //else if
				else{
					int randEnemy = g_cRandom.number(1, 4);
					switch(randEnemy){
					case 1:
						createObject(ENEMYINVADERIDLE_OBJECT, "enemyInvaderIdle", p, v);
						break;

					case 2:
						createObject(ENEMY1IDLE_OBJECT, "enemy1Idle", p, v);
						break;

					case 3:
						createObject(ENEMYZOOMERIDLE_OBJECT, "enemyZoomerIdle", p, Vector3(-15.0f, v.y, v.z));
						break;

					case 4:
						createObject(ENEMYTHIEFIDLE_OBJECT, "enemyThiefIdle", p, Vector3(1.0f, 3.0f, 0.0f));
						break;
					} //switch
				} //else
			}
			break;

		case ENEMYEXIT_OBJECT:
			if(m_bDiedOfAge){
				dropChance(Vector3(p.x, p.y, p.z - 1), v);
				m_nEnemyCount--;
			}
			break;

		case PROJECTILEF_OBJECT:
			createObject(EXPLOSION_OBJECT, "explosion", Vector3(p.x, p.y, p.z-30.0f), Vector3(0, 0, 0));
			break;

		case PROJECTILES_OBJECT:
			createObject(EXPLOSION_OBJECT, "explosion", Vector3(p.x, p.y, 376-30.0f), Vector3(0,0,0));
			break;

		case PROJECTILEP_OBJECT:
			createObject(EXPLOSION_OBJECT, "explosion", Vector3(p.x, p.y, 376-30.0f), Vector3(0, 0, 0));
			break;
  } //switch
} //CreateNextIncarnation

/// Master collision detection function.
/// Compare every object against every other object for collision. Only
/// bullets can collide right now.

void CObjectManager::CollisionDetection(){ 
  for(auto i=m_stlObjectList.begin(); i!=m_stlObjectList.end(); i++)
    if((*i)->m_nObjectType == PROJECTILEF_OBJECT || (*i)->m_nObjectType == PROJECTILES_OBJECT || (*i)->m_nObjectType == PROJECTILEP_OBJECT
		|| (*i)->m_nObjectType == PROJECTILED_OBJECT || (*i)->m_nObjectType == PROJECTILEZ_OBJECT || (*i)->m_nObjectType == PROJECTILEK_OBJECT
		|| (*i)->m_nObjectType == PROJECTILETHIEF_OBJECT) //and is a bullet
      CollisionDetection(*i); //check every object for collision with this bullet
  
  //player object
	if (!g_bPlayerIsInvulnerable) {//if player is vulnerable
		CollisionDetection(GetPlayerObjectPtr());
	}
} //CollisionDetection

/// Given an object pointer, compare that object against every other 
/// object for collision. If a collision is detected, replace the object hit
/// with the next in series (if one exists), and kill the object doing the
/// hitting (bullets don't go through objects in this game).
/// \param p Pointer to the object to be compared against.

void CObjectManager::CollisionDetection(CGameObject* p){ 
	if(p != nullptr){
		for (auto j = m_stlObjectList.begin(); j != m_stlObjectList.end(); j++){
			if(p->m_nObjectType == PROJECTILETHIEF_OBJECT && !p->m_bVulnerable && (*j)->m_nObjectType == ENEMYTHIEFATTACK_OBJECT){
				if(p->m_vPos.x - 15.0f > (*j)->m_vPos.x){
					p->kill();
					(*j)->kill();
					CreateNextIncarnation(p);
					CreateNextIncarnation(*j);
				}
			}
			else CollisionDetection(p, *j);
		} //for
		m_bCollided = FALSE;
	} //if
} //CollisionDetection

/// Given 2 object pointers, see whether the objects collide. 
/// If a collision is detected, replace the object hit
/// with the next in series (if one exists), and kill the object doing the
/// hitting (bullets don't go through objects in this game).
/// \param i iterator of the object to compare against

void CObjectManager::CollisionDetection(CGameObject* p0, CGameObject* p1)
{ 
  if(p1->m_bVulnerable && distance(p0, p1) < 25.0f){
	  //If player gets hit, take damage and lose some points
	  if((p0->m_nObjectType == FREDIDLE_OBJECT || p0->m_nObjectType == FREDATTACK_OBJECT 
		  || p0->m_nObjectType == SWAZIDLE_OBJECT || p0->m_nObjectType == SWAZATTACK_OBJECT
		  || p0->m_nObjectType == POLKIDLE_OBJECT || p0->m_nObjectType == POLKATTACK_OBJECT)
		  && (p1->m_nObjectType == ENEMY1IDLE_OBJECT || p1->m_nObjectType == PROJECTILEENEMY1_OBJECT 
				|| p1->m_nObjectType == PROJECTILEINVADER_OBJECT || p1->m_nObjectType == ENEMYINVADERIDLE_OBJECT || p1->m_nObjectType == ENEMYINVADERATTACK_OBJECT
				|| p1->m_nObjectType == ENEMYZOOMERIDLE_OBJECT || p1->m_nObjectType == ENEMYZOOMERIDLEFLIP_OBJECT
				|| p1->m_nObjectType == ENEMYTHIEFIDLE_OBJECT || p1->m_nObjectType == PROJECTILETHIEF_OBJECT) && !m_bCollided){

			if(p1->m_nObjectType == PROJECTILETHIEF_OBJECT){
				if(p1->m_vPos.x > p0->m_vPos.x) p1->m_vVelocity = -10*(p1->m_vVelocity);
				p1->m_bVulnerable = FALSE;
				BOOL lettersToSteal[3] = {FALSE, FALSE, FALSE};
				int numToSteal = 0;
				for(int i = 0; i < 3; i++){
					if(m_nAmmoCount[i] > 0){
						lettersToSteal[i] = TRUE;
						numToSteal += 1;
					}
				}
				//picks a random letter to steal that the player has
				int randLet;
				switch(numToSteal){
				case 1:
					if(lettersToSteal[0])
						reduceAmmoCount(0);
					else if (lettersToSteal[1])
						reduceAmmoCount(1);
					else
						reduceAmmoCount(2);
					break;

				case 2:
					randLet = g_cRandom.number(1, 2);
					if(!lettersToSteal[0])
						reduceAmmoCount(randLet);
					else if(!lettersToSteal[1]){
						if(randLet == 1) reduceAmmoCount(randLet - 1);
						else reduceAmmoCount(randLet);
					}
					else if(!lettersToSteal[2]){
						if(randLet == 2) reduceAmmoCount(randLet - 2);
						else reduceAmmoCount(randLet);
					}
					break;

				case 3:
					randLet = g_cRandom.number(0, 2);
					reduceAmmoCount(randLet);
					break;
				}
			}

			if(m_nPlayerHealth > 0 && !g_bShieldActive){
			  m_nPlayerHealth -= 1;
			  if(m_nScore > 0)
					m_nScore -= 50;
			  else
		      m_nScore = 0;

			  if(m_nPlayerHealth == 0){
				  switch(p0->m_nObjectType){
						case POLKIDLE_OBJECT: m_nPlayerHealth = 4; break;
						case POLKATTACK_OBJECT: m_nPlayerHealth = 4; break;
						case FREDIDLE_OBJECT: m_nPlayerHealth = 6; break;
						case FREDATTACK_OBJECT: m_nPlayerHealth = 6; break;
						case SWAZIDLE_OBJECT: m_nPlayerHealth = 8; break;
						case SWAZATTACK_OBJECT: m_nPlayerHealth = 8; break;
				  }
				  
					if(m_nPlayerLives == 0){
						g_nGameState = GAMEOVER_GAMESTATE;
						m_bDiedOfDmg = TRUE;
						if(g_nLevelState == COMICWORLD_STATE) g_pSoundManager->stop(UNLEASH_SOUND);
						else if(g_nLevelState == FANTASY_STATE) g_pSoundManager->stop(MOOSEHEADHONK_SOUND);
						else if(g_nLevelState == CITY_STATE) g_pSoundManager->stop(MOOSEHEADHONK_SOUND);
						g_pSoundManager->play(GAMEOVER_SOUND);
					} //if
					else m_nPlayerLives -= 1;
			  } //if
		  } //if
			
			//create hurt player if no shield is activated
			if(!g_bShieldActive){
				m_bPlayerHit = TRUE;
				g_pSoundManager->play(PLAYERHIT_SOUND);
				p0->kill();
				CreateNextIncarnation(p0);
				m_bPlayerHit = FALSE;
			}
			else g_pSoundManager->play(SHIELDHIT_SOUND);

			//power down
			if(g_bShieldActive && g_bAssistActive) g_bShieldActive = FALSE;
			else if(!g_bShieldActive && g_bAssistActive) g_bAssistActive = FALSE;
			else if(g_bShieldActive && !g_bAssistActive) g_bShieldActive = FALSE;
			m_bGotHit = TRUE;
			m_bCollided = TRUE;

			if(p1->m_nObjectType != PROJECTILETHIEF_OBJECT){
				p1->kill();
				CreateNextIncarnation(p1);
			}
		  return;
	  }
	  //Spiked enemies are industructable, so only kill the player's bullet when they collide
	  if((p1->m_nObjectType == ENEMY1IDLE_OBJECT || p1->m_nObjectType == PROJECTILEENEMY1_OBJECT || p1->m_nObjectType == PROJECTILEINVADER_OBJECT || p1->m_nObjectType == PROJECTILETHIEF_OBJECT)
				&& (p0->m_nObjectType == PROJECTILEF_OBJECT || p0->m_nObjectType == PROJECTILES_OBJECT || p0->m_nObjectType == PROJECTILEP_OBJECT)){
			p0->kill();
			if(p1->m_nObjectType == PROJECTILEINVADER_OBJECT || p1->m_nObjectType == PROJECTILETHIEF_OBJECT)
				p1->kill();
			g_pSoundManager->play(EXPLOSION_SOUND);
			CreateNextIncarnation(p0);
		  return;
	  }
	  //If pierce shot hits, the enemy will always die and the shot keeps going
		if((p1->m_nObjectType == ENEMY1AFTER_OBJECT || p1->m_nObjectType == ENEMY1IDLE_OBJECT 
			|| p1->m_nObjectType == ENEMYINVADERIDLE_OBJECT || p1->m_nObjectType == ENEMYINVADERATTACK_OBJECT || p1->m_nObjectType == PROJECTILEINVADER_OBJECT
			|| p1->m_nObjectType == ENEMYZOOMERIDLE_OBJECT || p1->m_nObjectType == ENEMYZOOMERIDLEFLIP_OBJECT || p1->m_nObjectType == ENEMYZOOMERBOUNCEFLIP_OBJECT || p1->m_nObjectType == ENEMYZOOMERBOUNCE_OBJECT
			|| p1->m_nObjectType == ENEMYTHIEFIDLE_OBJECT || p1->m_nObjectType == ENEMYTHIEFATTACK_OBJECT)
			&& (p0->m_nObjectType == PROJECTILED_OBJECT || p0->m_nObjectType == PROJECTILEZ_OBJECT || p0->m_nObjectType == PROJECTILEK_OBJECT)){
			p1->kill();
			g_pSoundManager->play(EXPLOSION_SOUND);
			m_nScore += 300;
			m_nHitShots += 1;
			p1->m_nHealth -= 3;
			CreateNextIncarnation(p1);
			return;
		}

		//if regular shot hits vulnerable enemy,  enemy takes damage depending on the shot
	  if((p1->m_nObjectType == ENEMY1AFTER_OBJECT || p1->m_nObjectType == ENEMYINVADERIDLE_OBJECT || p1->m_nObjectType == ENEMYINVADERATTACK_OBJECT
		  || p1->m_nObjectType == ENEMYZOOMERIDLE_OBJECT || p1->m_nObjectType == ENEMYZOOMERIDLEFLIP_OBJECT || p1->m_nObjectType == ENEMYZOOMERBOUNCEFLIP_OBJECT || p1->m_nObjectType == ENEMYZOOMERBOUNCE_OBJECT
			|| p1->m_nObjectType == ENEMYTHIEFIDLE_OBJECT || p1->m_nObjectType == ENEMYTHIEFATTACK_OBJECT)
			&& (p0->m_nObjectType == PROJECTILEF_OBJECT || p0->m_nObjectType == PROJECTILES_OBJECT || p0->m_nObjectType == PROJECTILEP_OBJECT)){
			p1->kill();
			p0->kill();
			g_pSoundManager->play(EXPLOSION_SOUND);
		  m_nScore += 100;
		  m_nHitShots += 1;
		  if(p0->m_nObjectType == PROJECTILEP_OBJECT)
			  p1->m_nHealth -= 3;
		  else if(p0->m_nObjectType == PROJECTILEF_OBJECT)
			  p1->m_nHealth -= 2;
		  else if(p0->m_nObjectType == PROJECTILES_OBJECT)
			  p1->m_nHealth -= 1;

			CreateNextIncarnation(p0);
			CreateNextIncarnation(p1);
		  return;
	  }
	  //When player gets health
	  if((p0->m_nObjectType == FREDIDLE_OBJECT || p0->m_nObjectType == FREDATTACK_OBJECT 
		  || p0->m_nObjectType == SWAZIDLE_OBJECT || p0->m_nObjectType == SWAZATTACK_OBJECT
		  || p0->m_nObjectType == POLKIDLE_OBJECT || p0->m_nObjectType == POLKATTACK_OBJECT)
		  && p1->m_nObjectType == ITEMHEART_OBJECT){
			p1->kill();
			g_pSoundManager->play(GETITEM_SOUND);

		  if(m_nPlayerHealth >= 3 && p0->m_nObjectType == POLKIDLE_OBJECT || p0->m_nObjectType == POLKATTACK_OBJECT)
			  m_nPlayerHealth = 4;
			else if(m_nPlayerHealth >= 5 && p0->m_nObjectType == FREDIDLE_OBJECT || p0->m_nObjectType == FREDATTACK_OBJECT)
			  m_nPlayerHealth = 6;
		  else if(m_nPlayerHealth >= 7 && p0->m_nObjectType == SWAZIDLE_OBJECT || p0->m_nObjectType == SWAZATTACK_OBJECT)
			  m_nPlayerHealth = 8;
		  else
			  m_nPlayerHealth += 2;
		  m_nScore += 50;
		  return;
	  }
	  //Player gets a letter item for this block and the two that follow
	  if((p0->m_nObjectType == FREDIDLE_OBJECT || p0->m_nObjectType == FREDATTACK_OBJECT
		  || p0->m_nObjectType == SWAZIDLE_OBJECT || p0->m_nObjectType == SWAZATTACK_OBJECT
		  || p0->m_nObjectType == POLKIDLE_OBJECT || p0->m_nObjectType == POLKATTACK_OBJECT)
		  && (p1->m_nObjectType == ITEMR_OBJECT || p1->m_nObjectType == ITEMW_OBJECT || p1->m_nObjectType == ITEMO_OBJECT)){
			p1->kill();
		  if(m_nAmmoCount[0] < 5)
			  m_nAmmoCount[0]++;
		  m_nScore += 50;
			if(m_nAmmoCount[0] > 0 && m_nAmmoCount[1] > 0 && m_nAmmoCount[2] > 0 && m_nAmmoCount[3] > 0)
				g_pSoundManager->play(SPECIALREADY_SOUND);
			else
				g_pSoundManager->play(GETITEM_SOUND);
		  return;
	  }

	  if((p0->m_nObjectType == FREDIDLE_OBJECT || p0->m_nObjectType == FREDATTACK_OBJECT
		  || p0->m_nObjectType == SWAZIDLE_OBJECT || p0->m_nObjectType == SWAZATTACK_OBJECT
		  || p0->m_nObjectType == POLKIDLE_OBJECT || p0->m_nObjectType == POLKATTACK_OBJECT)
		  && (p1->m_nObjectType == ITEME_OBJECT || p1->m_nObjectType == ITEMA_OBJECT || p1->m_nObjectType == ITEML_OBJECT)){
		  p1->kill();
		  if(m_nAmmoCount[1] < 5)
			  m_nAmmoCount[1]++;
		  m_nScore += 50;
			if(m_nAmmoCount[0] > 0 && m_nAmmoCount[1] > 0 && m_nAmmoCount[2] > 0 && m_nAmmoCount[3] > 0)
				g_pSoundManager->play(SPECIALREADY_SOUND);
			else
				g_pSoundManager->play(GETITEM_SOUND);
		  return;
	  }

	  if((p0->m_nObjectType == FREDIDLE_OBJECT || p0->m_nObjectType == FREDATTACK_OBJECT
		  || p0->m_nObjectType == SWAZIDLE_OBJECT || p0->m_nObjectType == SWAZATTACK_OBJECT
		  || p0->m_nObjectType == POLKIDLE_OBJECT || p0->m_nObjectType == POLKATTACK_OBJECT)
		  && (p1->m_nObjectType == ITEMD_OBJECT || p1->m_nObjectType == ITEMZ_OBJECT || p1->m_nObjectType == ITEMK_OBJECT)){
		  p1->kill();
		  if(m_nAmmoCount[2] < 5)
			  m_nAmmoCount[2]++;
		  m_nScore += 50;
			if(m_nAmmoCount[0] > 0 && m_nAmmoCount[1] > 0 && m_nAmmoCount[2] > 0 && m_nAmmoCount[3] > 0)
				g_pSoundManager->play(SPECIALREADY_SOUND);
			else
				g_pSoundManager->play(GETITEM_SOUND);
		  return;
	  }
  } //if
} //CollisionDetection

/// Collect garbage, that is, remove dead objects from the object list.

void CObjectManager::GarbageCollect(){
	for(auto i = m_stlObjectList.begin(); i != m_stlObjectList.end(); ){
		CGameObject* p = *i;
		if(p->m_bIsDead){
			i = m_stlObjectList.erase(i);
			delete p;
		}
		else
			++i;
	} //for
} //GarbageCollect

/// Find every enemy on the screen,  and kill it

void CObjectManager::SpecialAttack(){
	for(auto i = m_stlObjectList.begin(); i != m_stlObjectList.end(); i++){
		if((*i)->m_bVulnerable){ //and is an enemy
			(*i)->m_nHealth = 0;
			(*i)->kill(); //check every object for collision with this bullet
			m_bDiedOfSpecial = TRUE;
			CreateNextIncarnation(*i);
			m_bDiedOfSpecial = FALSE;
			m_nScore += 200;
			g_pSoundManager->play(EXPLOSION_SOUND);
		} //if
	} //for
	reduceAmmoCount(0);
	reduceAmmoCount(1);
	reduceAmmoCount(2);
	g_bSpecialActivate = FALSE;
} //SpecialAttack

/// Possibly drops a random item when enemy is killed

void CObjectManager::dropChance(Vector3 p, Vector3 v){
	int dropChance;
	g_cRandom.sowseed();

	dropChance = g_cRandom.number(1, 2);
	if(dropChance == 1){
		switch(g_cRandom.number(1, 4)){
		case 1:
			switch(player->m_nObjectType){
			case FREDIDLE_OBJECT: createObject(ITEMR_OBJECT, "itemLetterR", p, v); break;
			case FREDATTACK_OBJECT: createObject(ITEMR_OBJECT, "itemLetterR", p, v); break;
			case SWAZIDLE_OBJECT: createObject(ITEMW_OBJECT, "itemLetterW", p, v); break;
			case SWAZATTACK_OBJECT: createObject(ITEMW_OBJECT, "itemLetterW", p, v); break;
			case POLKIDLE_OBJECT: createObject(ITEMO_OBJECT, "itemLetterO", p, v); break;
			case POLKATTACK_OBJECT: createObject(ITEMO_OBJECT, "itemLetterO", p, v); break;
			} break;

		case 2:
			switch(player->m_nObjectType){
			case FREDIDLE_OBJECT: createObject(ITEME_OBJECT, "itemLetterE", p, v); break;
			case FREDATTACK_OBJECT: createObject(ITEME_OBJECT, "itemLetterE", p, v); break;
			case SWAZIDLE_OBJECT: createObject(ITEMA_OBJECT, "itemLetterA", p, v); break;
			case SWAZATTACK_OBJECT: createObject(ITEMA_OBJECT, "itemLetterA", p, v); break;
			case POLKIDLE_OBJECT: createObject(ITEML_OBJECT, "itemLetterL", p, v); break;
			case POLKATTACK_OBJECT: createObject(ITEML_OBJECT, "itemLetterL", p, v); break;
			} break;

		case 3:
			switch(player->m_nObjectType){
			case FREDIDLE_OBJECT: createObject(ITEMD_OBJECT, "itemLetterD", p, v); break;
			case FREDATTACK_OBJECT: createObject(ITEMD_OBJECT, "itemLetterD", p, v); break;
			case SWAZIDLE_OBJECT: createObject(ITEMZ_OBJECT, "itemLetterZ", p, v); break;
			case SWAZATTACK_OBJECT: createObject(ITEMZ_OBJECT, "itemLetterZ", p, v); break;
			case POLKIDLE_OBJECT: createObject(ITEMK_OBJECT, "itemLetterK", p, v); break;
			case POLKATTACK_OBJECT: createObject(ITEMK_OBJECT, "itemLetterK", p, v); break;
			} break;

		case 4:
			createObject(ITEMHEART_OBJECT, "itemHealth", p, v);  break;
		} //switch
	} //if
} //dropChance

int CObjectManager::getScore(){
	return m_nScore;
} //getScore

BOOL CObjectManager::gotHit(){
	return m_bGotHit;
} //gotHit

int CObjectManager::firedShots(){
	return m_nFiredShots;
} //firedShots

int CObjectManager::hitShots(){
	return m_nHitShots;
} //hitShots

int CObjectManager::getPlayerHealth(){
	return m_nPlayerHealth;
} //getPlayerHealth

int* CObjectManager::getAmmoCount(){
	return m_nAmmoCount;
} //getAmmoCount

void CObjectManager::reduceAmmoCount(int j){
	if(m_nAmmoCount[j] > 0)
		m_nAmmoCount[j] -= 1;
} //reduceAmmoCount

int CObjectManager::getPlayerLives(){
	return m_nPlayerLives;
} //getPlayerLives

void CObjectManager::setPlayer(int n, float i){
	m_nPlayerHealth = n;
	m_nPlayerSpeed = i;
} //setPlayer

float CObjectManager::getPlayerSpeed(){
	return m_nPlayerSpeed;
} //getPlayerSpeed

void CObjectManager::DeleteNameToObject(){
	auto i = GetPlayerObject();
	if(i != m_stlNameToObject.end()){
		m_stlNameToObject.erase(i); //erase it
	}
} //DeleteNameToObject

/// Reset player stats to initial conditions at beginning of level

void CObjectManager::ResetPlayerStats(){
	if(g_nLevelState != NONE_STATE && (g_nGameState == GAMEOVER_GAMESTATE || g_nGameState == CHARSELECT_GAMESTATE)){
		m_nPlayerLives = 2;
		m_nAmmoCount[0] = 0;
		m_nAmmoCount[1] = 0;
		m_nAmmoCount[2] = 0;
	}
	
	m_nScore = 0;
	m_bPlayerHit = FALSE;
	m_bGotHit = FALSE;
	m_bCollided = FALSE;
	m_bDiedOfAge = FALSE;
	m_bDiedOfDmg = FALSE;
	m_bDiedOfSpecial = FALSE;
	m_nFiredShots = 0;
	m_nHitShots = 0;
	m_nEnemyCount = 0;
} //ResetPlayerStats
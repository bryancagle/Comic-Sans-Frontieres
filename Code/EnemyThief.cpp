/// \file EnemyInvader.cpp
/// \brief Code for the enemyOne intelligent object class CEnemyInvaderObject.

#include <string>
#include <unordered_map>

#include "EnemyThief.h"
#include "timer.h" 
#include "random.h"
#include "debug.h"
#include "Defines.h"
#include "Sound.h"
#include "ObjMan.h"
#include "Object.h"

extern CTimer g_cTimer;  //game timer
extern CRandom g_cRandom; //random number generator
extern CSoundManager* g_pSoundManager; //sound manager
extern CObjectManager g_cObjectManager;

const int CLOSE_DISTANCE = 5000; ///< Distance for close to player. Probably just need this for attacking state
const int FAR_DISTANCE = 400; ///< Distance for "far from player" probably unused

/// Constructor for EnemyThief.
/// \param name Object name string.
/// \param location Vector location in world space.
/// \param velocity Velocity vector.

CEnemyThiefObject::CEnemyThiefObject(const char* name, const Vector3& location, const Vector3& velocity) :
	CIntelligentObject(ENEMYTHIEFIDLE_OBJECT, name, Vector3(location.x, location.y, location.z), velocity){
	m_nAttackDelayTime = m_nLastAttackTime = 0;
	m_nLastMoveTime = m_nMoveDelayTime = 0;
	m_eState = MOVING_STATE;
	m_nLastAiTime = 0; m_nAiDelayTime = 0; m_nLastSpawnTime = 0;
	m_bAttacked = FALSE;
} //constructor

/// Intelligent move function.
/// Just move like a dumb object then modify your private variables "intelligently".

void CEnemyThiefObject::move(){
	CGameObject::move(); //move like a dumb object
	think(); //act intelligently
} //move

/// Main enemyOne AI function.
/// The real work is done by a function for each state. Call the appropriate
/// function for the current state periodically, based on a timer.

void CEnemyThiefObject::think(){
	if(g_cTimer.elapsed(m_nLastAiTime, m_nAiDelayTime)){
		CIntelligentObject::think();
		switch (m_eState) { //behavior depends on state
		case MOVING_STATE: MovingAi(); break;
		case ATTACKING_STATE: AttackingAi(); break;
		default: break;
		} //switch
	} //if
} //think

/// Set the current state.
/// Change the current state, taking whatever actions are necessary on entering
/// the new state.
/// \param state New state

void CEnemyThiefObject::SetState(EnemyStateType state){
	m_eState = state; //change state

	switch (m_eState) { //change behavior settings.
	case MOVING_STATE:
		m_nAiDelayTime = 2000 + g_cRandom.number(0, 1000);
		break;

	case ATTACKING_STATE:
		m_nAiDelayTime = 1700;
		m_nAttackDelayTime = 2000 + g_cRandom.number(0, 1000);
		break;

	default: break;
	} //switch
} //SetState

/// AI for enemyOne in the moving state.
/// These enemyOne's are just moving along, periodically looking for the player.

void CEnemyThiefObject::MovingAi(){
	if(m_fDistance < CLOSE_DISTANCE && !m_bAttacked){
		SetState(ATTACKING_STATE);
		m_bAttacked = TRUE;
	}
} //MovingAi

/// AI for thief in attacking state. It shoots a projectile at the player

void CEnemyThiefObject::AttackingAi(){
	if(g_cTimer.elapsed(m_nLastAttackTime, m_nAttackDelayTime)){
		kill();
		g_cObjectManager.createObject(ENEMYTHIEFATTACK_OBJECT, "enemyThiefAttack", m_vPos, Vector3(0.0f, 0.0f, 0.0f));

		//enter the number of pixels from center of enemy to where it shoots
		const float fGunDx1 = 0;
		const float fGunDy1 = 0;

		//attack player. Shoot towards player's current position
		m_nAttackOrientation = atan2f(m_fYDistance, m_fXDistance);
		float angleDeg = m_nAttackOrientation * 180.0f / XM_PI;
		m_nAttackOrientation = angleDeg * XM_PI / 180.0f;
		const float fAngle = m_nAttackOrientation;
		const float fSine = sin(fAngle);
		const float fCosine = cos(fAngle);
		//initial bullet position
		Vector3 s;
		s = m_vPos + Vector3(fGunDx1, fGunDy1, 0);

		//velocity of bullet
		const float BULLETSPEED = 7.0f;
		const Vector3 v = BULLETSPEED * Vector3(-fCosine, -fSine, 374);

		g_cObjectManager.createObject(PROJECTILETHIEF_OBJECT, "projectileThief", s, v); //create bullet
		g_cObjectManager.GarbageCollect();
	} //if
	SetState(MOVING_STATE);
} //AttackingAi
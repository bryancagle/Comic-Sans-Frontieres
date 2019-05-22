/// \file EnemyOne.cpp
/// \brief Code for the enemyOne intelligent object class CEnemyOneObject.

#include <string>
#include <unordered_map>

#include "enemyOne.h"
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


const int CLOSE_DISTANCE = 100; ///< Distance for close to player. Probably just need this for attacking state
const int FAR_DISTANCE = 400; ///< Distance for "far from player" - unused

/// Constructor for enemyOne.
/// \param name Object name string.
/// \param location Vector location in world space.
/// \param velocity Velocity vector.

CEnemyOneObject::CEnemyOneObject(const char* name, const Vector3& location, const Vector3& velocity) :
	CIntelligentObject(ENEMY1IDLE_OBJECT, name, Vector3(location.x, location.y, location.z), velocity) {
	m_nAttackDelayTime = m_nLastAttackTime = 0;
	m_nLastMoveTime = m_nMoveDelayTime = 0;
	m_eState = MOVING_STATE;
	m_nLastAiTime = 0; m_nAiDelayTime = 0; m_nLastSpawnTime = 0;
	m_bAttacked = FALSE;
} //constructor

	/// Intelligent move function.
	/// Just move like a dumb object then modify your private variables "intelligently".

void CEnemyOneObject::move() {
	CGameObject::move(); //move like a dumb object
	think(); //act intelligently
} //move

	/// Main enemyOne AI function.
	/// The real work is done by a function for each state. Call the appropriate
	/// function for the current state periodically, based on a timer.

void CEnemyOneObject::think() {
	if (g_cTimer.elapsed(m_nLastAiTime, m_nAiDelayTime)) {
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

void CEnemyOneObject::SetState(EnemyStateType state) {
	m_eState = state; //change state

	switch (m_eState) { //change behavior settings.
	case MOVING_STATE:
		m_nAiDelayTime = 0 + g_cRandom.number(0, 100);
		break;

	case ATTACKING_STATE:
		m_nAiDelayTime = 0 + g_cRandom.number(0, 100);
		m_nAttackDelayTime = 200 + g_cRandom.number(0, 100);
		break;

	default: break;
	} //switch
} //SetState

	/// AI for enemyOne in the moving state.
	/// These enemyOne's are just moving along, periodically looking for the player.

void CEnemyOneObject::MovingAi() {
	if (m_fDistance < CLOSE_DISTANCE && !m_bAttacked) {
		SetState(ATTACKING_STATE);
		m_bAttacked = TRUE;
	}
} //MovingAi

	/// AI for enemy one in attacking. It releases its spikes and creates a "naked" enemy

void CEnemyOneObject::AttackingAi() {
	for (int i = 0; i < 10; i++) {
		//attack player. Shoot towards player's current position.
		const float fAngle = m_nAttackOrientation;
		const float fSine = sin(fAngle);
		const float fCosine = cos(fAngle);

		//enter the number of pixels from center of invader to gun
		const float fGunDx = -20.0f;
		const float fGunDy = 0.0f;

		//initial bullet position
		const Vector3 s = m_vPos + Vector3(fGunDx*fCosine - fGunDy*fSine, fGunDx*fSine - fGunDy*fCosine, 0);

		//velocity of bullet
		const float BULLETSPEED = 2.5f;
		const Vector3 v = BULLETSPEED * Vector3(-fCosine, -fSine, 374);

		g_cObjectManager.createObject(PROJECTILEENEMY1_OBJECT, "projectileEnemy1", s, v); //create bullet

		m_nAttackOrientation += 36.0f * 3.14f / 180;
	}
	kill();
	g_cObjectManager.createObject(ENEMY1AFTER_OBJECT, "enemy1IdleAfter", m_vPos, m_vVelocity);
	g_cObjectManager.GarbageCollect();
	SetState(MOVING_STATE);
} //AttackingAi
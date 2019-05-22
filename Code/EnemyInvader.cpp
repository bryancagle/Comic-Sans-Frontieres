/// \file EnemyInvader.cpp
/// \brief Code for the enemyOne intelligent object class CEnemyInvaderObject.

#include <string>
#include <unordered_map>

#include "EnemyInvader.h"
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

const int CLOSE_DISTANCE = 10000; ///< Distance for close to player. Probably just need this for attacking state
const int FAR_DISTANCE = 400; ///< Distance for "far from character"

/// Constructor for EnemyInvader.
/// \param name Object name string.
/// \param location Vector location in world space.
/// \param velocity Velocity vector.

CEnemyInvaderObject::CEnemyInvaderObject(const char* name, const Vector3& location, const Vector3& velocity) :
	CIntelligentObject(ENEMYINVADERIDLE_OBJECT, name, Vector3(location.x, location.y, location.z), velocity) {
	m_nAttackDelayTime = m_nLastAttackTime = 0;
	m_nLastMoveTime = m_nMoveDelayTime = 0;
	m_eState = MOVING_STATE;
	m_nLastAiTime = 0; m_nAiDelayTime = 0; m_nLastSpawnTime = 0;
	m_bAttacked = FALSE;
} //constructor

/// Intelligent move function.
/// Just move like a dumb object then modify your private variables "intelligently".

void CEnemyInvaderObject::move() {
	CGameObject::move(); //move like a dumb object
	think(); //act intelligently
} //move

/// Main enemyOne AI function.
/// The real work is done by a function for each state. Call the appropriate
/// function for the current state periodically, based on a timer.

void CEnemyInvaderObject::think() {
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

void CEnemyInvaderObject::SetState(EnemyStateType state) {
	m_eState = state; //change state

	switch (m_eState) { //change behavior settings.
	case MOVING_STATE:
		m_nAiDelayTime = 3000 + g_cRandom.number(0, 1000);
		break;

	case ATTACKING_STATE:
		m_nAiDelayTime = 1700;// +g_cRandom.number(0, 1000);
		m_nAttackDelayTime = 3000 + g_cRandom.number(0, 2000);
		break;

	default: break;
	} //switch
} //SetState

/// AI for enemy invader in the moving state.
/// These enemy invaders's are just moving along, periodically looking for the player.

void CEnemyInvaderObject::MovingAi() {
	if (m_fDistance < CLOSE_DISTANCE && !m_bAttacked) {
		SetState(ATTACKING_STATE);
		m_bAttacked = TRUE;
	}
} //MovingAi

/// invader AI for attacking state. It points at the player and shoots at them

void CEnemyInvaderObject::AttackingAi() {
	if (g_cTimer.elapsed(m_nLastAttackTime, m_nAttackDelayTime)) {
		kill();
		g_cObjectManager.createObject(ENEMYINVADERATTACK_OBJECT, "enemyInvaderAttack", m_vPos, m_vVelocity);

		const float fGunDx1 = 0;
		const float fGunDy1 = 0;

		const float fGunDx2 = 0;
		const float fGunDy2 = 0;

		const float fGunDx3 = 0;
		const float fGunDy3 = 0;

		for (int i = 0; i < 3; i++) {
			//attack player. Shoot towards player's current position
			m_nAttackOrientation = atan2f(m_fYDistance, m_fXDistance);
			float angleDeg = m_nAttackOrientation * 180.0f / XM_PI;
			m_nAttackOrientation = angleDeg * XM_PI / 180.0f;
			const float fAngle = m_nAttackOrientation - (.51) + (.51 * i);
			const float fSine = sin(fAngle);
			const float fCosine = cos(fAngle);

			//initial bullet position
			Vector3 s;
			switch (i) {
			case 0:
				s = m_vPos + Vector3(fGunDx1, fGunDy1, 0);
				break;
			case 1:
				s = m_vPos + Vector3(fGunDx2, fGunDy2, 0);
				break;
			case 2:
				s = m_vPos + Vector3(fGunDx3, fGunDy3, 0);
				break;
			}

			//velocity of bullet
			const float BULLETSPEED = 4.0f;
			const Vector3 v = BULLETSPEED * Vector3(-fCosine, -fSine, 374);

			g_cObjectManager.createObject(PROJECTILEINVADER_OBJECT, "projectileInvader", s, v); //create bullet
		} //for
		g_cObjectManager.GarbageCollect();
	} //if
	SetState(MOVING_STATE);
} //AttackingAi
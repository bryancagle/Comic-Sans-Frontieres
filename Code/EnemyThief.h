/// \file EnemyThief.h
/// \brief Instance for an enemy intelligent Object class CEnemyThiefObject.

#pragma once

/// AI state for enemy thief. Maybe use BOOL instead depending on if we want it moving the whole time.
/// So that we could set moving = TRUE and also have attacking = TRUE without interrupting moving and don't have to re-use code.

#include "ai.h"
#include "Defines.h"

/// \brief EnemyOne intelligent object.
///
/// AI for the enemy thief objects. As with all enemies in this game, they are on a set path and
/// do not change the way they move. They are in a moving state the entire time.
/// Once the player is close enough to enemy thief, the enemy fires at the player.

class CEnemyThiefObject: public CIntelligentObject{
	friend class CIntelligentObject;
	friend class CObject;
private:
	EnemyStateType m_eState; ///< Current state.

	int m_nLastSpawnTime;

	int m_nLastAiTime; ///< Last time AI was used.
	int m_nAiDelayTime; ///< Time until AI next used.
	int m_nLastMoveTime;
	int m_nMoveDelayTime;
	int m_nLastAttackTime;
	int m_nAttackDelayTime;

	BOOL m_bAttacked;

	void think(); ///< Artificial intelligence.
	void MovingAi(); ///< Ai for moving along.
	void AttackingAi(); ///< Ai for attacking player.
	void SetState(EnemyStateType state); ///< Change state.

public:
	CEnemyThiefObject(const char* name, const Vector3& location, const Vector3& velocity); ///< Constructor.
	void move(); ///< Move depending on time and speed.
}; //CEnemyOneObject
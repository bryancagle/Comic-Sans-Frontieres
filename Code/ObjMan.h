/// \file objman.h
/// \brief Interface for the object manager class CObjectManager.

#pragma once

#include <list>
#include <string>
#include <unordered_map>

#include "object.h"
#include "Defines.h"

/// \brief The object manager. 
///
/// The object manager is responsible for the care and feeding of
/// game objects. Objects can be named on creation so that they
/// can be accessed later - this is needed in particular for the player
/// object or objects.

class CObjectManager{
  private:
    list<CGameObject*> m_stlObjectList; ///< List of game objects.
    unordered_map<string, CGameObject*> m_stlNameToObject; ///< Map names to objects.
    unordered_map<string, ObjectType> m_stlNameToObjectType; ///< Map names to object types.
    
    int m_nLastGunFireTime; ///< Time gun was last fired.
    int m_nStartInvulnerableTime; ///< Time player started invulnerability.
		
		int m_nScore; ///< Level score.
		BOOL m_bPlayerHit; ///< Used for creating a player's hurt sprite when hit.
		BOOL m_bGotHit; ///< Used for special bonus.
		BOOL m_bCollided; ///< Makes sure that collision isn't tested on the player after it already has been hit by something.
		BOOL m_bDiedOfAge; ///< If the sprite's lifetime runs out.
		BOOL m_bDiedOfDmg; ///< If sprite is killed.
		BOOL m_bDiedOfSpecial; ///< If sprite died from player special attack.
		int m_nFiredShots; ///< Number of shots fired, used for accuracy calculation.
		int m_nHitShots; ///< Number of shots hit, used for accuracy calculation.

		int m_nPlayerHealth; ///< Player's health.
		float m_nPlayerSpeed; ///< Player's movement speed.
		int m_nAmmoCount[3]; ///< Ammo count for letters 2,3,4 respectively.
		int m_nPlayerLives; ///< Player's life count.

    //distance functions
    float distance(CGameObject *g0, CGameObject *g1); ///< Distance between objects.

    //collision detection
    void CollisionDetection(); ///< Process all collisions.
    void CollisionDetection(CGameObject* i); ///< Process collisions of all with one object.
    void CollisionDetection(CGameObject* i, CGameObject* j); ///< Process collisions of 2 objects.

    //managing dead objects
    void cull(); ///< Cull dead objects
    void CreateNextIncarnation(CGameObject* object); ///< Replace object by next in series.

  public:
    CObjectManager(); ///< Constructor.
    ~CObjectManager(); ///< Destructor.
		void GarbageCollect(); ///< Collect dead objects from object list.
		CGameObject * createObject(ObjectType obj, const char * name, const Vector3 & s, const Vector3 & v);
		CGameObject * createObject(ObjectType obj, const char * name, const Vector3 & s, const Vector3 & v, int health);
		///< Create new object.
		
		void clear(); ///< Reset to initial conditions.
		void move(); ///< Move all objects.
    void draw(); ///< Draw all objects.

    CGameObject* GetObjectByName(const char* name); ///< Get pointer to object by name.
    void InsertObjectType(const char* objname, ObjectType t); ///< Map name string to object type enumeration.
    ObjectType GetObjectType(const char* name); ///< Get object type corresponding to name string.
		unordered_map<string, CGameObject*>::iterator GetPlayerObject(); ///< Get iterator to player object.
		CGameObject* GetPlayerObjectPtr(); ///< Get pointer to player object.
    
    void FireGun(); ///< Fire a gun from named object.
		void FirePierce(); ///< Fire piercing bullet, goes through every enemy.
		void CreateShield(); ///< Protect player with a shield for one hit.
		void CreateAssist(); ///< Create a small version of the player for double shots.
		void SpecialAttack(); ///< Unleash special attack.

		void dropChance(Vector3 p, Vector3 v);
		int getScore();
		BOOL gotHit();
		int firedShots();
		int hitShots();
		int getPlayerHealth();
		int* getAmmoCount();
		void reduceAmmoCount(int j);
		int getPlayerLives();
		void setPlayer(int n, float i);
		float getPlayerSpeed();
		void DeleteNameToObject();
		void ResetPlayerStats();
}; //CObjectManager
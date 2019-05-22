/// \file ai.cpp
/// \brief Code for the intelligent object class CIntelligentObject.

#include "ai.h"
#include "debug.h"
#include "ObjMan.h"
#include "EnemyOne.h"
#include "Timer.h"
#include "Random.h"

extern CObjectManager g_cObjectManager; //object manager
extern CTimer g_cTimer;
extern CRandom g_cRandom;
extern int g_nScreenWidth;

/// Constructor for intelligent object.
/// \param object Object type.
/// \param name Object name string.
/// \param location Vector location in world space.
/// \param velocity Velocity vector.

CIntelligentObject::CIntelligentObject(ObjectType object, const char* name,
  const Vector3& location, const Vector3& velocity):
CGameObject(object, name, location, velocity){ //constructor
  m_bIntelligent = TRUE;
  m_fDistance = m_fXDistance = m_fYDistance = 0.0f;
  m_vPlaneLoc.x = m_vPlaneLoc.y = 0.0f;
} //constructor

/// Compute the distance to the player. Intelligent objects need to make
/// decisions based on how close the player is.

void CIntelligentObject::think(){
  //look for player
  CGameObject* pPlane = g_cObjectManager.GetPlayerObjectPtr();
  m_vPlaneLoc = pPlane->m_vPos; //remember player location

  //Euclidean and axial distances from player

  m_fYDistance = m_vPos.y - m_vPlaneLoc.y; //vertical distance

  //horizontal distance
  m_fXDistance = m_vPos.x - m_vPlaneLoc.x;
  //wrap horizontal distance to half of world width in magnitude
  const int nWorldWidth = 2 * g_nScreenWidth; //should be at least 2 times screen width
  if(m_fXDistance > nWorldWidth/2)
    m_fXDistance -= nWorldWidth;
  if(m_fXDistance < -nWorldWidth/2)
    m_fXDistance += nWorldWidth;

  //Euclidean distance
  m_fDistance = sqrt(m_fXDistance*m_fXDistance + m_fYDistance*m_fYDistance);
} //think
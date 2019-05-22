/// \file ai.h
/// \brief Interface for the intelligent object class CIntelligentObject.

#pragma once

#include "object.h"

/// \brief The intelligent object class.
///
/// This is the base class for our artificial intelligence. It maintains
/// the functionality that all intelligent objects have in common, that is,
/// knowledge of the player's location.

class CIntelligentObject: public CGameObject{
	friend class CEnemyOneObject;
  protected:
    Vector3 m_vPlaneLoc; ///< Players location.
    float m_fDistance; ///< Euclidean distance to player.
    float m_fYDistance; ///< Vertical distance to player.
    float m_fXDistance; ///< horizontal distance to player.

  public:
    CIntelligentObject(ObjectType object, const char* name, const Vector3& location,
      const Vector3& velocity); ///< Constructor.
    virtual void think(); ///< AI function.
}; //CIntelligentObject
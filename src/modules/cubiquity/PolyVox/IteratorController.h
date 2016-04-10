#pragma once

#include "Region.h"

namespace PolyVox {

/// Unfinished class/feature, not appropriate for end user at the moment.
template<typename IteratorType>
class IteratorController {
public:
	void reset(void);
	bool moveForward(void);

public:
	Region m_regValid;
	IteratorType* m_Iter;
};

template<typename IteratorType>
void IteratorController<IteratorType>::reset(void) {
	m_Iter->setPosition(m_regValid.getLowerCorner());
}

template<typename IteratorType>
bool IteratorController<IteratorType>::moveForward(void) {
	Vector3DInt32 v3dInitialPosition(m_Iter->getPosition().getX(), m_Iter->getPosition().getY(), m_Iter->getPosition().getZ());

	if (v3dInitialPosition.getX() < m_regValid.getUpperX()) {
		m_Iter->movePositiveX();
		return true;
	}

	v3dInitialPosition.setX(m_regValid.getLowerX());

	if (v3dInitialPosition.getY() < m_regValid.getUpperY()) {
		v3dInitialPosition.setY(v3dInitialPosition.getY() + 1);
		m_Iter->setPosition(v3dInitialPosition);
		return true;
	}

	v3dInitialPosition.setY(m_regValid.getLowerY());

	if (v3dInitialPosition.getZ() < m_regValid.getUpperZ()) {
		v3dInitialPosition.setZ(v3dInitialPosition.getZ() + 1);
		m_Iter->setPosition(v3dInitialPosition);
		return true;
	}

	return false;
}

}

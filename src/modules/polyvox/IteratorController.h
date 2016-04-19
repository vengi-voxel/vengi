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
	Vector3DInt32 v3dInitialPosition(m_Iter->getPosition().x, m_Iter->getPosition().y, m_Iter->getPosition().z);

	if (v3dInitialPosition.x < m_regValid.getUpperX()) {
		m_Iter->movePositiveX();
		return true;
	}

	v3dInitialPosition.x = m_regValid.getLowerX();

	if (v3dInitialPosition.y < m_regValid.getUpperY()) {
		v3dInitialPosition.y = v3dInitialPosition.y + 1;
		m_Iter->setPosition(v3dInitialPosition);
		return true;
	}

	v3dInitialPosition.y = m_regValid.getLowerY();

	if (v3dInitialPosition.z < m_regValid.getUpperZ()) {
		v3dInitialPosition.y = v3dInitialPosition.z + 1;
		m_Iter->setPosition(v3dInitialPosition);
		return true;
	}

	return false;
}

}

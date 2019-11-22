/**
 * @file
 */

#pragma once

#include "Animation.h"
#include "Skeleton.h"
#include "Vertex.h"

namespace animation {

class AnimationEntity {
protected:
	Animation _anim = Animation::Idle;
	Vertices _vertices;
	Indices _indices;
public:
	virtual ~AnimationEntity() {}
	void setAnimation(Animation animation);
	Animation animation() const;

	/**
	 * @brief The 'static' vertices of the character mesh where you have to apply
	 * the skeleton bones on
	 * @sa skeleton()
	 */
	const Vertices& vertices() const;
	/**
	 * @brief The 'static' indices of the character mesh
	 */
	const Indices& indices() const;

	/**
	 * @brief The skeleton data for the vertices
	 * @sa vertices()
	 */
	virtual const Skeleton& skeleton() const = 0;
};

inline Animation AnimationEntity::animation() const {
	return _anim;
}

inline void AnimationEntity::setAnimation(Animation animation) {
	_anim = animation;
}

inline const Vertices& AnimationEntity::vertices() const {
	return _vertices;
}

inline const Indices& AnimationEntity::indices() const {
	return _indices;
}

}

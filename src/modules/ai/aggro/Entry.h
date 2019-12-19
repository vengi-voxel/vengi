/**
 * @file
 */
#pragma once

#include "common/CharacterId.h"

namespace ai {

enum ReductionType {
	DISABLED, RATIO, VALUE
};

/**
 * @brief One entry for the @c AggroMgr
 */
class Entry {
protected:
	float _aggro;
	float _minAggro;
	float _reduceRatioSecond;
	float _reduceValueSecond;
	ReductionType _reduceType;
	CharacterId _id;

	void reduceByRatio(float ratio);
	void reduceByValue(float value);

public:
	Entry(const CharacterId& id, float aggro = 0.0f) :
			_aggro(aggro), _minAggro(0.0f), _reduceRatioSecond(0.0f), _reduceValueSecond(0.0f), _reduceType(DISABLED), _id(id) {
	}

	Entry(const Entry &other) :
			_aggro(other._aggro), _minAggro(other._minAggro), _reduceRatioSecond(other._reduceRatioSecond), _reduceValueSecond(other._reduceValueSecond), _reduceType(
					other._reduceType), _id(other._id) {
	}

	Entry(Entry &&other) :
			_aggro(other._aggro), _minAggro(other._minAggro), _reduceRatioSecond(other._reduceRatioSecond), _reduceValueSecond(other._reduceValueSecond), _reduceType(
					other._reduceType), _id(other._id) {
	}

	float getAggro() const;
	void addAggro(float aggro);
	void setReduceByRatio(float reductionRatioPerSecond, float minimumAggro);
	void setReduceByValue(float reductionValuePerSecond);
	/**
	 * @return @c true if any reduction was done
	 */
	bool reduceByTime(int64_t millis);
	void resetAggro();

	const CharacterId& getCharacterId() const;
	bool operator <(Entry& other) const;
	Entry& operator=(const Entry& other);
};

typedef Entry* EntryPtr;

inline void Entry::addAggro(float aggro) {
	_aggro += aggro;
}

inline void Entry::setReduceByRatio(float reduceRatioSecond, float minAggro) {
	_reduceType = RATIO;
	_reduceRatioSecond = reduceRatioSecond;
	_minAggro = minAggro;
}

inline void Entry::setReduceByValue(float reduceValueSecond) {
	_reduceType = VALUE;
	_reduceValueSecond = reduceValueSecond;
}

inline bool Entry::reduceByTime(int64_t millis) {
	switch (_reduceType) {
	case RATIO: {
		const float f = static_cast<float>(millis) / 1000.0f;
		reduceByRatio(f * _reduceRatioSecond);
		return true;
	}
	case VALUE: {
		const float f = static_cast<float>(millis) / 1000.0f;
		reduceByValue(f * _reduceValueSecond);
		return true;
	}
	case DISABLED:
		break;
	}
	return false;
}

inline void Entry::reduceByRatio(float ratio) {
	_aggro *= (1.0f - ratio);
	if (_aggro < _minAggro) {
		_aggro = 0.0f;
	}
}

inline void Entry::reduceByValue(float value) {
	_aggro -= value;

	if (_aggro < 0.000001f) {
		_aggro = 0.0f;
	}
}

inline float Entry::getAggro() const {
	return _aggro;
}

inline void Entry::resetAggro() {
	_aggro = 0.0f;
}

inline bool Entry::operator <(Entry& other) const {
	return _aggro < other._aggro;
}

inline Entry& Entry::operator=(const Entry& other) {
	_aggro = other._aggro;
	_minAggro = other._minAggro;
	_reduceRatioSecond = other._reduceRatioSecond;
	_reduceValueSecond = other._reduceValueSecond;
	_reduceType = other._reduceType;
	_id = other._id;
	return *this;
}

inline const CharacterId& Entry::getCharacterId() const {
	return _id;
}

}

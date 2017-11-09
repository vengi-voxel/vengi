/**
 * @file
 */

#include "Cooldown.h"

namespace cooldown {

Cooldown::Cooldown(Type type, unsigned long durationMillis,
		const CooldownCallback& callback, const core::TimeProviderPtr& timeProvider,
		unsigned long startMillis, unsigned long expireMillis) :
		_type(type), _durationMillis(durationMillis), _startMillis(startMillis), _expireMillis(
				expireMillis), _timeProvider(timeProvider), _callback(callback) {
}

void Cooldown::start() {
	_startMillis = _timeProvider->tickMillis();
	_expireMillis = _startMillis + _durationMillis;
	if (_callback) {
		_callback(CallbackType::Started);
	}
}

void Cooldown::reset() {
	_startMillis = 0ul;
	_expireMillis = 0ul;
}

void Cooldown::expire() {
	reset();
	if (_callback) {
		_callback(CallbackType::Expired);
	}
}

void Cooldown::cancel() {
	reset();
	if (_callback) {
		_callback(CallbackType::Canceled);
	}
}

unsigned long Cooldown::durationMillis() const {
	return _durationMillis;
}

bool Cooldown::started() const {
	return _expireMillis > 0ul;
}

bool Cooldown::running() const {
	return _expireMillis > 0ul && _timeProvider->tickMillis() < _expireMillis;
}

unsigned long Cooldown::duration() const {
	return _expireMillis - _startMillis;
}

unsigned long Cooldown::startMillis() const {
	return _startMillis;
}

Type Cooldown::type() const {
	return _type;
}

bool Cooldown::operator<(const Cooldown& rhs) const {
	return _expireMillis < rhs._expireMillis;
}

}

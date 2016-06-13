/**
 * @file
 */

#include "Var.h"
#include "Log.h"
#include "Common.h"

namespace core {

Var::VarMap Var::_vars;
ReadWriteLock Var::_lock("Var");

MAKE_SHARED_INVIS_CTOR(Var);

VarPtr Var::get(const std::string& name, const std::string& value, unsigned int flags) {
	VarMap::iterator i;
	bool missing;
	{
		ScopedReadLock lock(_lock);
		i = _vars.find(name);
		missing = i == _vars.end();
	}
	if (missing) {
		if (value.empty() && (flags & CV_NOTCREATEEMPTY)) {
			return VarPtr();
		}
		const VarPtr& p = std::make_shared<make_shared_enabler>(name, value, flags);
		ScopedWriteLock lock(_lock);
		_vars[name] = p;
		return p;
	}
	if (flags != 0) {
		i->second->_flags = flags;
	}
	return i->second;
}

Var::Var(const std::string& name, const std::string& value, unsigned int flags) :
		_name(name), _flags(flags), _dirty(false) {
	addValueToHistory(value);
}

Var::~Var() {
}

void Var::addValueToHistory(const std::string& value) {
	Value v;
	v._value = value;
	v._intValue = string::toInt(v._value);
	v._longValue = string::toLong(v._value);
	v._floatValue = string::toFloat(v._value);
	_history.push_back(v);
}

bool Var::useHistory(uint32_t historyIndex) {
	if (historyIndex >= getHistorySize()) {
		return false;
	}

	_dirty = _history[_currentHistoryPos]._value != _history[historyIndex]._value;
	_currentHistoryPos = historyIndex;

	return true;
}

void Var::setVal(const std::string& value) {
	if (_flags & CV_READONLY) {
		Log::error("%s is write protected", _name.c_str());
		return;
	}
	_dirty = _history[_currentHistoryPos]._value != value;
	if (_dirty) {
		addValueToHistory(value);
		++_currentHistoryPos;
	}
}

}

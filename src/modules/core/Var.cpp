/**
 * @file
 */

#include "Var.h"
#include "Log.h"

namespace core {

Var::VarMap Var::_vars;
ReadWriteLock Var::_lock("Var");

VarPtr Var::get(const std::string& name, const std::string& value, unsigned int flags) {
	VarMap::const_iterator i;
	bool missing;
	{
		ScopedReadLock lock(_lock);
		i = _vars.find(name);
		missing = i == _vars.end();
	}
	if (missing) {
		if (value.empty() && (flags & CV_NOTCREATEEMPTY))
			return VarPtr();
		VarPtr p(new Var(name, value, flags));
		ScopedWriteLock lock(_lock);
		_vars[name] = p;
		return p;
	}
	return i->second;
}

Var::Var(const std::string& name, const std::string& value, unsigned int flags) :
		_name(name), _flags(flags), _value(value), _dirty(false) {
	_intValue = string::toInt(_value);
	_longValue = string::toLong(_value);
	_floatValue = string::toFloat(_value);
}

Var::~Var() {
}

void Var::setVal(const std::string& value) {
	if (_flags & CV_READONLY) {
		Log::error("%s is write protected", _name.c_str());
		return;
	}
	_dirty = _value != value;
	if (_dirty) {
		_value = value;
		_intValue = string::toInt(_value);
		_longValue = string::toLong(_value);
		_floatValue = string::toFloat(_value);
	}
}

}

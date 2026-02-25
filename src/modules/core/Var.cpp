/**
 * @file
 */

#include "Var.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "core/concurrent/Lock.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>

namespace core {

VarDef::VarDef(const core::String &defName, const core::String &defValue, int32_t defFlags):
	type(VarType::Unknown), name(defName), value(defValue), flags(defFlags), title(""), description("") {
	core_assert(!defName.empty());
}

VarDef::VarDef(const core::String &defName, const core::String &defValue, const char *defTitle,
			   const char *defDescription, int32_t defFlags)
	: type(VarType::String), name(defName), value(defValue), flags(defFlags), title(defTitle ? defTitle : ""),
	  description(defDescription ? defDescription : "") {
	core_assert(!defName.empty());
}

VarDef::VarDef(const core::String &defName, const char *defValue, const char *defTitle, const char *defDescription,
			   int32_t defFlags)
	: type(VarType::String), name(defName), value(defValue), flags(defFlags), title(defTitle ? defTitle : ""),
	  description(defDescription ? defDescription : "") {
	core_assert(!defName.empty());
}

VarDef::VarDef(const core::String &defName, bool defValue, const char *defTitle, const char *defDescription,
			   int32_t defFlags)
	: type(VarType::Boolean), name(defName), value(defValue ? "true" : "false"), flags(defFlags),
	  title(defTitle ? defTitle : ""), description(defDescription ? defDescription : "") {
	core_assert(!defName.empty());
}

VarDef::VarDef(const core::String &defName, const core::Path &path, const char *defTitle, const char *defDescription,
			   int32_t defFlags)
	: type(VarType::Path), name(defName), value(path.toString()), flags(defFlags), title(defTitle ? defTitle : ""),
	  description(defDescription ? defDescription : "") {
}

VarDef::VarDef(const core::String &defName, int defValue, const char *defTitle, const char *defDescription,
			   int32_t defFlags)
	: type(VarType::Int), name(defName), value(core::String::format("%i", defValue)), flags(defFlags),
	  title(defTitle ? defTitle : ""), description(defDescription ? defDescription : ""), minValue(0), maxValue(-1) {
	core_assert(!defName.empty());
}

VarDef::VarDef(const core::String &defName, int defValue, int defMin, int defMax, const char *defTitle,
			   const char *defDescription, int32_t defFlags)
	: type(VarType::Int), name(defName), value(core::String::format("%i", defValue)), flags(defFlags),
	  title(defTitle ? defTitle : ""), description(defDescription ? defDescription : ""), minValue(defMin),
	  maxValue(defMax) {
	core_assert(!defName.empty());
}

VarDef::VarDef(const core::String &defName, float defValue, const char *defTitle, const char *defDescription,
			   int32_t defFlags)
	: type(VarType::Float), name(defName), value(core::String::format("%f", defValue)), flags(defFlags),
	  title(defTitle ? defTitle : ""), description(defDescription ? defDescription : ""), minValue(0.0f),
	  maxValue(-1.0f) {
	core_assert(!defName.empty());
}

VarDef::VarDef(const core::String &defName, float defValue, float defMin, float defMax, const char *defTitle,
			   const char *defDescription, int32_t defFlags)
	: type(VarType::Float), name(defName), value(core::String::format("%f", defValue)), flags(defFlags),
	  title(defTitle ? defTitle : ""), description(defDescription ? defDescription : ""), minValue(defMin),
	  maxValue(defMax) {
	core_assert(!defName.empty());
}

VarDef::VarDef(const core::String &defName, const core::String &defValue,
			   const core::DynamicArray<core::String> &defValidValues, const char *defTitle, const char *defDescription,
			   int32_t defFlags)
	: type(VarType::Enum), name(defName), value(defValue), flags(defFlags), title(defTitle ? defTitle : ""),
	  description(defDescription ? defDescription : ""), validValues(defValidValues) {
	core_assert(!defName.empty());
}

bool VarDef::hasMinMax() const {
	switch (type) {
	case VarType::Int:
		return minValue.i <= maxValue.i;
	case VarType::Float:
		return minValue.f <= maxValue.f;
	default:
		return false;
	}
}

bool VarDef::hasValidValues() const {
	return !validValues.empty();
}

Var::VarMap Var::_vars;
uint8_t Var::_visitFlags = 0u;
core_trace_mutex_static(Lock, Var, _lock);

void Var::shutdown() {
	ScopedLock lock(_lock);
	_vars.clear();
}

void Var::toggleBool() {
	if (type() != VarType::Boolean) {
		return;
	}
	setVal(boolVal() ? false : true);
}

bool Var::setVal(bool value) {
	if (type() != VarType::Boolean) {
		return false;
	}
	return setVal(value ? "true" : "false");
}

bool Var::setVal(int value) {
	if (intVal() == value) {
		return true;
	}
	return setVal(core::String::format("%i", value));
}

bool Var::setVal(float value) {
	if (glm::epsilonEqual(floatVal(), value, glm::epsilon<float>())) {
		return true;
	}
	return setVal(core::String::format("%f", value));
}

VarPtr Var::registerVar(const VarDef &def) {
	return createVar(def);
}

VarPtr Var::getVar(const core::String &name) {
	const VarPtr &var = findVar(name);
	core_assert_msg(var, "var %s doesn't exist yet", name.c_str());
	return var;
}

void Var::vec3Val(float out[3]) const {
	float x, y, z;
#ifdef _MSC_VER
	if (::sscanf_s(strVal().c_str(), "%f:%f:%f", &x, &y, &z) != 3) {
		if (::sscanf_s(strVal().c_str(), "%f %f %f", &x, &y, &z) != 3) {
#else
	if (::sscanf(strVal().c_str(), "%f:%f:%f", &x, &y, &z) != 3) {
		if (::sscanf(strVal().c_str(), "%f %f %f", &x, &y, &z) != 3) {
#endif
			out[0] = out[1] = out[2] = 0.0f;
			return;
		}
	}
	out[0] = x;
	out[1] = y;
	out[2] = z;
}

VarPtr Var::findVar(const core::String& name) {
	ScopedLock lock(_lock);
	VarMap::iterator i = _vars.find(name);
	if (i == _vars.end()) {
		return VarPtr();
	}
	return i->second;
}

VarPtr Var::createVar(const VarDef &def) {
	VarMap::iterator i;
	bool missing;
	{
		ScopedLock lock(_lock);
		i = _vars.find(def.name);
		missing = i == _vars.end();
	}

	const char *value = def.value.c_str();
	uint32_t flagsMask = def.flags < 0 ? 0u : static_cast<uint32_t>(def.flags);
	if (missing) {
		// environment variables have higher priority than config file values - but command line
		// arguments have the highest priority
		if ((flagsMask & CV_FROMCOMMANDLINE) == 0) {
			const char *envValue = SDL_getenv(def.name.c_str());
			if (envValue == nullptr || envValue[0] == '\0') {
				const core::String &upper = def.name.toUpper();
				envValue = SDL_getenv(upper.c_str());
			}
			if (envValue != nullptr && envValue[0] != '\0') {
				value = envValue;
				flagsMask |= CV_FROMENV;
				flagsMask &= ~CV_FROMFILE;
			}
		}

		if (value == nullptr) {
			return VarPtr();
		}

		const VarPtr &p = core::make_shared<Var>(def, value, flagsMask);
		ScopedLock lock(_lock);
		_vars.put(def.name, p);
		return p;
	}
	const VarPtr &v = i->second;
	if (def.flags >= 0) {
		if ((flagsMask & CV_FROMFILE) == CV_FROMFILE && (v->_flags & (CV_FROMCOMMANDLINE | CV_FROMENV)) == 0u) {
			Log::debug("Look for env var to resolve value of %s", def.name.c_str());
			// environment variables have higher priority than config file values
			const char *envValue = SDL_getenv(def.name.c_str());
			if (envValue == nullptr || envValue[0] == '\0') {
				const core::String &upper = def.name.toUpper();
				envValue = SDL_getenv(upper.c_str());
			}
			if (envValue != nullptr && envValue[0] != '\0') {
				value = envValue;
			}
			v->setVal(value);
		} else if ((flagsMask & CV_FROMCOMMANDLINE) == CV_FROMCOMMANDLINE) {
			// in case it was already created, make sure that the command line value is set again
			// this might happen in cases, where multiple -set parameters were specified
			v->setVal(value);
		}

		// ensure that the commandline and env options are kept
		if ((v->_flags & CV_FROMCOMMANDLINE) == CV_FROMCOMMANDLINE) {
			flagsMask |= CV_FROMCOMMANDLINE;
		} else if ((v->_flags & CV_FROMENV) == CV_FROMENV) {
			flagsMask |= CV_FROMENV;
		}

		// some flags should not get removed
		const uint32_t preserve = v->_flags & CV_PRESERVE;
		v->_flags = flagsMask | preserve;
	}
	if (def.hasMinMax()) {
		v->_def.minValue = def.minValue;
		v->_def.maxValue = def.maxValue;
	}
	if (v->_def.type == VarType::Unknown) {
		v->_def.type = def.type;
	}
	if (v->_def.validValues.empty() && def.hasValidValues()) {
		v->_def.validValues = def.validValues;
	}
	if (v->_def.title.empty() && !def.title.empty()) {
		v->_def.title = def.title;
	}
	if (v->_def.description.empty() && !def.description.empty()) {
		v->_def.description = def.description;
	}
	return v;
}

Var::Var(VarDef def, const core::String &currentValue, uint32_t flags) :
		_def(core::move(def)), _flags(flags) {
	addValueToHistory(currentValue);
	core_assert(_currentHistoryPos == 0);
	if (_def.value.empty()) {
		_def.value = currentValue;
	}
}

Var::~Var() {
}

void Var::addValueToHistory(const core::String &value) {
	Value v;
	v._value = value;
	const bool isTrue = v._value == "true";
	v._intValue = isTrue ? 1 : string::toInt(v._value);
	v._floatValue = isTrue ? 1.0f : string::toFloat(v._value);
	_history.push_back(v);
	Log::debug("new value for %s is %s", _def.name.c_str(), value.c_str());
}

bool Var::useHistory(uint32_t historyIndex) {
	if (historyIndex >= getHistorySize()) {
		return false;
	}

	_dirty = _history[_currentHistoryPos]._value != _history[historyIndex]._value;
	_currentHistoryPos = historyIndex;

	if (_dirty) {
		if ((_flags & CV_SHADER) != 0u) {
			_visitFlags |= NEEDS_SHADERUPDATE;
		}
		if ((_flags & (CV_NOPERSIST | CV_READONLY)) == 0u) {
			_visitFlags |= NEEDS_SAVING;
		}
	}

	return true;
}

bool Var::setVal(const char* value) {
	if (!SDL_strcmp(strVal().c_str(), value)) {
		return true;
	}
	return setVal(core::String(value));
}

bool Var::setVal(const core::String &value) {
	if ((_flags & CV_READONLY) != 0u) {
		Log::error("%s is write protected", _def.name.c_str());
		return false;
	}
	if (!validate(value)) {
		Log::error("Validation failed for %s with value %s", _def.name.c_str(), value.c_str());
		return false;
	}
	_dirty = _history[_currentHistoryPos]._value != value;
	if (_dirty) {
		addValueToHistory(value);
		++_currentHistoryPos;
		if ((_flags & CV_SHADER) != 0u) {
			_visitFlags |= NEEDS_SHADERUPDATE;
		}
		if ((_flags & (CV_NOPERSIST | CV_READONLY)) == 0u) {
			_visitFlags |= NEEDS_SAVING;
		}
		if (_history.size() >= _history.increase()) {
			_history.erase(0, _history.increase() / 2);
			_currentHistoryPos = (uint32_t)_history.size() - 1;
		}
	}
	return true;
}

bool Var::validate(const core::String &value) const {
	if (_def.type == VarType::Boolean) {
		if (value != "1" && value != "true" && value != "false" && value != "0") {
			Log::debug("Validator doesn't allow to set '%s' for '%s'", value.c_str(), _def.name.c_str());
			return false;
		}
	}
	if (_def.hasMinMax() && !value.empty()) {
		if (_def.type == VarType::Int) {
			const int iv = string::toInt(value);
			if (iv < _def.minValue.i || iv > _def.maxValue.i) {
				Log::debug("Value %s is out of range [%i, %i] for '%s'", value.c_str(), _def.minValue.i, _def.maxValue.i, _def.name.c_str());
				return false;
			}
		} else {
			const float fv = string::toFloat(value);
			if (fv < _def.minValue.f || fv > _def.maxValue.f) {
				Log::debug("Value %s is out of range [%f, %f] for '%s'", value.c_str(), _def.minValue.f, _def.maxValue.f, _def.name.c_str());
				return false;
			}
		}
	}
	if (_def.hasValidValues() && !value.empty()) {
		bool found = false;
		for (const core::String &valid : _def.validValues) {
			if (valid == value) {
				found = true;
				break;
			}
		}
		if (!found) {
			Log::debug("Value '%s' is not a valid value for '%s'", value.c_str(), _def.name.c_str());
			return false;
		}
	}
	return true;
}

} // namespace core

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

Var::VarMap Var::_vars;
uint8_t Var::_visitFlags = 0u;
core_trace_mutex_static(Lock, Var, _lock);

void Var::shutdown() {
	ScopedLock lock(_lock);
	_vars.clear();
}

void Var::toggleBool() {
	if (typeIsBool()) {
		setVal(boolVal() ? false : true);
	}
}

bool Var::setVal(bool value) {
	if (boolVal() == value) {
		return true;
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

VarPtr Var::getVar(const core::String &name) {
	const VarPtr &var = findVar(name);
	core_assert_msg(var, "var %s doesn't exist yet", name.c_str());
	return var;
}

bool Var::_ivec3ListValidator(const core::String &value, int nmin, int nmax) {
	core::DynamicArray<core::String> regionSizes;
	core::string::splitString(value, regionSizes, ",");
	for (const core::String &s : regionSizes) {
		glm::ivec3 maxs;
		core::string::parseIVec3(s, &maxs[0]);
		for (int i = 0; i < 3; ++i) {
			if (maxs[i] < nmin || maxs[i] > nmax) {
				return false;
			}
		}
	}
	return true;
}

bool Var::_minMaxValidator(const core::String &value, int nmin, int nmax) {
	const int v = value.toInt();
	if (v < nmin || v > nmax) {
		return false;
	}
	return true;
}

core::String Var::str(const core::String &name) {
	const VarPtr &var = findVar(name);
	if (!var) {
		return core::String::Empty;
	}
	return var->strVal();
}

bool Var::boolean(const core::String &name) {
	const VarPtr &var = findVar(name);
	if (!var) {
		return false;
	}
	return var->boolVal();
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
	core_assert(!def.name.empty());
	VarMap::iterator i;
	const char *defaultValue = def.value.c_str();
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
				if (!def.validatorFunc || def.validatorFunc(envValue)) {
					value = envValue;
				}
				flagsMask |= CV_FROMENV;
				flagsMask &= ~CV_FROMFILE;
			}
		}

		if (value == nullptr) {
			return VarPtr();
		}

		const VarPtr &p = core::make_shared<Var>(def.name, value, defaultValue == nullptr ? "" : defaultValue, flagsMask,
												 def.help, def.validatorFunc);
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
				if (!def.validatorFunc || def.validatorFunc(envValue)) {
					value = envValue;
				}
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
	if (def.validatorFunc != nullptr) {
		v->_validator = def.validatorFunc;
	}
	if (!v->_help) {
		v->_help = def.help;
	}
	return v;
}

Var::Var(const core::String& name, const core::String& value, const core::String &defaultValue, unsigned int flags, const char *help, ValidatorFunc validatorFunc) :
		_name(name), _help(help), _flags(flags), _defaultValue(defaultValue), _validator(validatorFunc) {
	addValueToHistory(value);
	core_assert(_currentHistoryPos == 0);
	if (_defaultValue.empty()) {
		_defaultValue = value;
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
	Log::debug("new value for %s is %s", _name.c_str(), value.c_str());
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

bool Var::setVal(const core::String &value) {
	if ((_flags & CV_READONLY) != 0u) {
		Log::error("%s is write protected", _name.c_str());
		return false;
	}
	if (_validator != nullptr) {
		if (!_validator(value)) {
			Log::debug("Validator doesn't allow to set '%s' for '%s'", value.c_str(), _name.c_str());
			return false;
		}
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

} // namespace core

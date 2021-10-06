/**
 * @file
 */

#include "Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "core/Assert.h"
#include "core/StringUtil.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>

namespace core {

const core::String VAR_TRUE("true");
const core::String VAR_FALSE("false");

Var::VarMap Var::_vars;
ReadWriteLock Var::_lock("Var");
uint8_t Var::_visitFlags = 0u;

VarPtr Var::get(const core::String& name, int value, int32_t flags) {
	char buf[64];
	core::string::formatBuf(buf, sizeof(buf), "%i", value);
	return get(name, buf, flags);
}

void Var::shutdown() {
	ScopedWriteLock lock(_lock);
	_vars.clear();
}

bool Var::setVal(int value) {
	if (intVal() == value) {
		return true;
	}
	return setVal(core::string::format("%i", value));
}

bool Var::setVal(float value) {
	if (glm::epsilonEqual(floatVal(), value, glm::epsilon<float>())) {
		return true;
	}
	return setVal(core::string::format("%f", value));
}

VarPtr Var::getSafe(const core::String& name) {
	const VarPtr& var = get(name);
	core_assert_msg(var, "var %s doesn't exist yet", name.c_str());
	return var;
}

core::String Var::str(const core::String& name) {
	const VarPtr& var = get(name);
	if (!var) {
		return "";
	}
	return var->strVal();
}

bool Var::boolean(const core::String& name) {
	const VarPtr& var = get(name);
	if (!var) {
		return false;
	}
	return var->boolVal();
}

glm::vec3 Var::vec3Val() const {
	float x, y, z;
#ifdef _MSC_VER
	if (::sscanf_s(strVal().c_str(), "%f:%f:%f", &x, &y, &z) != 3) {
		if (::sscanf_s(strVal().c_str(), "%f %f %f", &x, &y, &z) != 3) {
#else
	if (::sscanf(strVal().c_str(), "%f:%f:%f", &x, &y, &z) != 3) {
		if (::sscanf(strVal().c_str(), "%f %f %f", &x, &y, &z) != 3) {
#endif
			return glm::zero<glm::vec3>();
		}
	}
	return glm::vec3(x, y, z);
}

VarPtr Var::get(const core::String& name, const char* value, int32_t flags, const char *help, ValidatorFunc validatorFunc) {
	VarMap::iterator i;
	bool missing;
	{
		ScopedReadLock lock(_lock);
		i = _vars.find(name);
		missing = i == _vars.end();
	}

	uint32_t flagsMask = flags < 0 ? 0u : static_cast<uint32_t>(flags);
	if (missing) {
		// environment variables have higher priority than config file values - but command line
		// arguments have the highest priority
		if ((flagsMask & CV_FROMCOMMANDLINE) == 0) {
			const char* envValue = SDL_getenv(name.c_str());
			if (envValue == nullptr || envValue[0] == '\0') {
				const core::String& upper = name.toUpper();
				envValue = SDL_getenv(upper.c_str());
			}
			if (envValue != nullptr && envValue[0] != '\0') {
				if (!validatorFunc || validatorFunc(envValue)) {
					value = envValue;
				}
				flagsMask |= CV_FROMENV;
				flagsMask &= ~CV_FROMFILE;
			}
		}

		if (value == nullptr) {
			return VarPtr();
		}

		const VarPtr& p = core::make_shared<Var>(name, value, flagsMask, help, validatorFunc);
		ScopedWriteLock lock(_lock);
		_vars.put(name, p);
		return p;
	}
	const VarPtr& v = i->second;
	if (flags >= 0) {
		if ((flagsMask & CV_FROMFILE) == CV_FROMFILE && (v->_flags & (CV_FROMCOMMANDLINE | CV_FROMENV)) == 0u) {
			Log::debug("Look for env var to resolve value of %s", name.c_str());
			// environment variables have higher priority than config file values
			const char* envValue = SDL_getenv(name.c_str());
			if (envValue == nullptr || envValue[0] == '\0') {
				const core::String& upper = name.toUpper();
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
		if (validatorFunc != nullptr) {
			v->_validator = validatorFunc;
		}
	}
	if (!v->_help) {
		v->_help = help;
	}
	return v;
}

Var::Var(const core::String& name, const core::String& value, unsigned int flags, const char *help, ValidatorFunc validatorFunc) :
		_name(name), _help(help), _flags(flags), _validator(validatorFunc) {
	addValueToHistory(value);
	core_assert(_currentHistoryPos == 0);
}

Var::~Var() {
}

void Var::addValueToHistory(const core::String& value) {
	Value v;
	v._value = value;
	const bool isTrue = v._value == VAR_TRUE;
	v._intValue = isTrue ? 1 : string::toInt(v._value);
	v._longValue = isTrue ? 1l : (long)string::toLong(v._value);
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

	return true;
}

bool Var::setVal(const core::String& value) {
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
		if ((_flags & CV_REPLICATE) != 0u) {
			_updateFlags |= NEEDS_REPLICATE;
			_visitFlags |= NEEDS_REPLICATE;
		}
		if ((_flags & CV_BROADCAST) != 0u) {
			_updateFlags |= NEEDS_BROADCAST;
			_visitFlags |= NEEDS_BROADCAST;
		}
		if ((_flags & CV_SHADER) != 0u) {
			_visitFlags |= NEEDS_SHADERUPDATE;
		}
		if (_history.size() > 16) {
			_history.erase(0, 8);
			_currentHistoryPos = (uint32_t)_history.size() - 1;
		}
	}
	return true;
}

}

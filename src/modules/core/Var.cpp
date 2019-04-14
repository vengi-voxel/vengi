/**
 * @file
 */

#include "Var.h"
#include "Log.h"
#include "Common.h"
#include "Assert.h"

namespace core {

const std::string VAR_TRUE("true");
const std::string VAR_FALSE("false");

Var::VarMap Var::_vars;
ReadWriteLock Var::_lock("Var");

MAKE_SHARED_INVIS_CTOR(Var);

void Var::shutdown() {
	ScopedWriteLock lock(_lock);
	_vars.clear();
}

VarPtr Var::getSafe(const std::string& name) {
	const VarPtr& var = get(name);
	core_assert_msg(var, "var %s doesn't exist yet", name.c_str());
	return var;
}

std::string Var::str(const std::string& name) {
	const VarPtr& var = get(name);
	if (!var) {
		return "";
	}
	return var->strVal();
}

bool Var::boolean(const std::string& name) {
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
#else
	if (::sscanf(strVal().c_str(), "%f:%f:%f", &x, &y, &z) != 3) {
#endif
		return glm::zero<glm::vec3>();
	}
	return glm::vec3(x, y, z);
}

VarPtr Var::get(const std::string& name, const char* value, int32_t flags, const char *help) {
	VarMap::iterator i;
	bool missing;
	{
		ScopedReadLock lock(_lock);
		i = _vars.find(name);
		missing = i == _vars.end();
	}

	const uint32_t flagsMask = flags < 0 ? 0u : static_cast<uint32_t>(flags);
	if (missing) {
		// environment variables have higher priority than config file values
		const char* envValue = SDL_getenv(name.c_str());
		if (envValue == nullptr) {
			const std::string& upper = string::toUpper(name);
			envValue = SDL_getenv(upper.c_str());
		}
		if (envValue != nullptr) {
			value = envValue;
		}

		if (value == nullptr) {
			return VarPtr();
		}

		const VarPtr& p = std::make_shared<make_shared_enabler>(name, value, flagsMask, help);
		ScopedWriteLock lock(_lock);
		_vars[name] = p;
		return p;
	}
	const VarPtr& v = i->second;
	if (flags >= 0) {
		if ((flagsMask & CV_FROMFILE) == CV_FROMFILE && (flagsMask & CV_FROMCOMMANDLINE) == 0u) {
			// environment variables have higher priority than config file values
			const char* envValue = SDL_getenv(name.c_str());
			if (envValue == nullptr) {
				const std::string& upper = string::toUpper(name);
				envValue = SDL_getenv(upper.c_str());
			}
			if (envValue != nullptr) {
				value = envValue;
			}
			v->setVal(value);
		}
		v->_flags = flagsMask;
	}
	return v;
}

Var::Var(const std::string& name, const std::string& value, unsigned int flags, const char *help) :
		_name(name), _help(help), _flags(flags), _dirty(false) {
	addValueToHistory(value);
}

Var::~Var() {
}

void Var::addValueToHistory(const std::string& value) {
	Value v;
	v._value = value;
	v._intValue = string::toInt(v._value);
	v._longValue = (long)string::toLong(v._value);
	v._floatValue = string::toFloat(v._value);
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

void Var::setVal(const std::string& value) {
	if ((_flags & CV_READONLY) != 0u) {
		Log::error("%s is write protected", _name.c_str());
		return;
	}
	_dirty = _history[_currentHistoryPos]._value != value;
	if (_dirty) {
		addValueToHistory(value);
		++_currentHistoryPos;
		if ((_flags & CV_REPLICATE) != 0u) {
			_updateFlags |= NEEDS_REPLICATE;
		}
		if ((_flags & CV_BROADCAST) != 0u) {
			_updateFlags |= NEEDS_BROADCAST;
		}
		if (_history.size() > 16) {
			std::vector<Value>(_history.begin() + 8, _history.end()).swap(_history);
			_currentHistoryPos = _history.size();
		}
	}
}

}

/**
 * @file
 */

#pragma once

#include "core/ConfigVar.h"
#include "core/SharedPtr.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "core/concurrent/Lock.h"
#include "io/Stream.h"
#include <string.h>

namespace core {

/**
 * @defgroup Var Var
 * @{
 */

/** @brief Variable may only be modified at application start via command line */
const uint32_t CV_READONLY = 1 << 0;
/** @brief will not get saved to the file */
const uint32_t CV_NOPERSIST = 1 << 1;
/** @brief will be put as define in every shader - a change will update the shaders at runtime */
const uint32_t CV_SHADER = 1 << 2;
/** @brief don't show the value to users, but just ***secure*** it out */
const uint32_t CV_SECRET = 1 << 5;
const uint32_t CV_PRESERVE = (CV_READONLY | CV_NOPERSIST | CV_SHADER | CV_SECRET);

const uint32_t CV_FROMFILE = 1 << 6;
const uint32_t CV_FROMCOMMANDLINE = 1 << 7;
const uint32_t CV_FROMENV = 1 << 8;

class Var;
typedef core::SharedPtr<Var> VarPtr;

/**
 * @brief A var can be changed and queried at runtime
 *
 * Create a new variable with all parameters
 * @code
 * core::VarPtr var = core::Var::get("prefix_name", "defaultvalue", 0);
 * @endcode
 *
 * If you just want to get an existing variable use:
 * @code
 * core::Var::get("prefix_name");
 * @endcode
 */
class Var {
public:
	typedef bool (*ValidatorFunc)(const core::String& value);
protected:
	friend class SharedPtr<Var>;
	typedef StringMap<VarPtr, 64> VarMap;
	static VarMap _vars;
	static Lock _lock;

	const core::String _name;
	const char* _help = nullptr;
	uint32_t _flags;
	static constexpr int NEEDS_SHADERUPDATE = 1 << 2;
	static constexpr int NEEDS_SAVING = 1 << 3;
	uint8_t _updateFlags = 0u;

	static uint8_t _visitFlags;

	struct Value {
		float _floatValue = 0.0f;
		int _intValue = 0;
		long _longValue = 0l;
		core::String _value;
	};

	core::String _defaultValue;
	core::DynamicArray<Value, 16u> _history;
	uint32_t _currentHistoryPos = 0;
	bool _dirty = false;
	ValidatorFunc _validator = nullptr;

	void addValueToHistory(const core::String& value);
	static bool _ivec3ListValidator(const core::String& value, int nmin, int nmax);
	static bool _minMaxValidator(const core::String& value, int nmin, int nmax);

	// invisible - use the static get method
	Var(const core::String& name, const core::String& value, const core::String &defaultValue, uint32_t flags, const char *help, ValidatorFunc validatorFunc);
public:
	~Var();

	void reset() {
		setVal(_defaultValue);
	}

	static size_t size() {
		ScopedLock scoped(_lock);
		return _vars.size();
	}

	static bool boolValidator(const core::String& value) {
		return value == "1" || value == "true" || value == "false" || value == "0";
	}

	template<size_t NMIN, size_t NMAX>
	static bool ivec3ListValidator(const core::String& value) {
		if (value.empty()) {
			return true;
		}
		return _ivec3ListValidator(value, NMIN, NMAX);
	}

	template<size_t NMIN, size_t NMAX>
	static bool minMaxValidator(const core::String& value) {
		if (value.empty()) {
			return true;
		}
		return _minMaxValidator(value, NMIN, NMAX);
	}

	/**
	 * @brief Creates a new or gets an already existing var
	 *
	 * @param[in] name The name that this var is registered under (must be unique)
	 * @param[in] value The initial value of the var. If this is @c nullptr a new cvar
	 * is not created by this call.
	 * @param[in] flags A bitmask of var flags - e.g. @c CV_READONLY
	 *
	 * @note This is using a read/write lock to allow access from different threads.
	 */
	static VarPtr get(const core::String& name, const char* value = nullptr, int32_t flags = -1, const char *help = nullptr, ValidatorFunc validatorFunc = nullptr);
	static VarPtr findVar(const core::String& name);

	static inline VarPtr get(const core::String& name, const char* value, const char *help, ValidatorFunc validatorFunc = nullptr) {
		return get(name, value, -1, help, validatorFunc);
	}

	static inline VarPtr get(const core::String& name, const String& value, int32_t flags = -1, const char *help = nullptr, ValidatorFunc validatorFunc = nullptr) {
		return get(name, value.c_str(), flags, help, validatorFunc);
	}

	/**
	 * @note variable must exists, otherwise @c core_assert triggers
	 */
	static VarPtr getVar(const core::String& name);

	/**
	 * @return empty string if var with given name wasn't found, otherwise the value of the var
	 */
	static core::String str(const core::String& name);

	/**
	 * The memory is now owned. Make sure it is available for the whole lifetime of this instance.
	 */
	void setHelp(const char *help);
	void setValidator(ValidatorFunc func);

	const char *help() const;

	/**
	 * @return @c false if var with given name wasn't found, otherwise the bool value of the var
	 */
	static bool boolean(const core::String& name);

	static VarPtr get(const core::String& name, int value, int32_t flags = -1, const char *help = nullptr, ValidatorFunc validatorFunc = {});

	static void shutdown();

	template<class Functor>
	static void visit(Functor&& func) {
		ScopedLock scoped(_lock);
		for (auto i = _vars.begin(); i != _vars.end(); ++i) {
			func(i->second);
		}
	}

	/**
	 * @brief Reset the flag after calling it
	 */
	static bool hasDirtyShaderVars() {
		const bool dirty = _visitFlags & NEEDS_SHADERUPDATE;
		_visitFlags &= ~NEEDS_SHADERUPDATE;
		return dirty;
	}

	static bool needsSaving() {
		const bool dirty = _visitFlags & NEEDS_SAVING;
		_visitFlags &= ~NEEDS_SAVING;
		return dirty;
	}

	void clearHistory();
	uint32_t getHistorySize() const;
	uint32_t getHistoryIndex() const;
	bool useHistory(uint32_t historyIndex);

	/**
	 * @return the bitmask of flags for this var
	 * @note See the existing @c CV_ ints
	 */
	uint32_t getFlags() const;
	/**
	 * @return the value of the variable as @c int.
	 *
	 * @note There is no conversion happening here - this is done in @c Var::setVal
	 */
	int intVal() const;
	/**
	 * @return the value of the variable as @c unsigned int.
	 *
	 * @note There is no conversion happening here - this is done in @c Var::setVal
	 */
	unsigned int uintVal() const;
	/**
	 * @return the value of the variable as @c long.
	 *
	 * @note There is no conversion happening here - this is done in @c Var::setVal
	 */
	long longVal() const;
	unsigned long ulongVal() const;
	/**
	 * @return the value of the variable as @c float.
	 *
	 * @note There is no conversion happening here - this is done in @c Var::setVal
	 */
	float floatVal() const;
	/**
	 * @return the value of the variable as @c bool. @c true if the string value is either @c 1 or @c true, @c false otherwise
	 */
	bool boolVal() const;
	void toggleBool();
	void vec3Val(float out[3]) const;
	bool setVal(const core::String& value);
	inline bool setVal(const char* value) {
		if (!SDL_strcmp(_history[_currentHistoryPos]._value.c_str(), value)) {
			return true;
		}
		return setVal(core::String(value));
	}
	bool setVal(bool value);
	bool setVal(int value);
	bool setVal(float value);
	/**
	 * @return The string value of this var
	 */
	const core::String& strVal() const;
	const core::String& name() const;
	/**
	 * @return @c true if some @c Var::setVal call changed the initial/default value that was specified on construction
	 */
	bool isDirty() const;
	void markClean();

	bool typeIsBool() const;
};

inline void Var::setValidator(Var::ValidatorFunc func) {
	_validator = func;
}

inline uint32_t Var::getHistorySize() const {
	return (uint32_t)_history.size();
}

inline uint32_t Var::getHistoryIndex() const {
	return _currentHistoryPos;
}

inline void Var::clearHistory() {
	if (_history.size() == 1u) {
		return;
	}
	_history.release();
}

inline float Var::floatVal() const {
	return _history[_currentHistoryPos]._floatValue;
}

inline int Var::intVal() const {
	return _history[_currentHistoryPos]._intValue;
}

inline long Var::longVal() const {
	return _history[_currentHistoryPos]._longValue;
}

inline unsigned long Var::ulongVal() const {
	return static_cast<unsigned long>(_history[_currentHistoryPos]._longValue);
}

inline bool Var::boolVal() const {
	return _history[_currentHistoryPos]._value == "true" || _history[_currentHistoryPos]._value == "1";
}

inline bool Var::typeIsBool() const {
	return _history[_currentHistoryPos]._value == "true" || _history[_currentHistoryPos]._value == "1" || _history[_currentHistoryPos]._value == "false" || _history[_currentHistoryPos]._value == "0";
}

inline const core::String& Var::strVal() const {
	return _history[_currentHistoryPos]._value;
}

inline const core::String& Var::name() const {
	return _name;
}

inline bool Var::isDirty() const {
	return _dirty;
}

inline void Var::markClean() {
	_dirty = false;
}

inline uint32_t Var::getFlags() const {
	return _flags;
}

inline unsigned int Var::uintVal() const {
	return static_cast<unsigned int>(_history[_currentHistoryPos]._intValue);
}

inline void Var::setHelp(const char *help) {
	_help = help;
}

inline const char *Var::help() const {
	return _help;
}

/**
 * @}
 */

}

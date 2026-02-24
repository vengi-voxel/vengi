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

const int32_t CV_NONE = -1;
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

typedef bool (*ValidatorFunc)(const core::String &value);

enum class VarType : uint8_t {
	String,
	Int,
	Float,
	Bool,
	Enum
};

union RangeValue {
	float f;
	int i;
	RangeValue() : f(0.0f) {}
	RangeValue(float v) : f(v) {}
	RangeValue(int v) : i(v) {}
};

struct VarDef {
	VarDef(const core::String &defName, const core::String &defValue, int32_t defFlags = CV_NONE,
		   const char *defTitle = nullptr, const char *defDescription = nullptr,
		   ValidatorFunc defValidatorFunc = nullptr);
	VarDef(const core::String &defName, const char *defValue, int32_t defFlags = CV_NONE, const char *defTitle = nullptr,
		   const char *defDescription = nullptr, ValidatorFunc defValidatorFunc = nullptr);
	VarDef(const core::String &defName, int defValue, int32_t defFlags = CV_NONE, const char *defTitle = nullptr,
		   const char *defDescription = nullptr, ValidatorFunc defValidatorFunc = nullptr);
	VarDef(const core::String &defName, bool defValue, int32_t defFlags = CV_NONE, const char *defTitle = nullptr,
		   const char *defDescription = nullptr);
	/**
	 * @brief Construct an integer cvar with min/max range validation.
	 * The range is checked automatically - no need to specify a validator.
	 */
	VarDef(const core::String &defName, int defValue, int defMin, int defMax, int32_t defFlags = CV_NONE,
		   const char *defTitle = nullptr, const char *defDescription = nullptr);
	VarDef(const core::String &defName, float defValue, int32_t defFlags = CV_NONE, const char *defTitle = nullptr,
		   const char *defDescription = nullptr);
	/**
	 * @brief Construct a float cvar with min/max range validation.
	 * The range is checked automatically - no need to specify a validator.
	 */
	VarDef(const core::String &defName, float defValue, float defMin, float defMax, int32_t defFlags = CV_NONE,
		   const char *defTitle = nullptr, const char *defDescription = nullptr);
	/**
	 * @brief Construct a string cvar that only accepts values from a predefined list.
	 * The valid values are checked automatically - no need to specify a validator.
	 */
	VarDef(const core::String &defName, const core::String &defValue,
		   const core::DynamicArray<core::String> &defValidValues, int32_t defFlags = CV_NONE,
		   const char *defTitle = nullptr, const char *defDescription = nullptr);

	VarType type;
	// The name that this var is registered under (must be unique)
	core::String name;
	/**
	 * The initial value of the var.
	 */
	core::String value;
	// A bitmask of var flags - e.g. @c CV_READONLY
	int32_t flags = CV_NONE;
	/**
	 * untranslated title for this var that can be used as a label in the ui
	 * use N_() to mark it for translation, but don't translate it immediately
	 * when using it in the ui, use the _() macro to translate it
	 * if empty, the name is used as a fallback
	 */
	core::String title;
	/**
	 * untranslated description text for this var, can be empty
	 * when using it in the ui, use the _() macro to translate it
	 * use N_() to mark it for translation, but don't translate it immediately
	 */
	core::String description;
	ValidatorFunc validatorFunc = nullptr;
	/**
	 * @brief Min/max range for numeric cvars.
	 * If @c hasMinMax() returns true, the range is active and checked on every @c setVal call.
	 * Use a @c RangeValue union to store int or float range values depending on @c VarType.
	 */
	RangeValue minValue;
	RangeValue maxValue;
	bool hasMinMax() const;
	/**
	 * @brief List of valid string values for enum-style cvars.
	 * If not empty, only these values are accepted on @c setVal calls.
	 */
	core::DynamicArray<core::String> validValues;
	bool hasValidValues() const;
};

/**
 * @brief A var can be changed and queried at runtime
 *
 * Create a new variable with all parameters
 * @code
 * core::VarDef varDef("prefix_name", "value");
 * core::VarPtr var = core::Var::registerVar(varDef);
 * @endcode
 *
 * If you just want to get an existing variable use:
 * @code
 * core::getVar("prefix_name");
 * @endcode
 */
class Var {
public:
protected:
	friend class SharedPtr<Var>;
	typedef StringMap<VarPtr, 64> VarMap;
	static VarMap _vars;
	static Lock _lock;

	static constexpr int NEEDS_SHADERUPDATE = 1 << 2;
	static constexpr int NEEDS_SAVING = 1 << 3;

	static uint8_t _visitFlags;

	struct Value {
		float _floatValue = 0.0f;
		int _intValue = 0;
		core::String _value;
	};

	VarDef _def;
	uint32_t _flags;

	core::DynamicArray<Value, 16u> _history;
	uint16_t _currentHistoryPos = 0;
	bool _dirty = false;
	uint8_t _updateFlags = 0u;

	void addValueToHistory(const core::String& value);
	static bool _ivec3ListValidator(const core::String& value, int nmin, int nmax);

	/**
	 * @brief Creates a new or gets an already existing var
	 */
	static VarPtr createVar(const VarDef &def);

	// invisible - use the static get method
	Var(VarDef def, const core::String &currentValue, uint32_t flags);
public:
	~Var();

	void reset() {
		setVal(_def.value);
	}

	static size_t size() {
		ScopedLock scoped(_lock);
		return _vars.size();
	}

	// TODO: move into validator namespace
	template<size_t NMIN, size_t NMAX>
	static bool ivec3ListValidator(const core::String& value) {
		if (value.empty()) {
			return true;
		}
		return _ivec3ListValidator(value, NMIN, NMAX);
	}

	/**
	 * @brief Find a var by name.
	 * @return An empty pointer if the var doesn't exist yet.
	 * @sa getVar()
	 */
	static VarPtr findVar(const core::String& name);

	static VarPtr registerVar(const VarDef &def);

	/**
	 * @note variable must exists, otherwise @c core_assert triggers
	 * @sa findVar()
	 */
	static VarPtr getVar(const core::String& name);

	/**
	 * @return Untranslated title
	 */
	const core::String &title() const;

	/**
	 * @return Untranslated description
	 */
	const core::String &description() const;

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
	/**
	 * @return @c true if the value was set, @c false otherwise
	 */
	bool setVal(const core::String& value);
	bool setVal(const char* value);
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

	/**
	 * @return the minimum value for numeric cvars as float
	 * @note For int vars this converts from int to float
	 */
	float minValue() const;
	/**
	 * @return the maximum value for numeric cvars as float
	 * @note For int vars this converts from int to float
	 */
	float maxValue() const;
	/**
	 * @return the minimum value for int cvars
	 */
	int intMinValue() const;
	/**
	 * @return the maximum value for int cvars
	 */
	int intMaxValue() const;
	/**
	 * @return the minimum value for float cvars
	 */
	float floatMinValue() const;
	/**
	 * @return the maximum value for float cvars
	 */
	float floatMaxValue() const;
	/**
	 * @return @c true if this var has a min/max range set
	 */
	bool hasMinMax() const;
	/**
	 * @return the @c VarType of this var
	 */
	VarType type() const;
	/**
	 * @return The list of valid values for enum-style cvars. Empty if any value is accepted.
	 */
	const core::DynamicArray<core::String> &validValues() const;
};

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

inline const core::String& Var::strVal() const {
	return _history[_currentHistoryPos]._value;
}

inline bool Var::boolVal() const {
	return strVal() == "true" || strVal() == "1";
}

inline const core::String& Var::name() const {
	return _def.name;
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
	return static_cast<unsigned int>(intVal());
}

inline const core::String &Var::title() const {
	return _def.title;
}

inline const core::String &Var::description() const {
	return _def.description;
}

inline float Var::minValue() const {
	if (_def.type == VarType::Int)
		return (float)_def.minValue.i;
	return _def.minValue.f;
}

inline float Var::maxValue() const {
	if (_def.type == VarType::Int)
		return (float)_def.maxValue.i;
	return _def.maxValue.f;
}

inline int Var::intMinValue() const {
	return _def.minValue.i;
}

inline int Var::intMaxValue() const {
	return _def.maxValue.i;
}

inline float Var::floatMinValue() const {
	return _def.minValue.f;
}

inline float Var::floatMaxValue() const {
	return _def.maxValue.f;
}

inline bool Var::hasMinMax() const {
	return _def.hasMinMax();
}

inline VarType Var::type() const {
	return _def.type;
}

inline const core::DynamicArray<core::String> &Var::validValues() const {
	return _def.validValues;
}

static inline VarPtr findVar(const core::String& name) {
	return Var::findVar(name);
}

static inline VarPtr getVar(const core::String& name) {
	return Var::getVar(name);
}

static inline VarPtr registerVar(const VarDef &def) {
	return Var::registerVar(def);
}

/**
 * @}
 */

}

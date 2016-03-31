#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "String.h"
#include "ReadWriteLock.h"
#include "GameConfig.h"

namespace core {

const unsigned int CV_READONLY = 1 << 0;
// don't create empty value config vars
const unsigned int CV_NOTCREATEEMPTY = 1 << 1;
// will not get saved to the file
const unsigned int CV_NOPERSIST = 1 << 2;

class Var;
typedef std::shared_ptr<Var> VarPtr;

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
 * But beware, if the variable is not yet created, you will create a new variable with an empty string.
 * A later following call that specifies a saner default value would not count anymore (because the
 * variable already exists.
 */
class Var {
private:
	typedef std::unordered_map<std::string, VarPtr> VarMap;
	static VarMap _vars;
	static ReadWriteLock _lock;

	const std::string _name;
	const unsigned int _flags;

	float _floatValue;
	int _intValue;
	long _longValue;
	std::string _value;
	bool _dirty;

	// private - use the static get method
	Var(const std::string& name, const std::string& value = "", unsigned int flags = 0u);
public:
	/**
	 * @brief Creates a new or gets an already existing var
	 *
	 * @param[in] name The name that this var is registered under (must be unique)
	 * @param[in] value The initial value of the var
	 * @param[in] flags A bitmask of var flags - e.g. @c CV_READONLY
	 *
	 * @note This is using a read/write lock to allow access from different threads.
	 */
	static VarPtr get(const std::string& name, const std::string& value = "", unsigned int flags = 0u);
	virtual ~Var();

	template<class Functor>
	static void visit(Functor func) {
		ScopedReadLock lock(_lock);
		for (auto i = _vars.begin(); i != _vars.end(); ++i) {
			func(i->second);
		}
	}

	/**
	 * @return the bitmask of flags for this var
	 * @note See the existing @c CV_ ints
	 */
	int getFlags() const;
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
	void setVal(const std::string& value);
	/**
	 * @return The string value of this var
	 */
	const std::string& strVal() const;
	const std::string& name() const;
	/**
	 * @return @c true if some @c Var::setVal call changed the initial/default value that was specified on construction
	 */
	bool isDirty() const;
};

inline float Var::floatVal() const {
	return _floatValue;
}

inline int Var::intVal() const {
	return _intValue;
}

inline long Var::longVal() const {
	return _longValue;
}

inline unsigned long Var::ulongVal() const {
	return static_cast<unsigned long>(_longValue);
}

inline bool Var::boolVal() const {
	return _value == "true" || _value == "1";
}

inline const std::string& Var::strVal() const {
	return _value;
}

inline const std::string& Var::name() const {
	return _name;
}

inline bool Var::isDirty() const {
	return _dirty;
}

inline int Var::getFlags() const {
	return _flags;
}

inline unsigned int Var::uintVal() const {
	return static_cast<unsigned int>(_intValue);
}


}

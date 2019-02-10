/**
 * @file
 */

#pragma once

#include "tb_core.h"
#include "tb_linklist.h"

namespace tb {

typedef void* TB_TYPE_ID;

/*  TBTypedObject implements custom RTTI so we can get type safe casts,
	and the class name at runtime.

	Each subclass is expected to define TBOBJECT_SUBCLASS to get the
	necessary implementations, instead of implementing those manually. */
class TBTypedObject
{
public:
	virtual ~TBTypedObject() {}

	/** A static template method that returns a unique id for each type. */
	template<class T> static TB_TYPE_ID getTypeId() { static char type_id; return &type_id; }

	/** Returns true if the class or the base class matches the type id */
	virtual bool isOfTypeId(const TB_TYPE_ID type_id) const { return type_id == getTypeId<TBTypedObject>(); }

	/** Returns this object as the given type or nullptr if it's not that type. */
	template<class T> T *safeCastTo() const { return (T*) (isOfTypeId(getTypeId<T>()) ? this : nullptr); }

	/** Return true if this object can safely be casted to the given type. */
	template<class T> bool isOfType() const { return safeCastTo<T>() ? true : false; }

	/** Get the classname of the object. */
	virtual const char *getClassName() const { return "TBTypedObject"; }
};

/** Returns the given object as the given type, or nullptr if it's not that type
	or if the object is nullptr. */
template<class T> T *TBSafeCast(TBTypedObject *obj) {
	return obj ? obj->safeCastTo<T>() : nullptr;
}

/** Returns the given object as the given type, or nullptr if it's not that type
	or if the object is nullptr. */
template<class T> const T *TBSafeCast(const TBTypedObject *obj) {
	return obj ? obj->safeCastTo<T>() : nullptr;
}

/** Implement the methods for safe typecasting without requiring RTTI. */
#define TBOBJECT_SUBCLASS(clazz, baseclazz) \
	virtual const char *getClassName() const override { return #clazz; } \
	virtual bool isOfTypeId(const tb::TB_TYPE_ID type_id) const override \
		{ return getTypeId<clazz>() == type_id ? true : baseclazz::isOfTypeId(type_id); }

} // namespace tb

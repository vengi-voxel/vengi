/**
 * @file
 */

#pragma once

#include "tb_types.h"
#include "tb_hash.h"
#include "tb_str.h"

namespace tb {

/** TBID is a wrapper for a uint32 to be used as ID.
	The uint32 can be set directly to any uint32, or it can be
	set from a string which will be hashed into the uint32. */
class TBID
{
public:
	constexpr TBID(uint32_t newid = 0) : id(newid) {}
	constexpr TBID(const char *string) : id(TBGetHash(string)) {}
	constexpr TBID(const TBID &newid) : id(newid.id) {}
	void Set(uint32_t newid)			{ id = newid; }
	void Set(const TBID &newid)		{ id = newid; }
	void Set(const char *string)	{ id = TBGetHash(string); }

	operator uint32_t () const		{ return id; }
	const TBID& operator = (const TBID &id) { Set(id); return *this; }
private:
	uint32_t id;
};

} // namespace tb

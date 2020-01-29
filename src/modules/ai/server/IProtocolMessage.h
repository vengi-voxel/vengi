/**
 * @file
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include "core/String.h"
#include <deque>
#define AI_LIL_ENDIAN  1234
#define AI_BIG_ENDIAN  4321
#ifdef __linux__
#include <endian.h>
#define AI_BYTEORDER  __BYTE_ORDER
#else
#define AI_BYTEORDER   AI_LIL_ENDIAN
#endif

#if AI_BYTEORDER == AI_LIL_ENDIAN
#define AI_SwapLE16(X) (X)
#define AI_SwapLE32(X) (X)
#define AI_SwapLE64(X) (X)
#define AI_SwapBE16(X) AI_Swap16(X)
#define AI_SwapBE32(X) AI_Swap32(X)
#define AI_SwapBE64(X) AI_Swap64(X)
#else
#define AI_SwapLE16(X) AI_Swap16(X)
#define AI_SwapLE32(X) AI_Swap32(X)
#define AI_SwapLE64(X) AI_Swap64(X)
#define AI_SwapBE16(X) (X)
#define AI_SwapBE32(X) (X)
#define AI_SwapBE64(X) (X)
#endif

namespace ai {

typedef uint8_t ProtocolId;
typedef std::deque<uint8_t> streamContainer;

const ProtocolId PROTO_PING = 0;
const ProtocolId PROTO_STATE = 1;
const ProtocolId PROTO_CHARACTER_STATIC = 2;
const ProtocolId PROTO_CHARACTER_DETAILS = 3;
const ProtocolId PROTO_SELECT = 4;
const ProtocolId PROTO_PAUSE = 5;
const ProtocolId PROTO_CHANGE = 6;
const ProtocolId PROTO_NAMES = 7;
const ProtocolId PROTO_RESET = 8;
const ProtocolId PROTO_STEP = 9;
const ProtocolId PROTO_UPDATENODE = 10;
const ProtocolId PROTO_DELETENODE = 11;
const ProtocolId PROTO_ADDNODE = 12;

/**
 * @brief A protocol message is used for the serialization of the ai states for remote debugging
 *
 * @note Message byte order is big endian
 */
class IProtocolMessage {
private:
#if defined(__GNUC__) && defined(__i386__)
	static inline uint16_t AI_Swap16(uint16_t x) {
		__asm__("xchgb %b0,%h0": "=q"(x):"0"(x));
		return x;
	}
#elif defined(__GNUC__) && defined(__x86_64__)
	static inline uint16_t AI_Swap16(uint16_t x) {
		__asm__("xchgb %b0,%h0": "=Q"(x):"0"(x));
		return x;
	}
#else
	static inline uint16_t AI_Swap16(uint16_t x) {
		return static_cast<uint16_t>((x << 8) | (x >> 8));
	}
#endif

#if defined(__GNUC__) && defined(__i386__)
	static inline uint32_t AI_Swap32(uint32_t x) {
		__asm__("bswap %0": "=r"(x):"0"(x));
		return x;
	}
#elif defined(__GNUC__) && defined(__x86_64__)
	static inline uint32_t AI_Swap32(uint32_t x) {
		__asm__("bswapl %0": "=r"(x):"0"(x));
		return x;
	}
#else
	static inline uint32_t AI_Swap32(uint32_t x) {
		return static_cast<uint32_t>((x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x >> 24));
	}
#endif

#if defined(__GNUC__) && defined(__i386__)
	static inline uint64_t AI_Swap64(uint64_t x) {
		union {
			struct {
				uint32_t a, b;
			}s;
			uint64_t u;
		}v;
		v.u = x;
		__asm__("bswapl %0 ; bswapl %1 ; xchgl %0,%1": "=r"(v.s.a), "=r"(v.s.b):"0"(v.s.a), "1"(v.s. b));
		return v.u;
	}
#elif defined(__GNUC__) && defined(__x86_64__)
	static inline uint64_t AI_Swap64(uint64_t x) {
		__asm__("bswapq %0": "=r"(x):"0"(x));
		return x;
	}
#else
	static inline uint64_t AI_Swap64(uint64_t x) {
		/* Separate into high and low 32-bit values and swap them */
		const uint32_t lo = static_cast<uint32_t>(x & 0xFFFFFFFF);
		x >>= 32;
		const uint32_t hi = static_cast<uint32_t>(x & 0xFFFFFFFF);
		x = AI_Swap32(lo);
		x <<= 32;
		x |= AI_Swap32(hi);
		return x;
	}
#endif

protected:
	const ProtocolId _id;

public:
	static void addByte(streamContainer& out, uint8_t byte);
	static void addBool(streamContainer& out, bool value);
	static void addShort(streamContainer& out, int16_t word);
	static void addInt(streamContainer& out, int32_t dword);
	static void addLong(streamContainer& out, int64_t dword);
	static void addFloat(streamContainer& out, float value);
	static void addString(streamContainer& out, const core::String& string);

	static bool readBool(streamContainer& in);
	static uint8_t readByte(streamContainer& in);
	static int16_t readShort(streamContainer& in);
	static int32_t peekInt(const streamContainer& in);
	static int32_t readInt(streamContainer& in);
	static int64_t readLong(streamContainer& in);
	static float readFloat(streamContainer& in);
	static core::String readString(streamContainer& in);

public:
	explicit IProtocolMessage(const ProtocolId& id) :
			_id(id) {
	}

	virtual ~IProtocolMessage() {
	}

	inline const ProtocolId& getId() const {
		return _id;
	}

	virtual void serialize(streamContainer& out) const {
		addByte(out, _id);
	}
};

inline void IProtocolMessage::addByte(streamContainer& out, uint8_t byte) {
	out.push_back(byte);
}

inline void IProtocolMessage::addBool(streamContainer& out, bool value) {
	out.push_back(value);
}

inline bool IProtocolMessage::readBool(streamContainer& in) {
	return readByte(in) == 1;
}

inline uint8_t IProtocolMessage::readByte(streamContainer& in) {
	const uint8_t b = in.front();
	in.pop_front();
	return b;
}

inline void IProtocolMessage::addFloat(streamContainer& out, float value) {
	union toint {
		float f;
		int32_t i;
	} tmp;
	tmp.f = value;
	addInt(out, tmp.i);
}

inline float IProtocolMessage::readFloat(streamContainer& in) {
	union toint {
		float f;
		int32_t i;
	} tmp;
	tmp.i = readInt(in);
	return tmp.f;
}

inline core::String IProtocolMessage::readString(streamContainer& in) {
	core::String strbuff;
	strbuff.reserve(64);
	for (;;) {
		const char chr = static_cast<char>(in.front());
		in.pop_front();
		if (chr == '\0')
			break;
		strbuff += chr;
	}
	return strbuff;
}

inline void IProtocolMessage::addString(streamContainer& out, const core::String& string) {
	const std::size_t length = string.size();
	for (std::size_t i = 0; i < length; ++i) {
		out.push_back(uint8_t(string[i]));
	}
	out.push_back(uint8_t('\0'));
}

inline void IProtocolMessage::addShort(streamContainer& out, int16_t word) {
	const int16_t swappedWord = AI_SwapLE16(word);
	out.push_back(uint8_t(swappedWord));
	out.push_back(uint8_t(swappedWord >> CHAR_BIT));
}

inline void IProtocolMessage::addInt(streamContainer& out, int32_t dword) {
	int32_t swappedDWord = AI_SwapLE32(dword);
	out.push_back(uint8_t(swappedDWord));
	out.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	out.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	out.push_back(uint8_t(swappedDWord >> CHAR_BIT));
}

inline void IProtocolMessage::addLong(streamContainer& out, int64_t dword) {
	int64_t swappedDWord = AI_SwapLE64(dword);
	out.push_back(uint8_t(swappedDWord));
	out.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	out.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	out.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	out.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	out.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	out.push_back(uint8_t(swappedDWord >>= CHAR_BIT));
	out.push_back(uint8_t(swappedDWord >> CHAR_BIT));
}

inline int16_t IProtocolMessage::readShort(streamContainer& in) {
	uint8_t buf[2];
	const int l = sizeof(buf);
	for (int i = 0; i < l; ++i) {
		buf[i] = in.front();
		in.pop_front();
	}
	const int16_t *word = (const int16_t*) (void*) &buf;
	const int16_t val = AI_SwapLE16(*word);
	return val;
}

inline int32_t IProtocolMessage::readInt(streamContainer& in) {
	uint8_t buf[4];
	const int l = sizeof(buf);
	for (int i = 0; i < l; ++i) {
		buf[i] = in.front();
		in.pop_front();
	}
	const int32_t *word = (const int32_t*) (void*) &buf;
	const int32_t val = AI_SwapLE32(*word);
	return val;
}

inline int32_t IProtocolMessage::peekInt(const streamContainer& in) {
	uint8_t buf[4];
	const int l = sizeof(buf);
	streamContainer::const_iterator it = in.begin();
	for (int i = 0; i < l; ++i) {
		if (it == in.end())
			return -1;
		buf[i] = *it;
		++it;
	}
	const int32_t *word = (const int32_t*) (void*) &buf;
	const int32_t val = AI_SwapLE32(*word);
	return val;
}

inline int64_t IProtocolMessage::readLong(streamContainer& in) {
	uint8_t buf[8];
	const int l = sizeof(buf);
	for (int i = 0; i < l; ++i) {
		buf[i] = in.front();
		in.pop_front();
	}
	const int64_t *word = (const int64_t*) (void*) &buf;
	const int64_t val = AI_SwapLE64(*word);
	return val;
}

#define PROTO_MSG(name, id) class name : public IProtocolMessage { public: name() : IProtocolMessage(id) {} }

/**
 * @brief Reset the behaviour tree states for all ai controlled entities
 */
PROTO_MSG(AIResetMessage, PROTO_RESET);
/**
 * @brief Protocol keep alive message
 */
PROTO_MSG(AIPingMessage, PROTO_PING);
}

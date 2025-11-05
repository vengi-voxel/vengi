/**
 * @file lzav.h
 *
 * @version 4.23
 *
 * @brief Self-contained inclusion file for the "LZAV" in-memory data
 * compression and decompression algorithms.
 *
 * The source code is written in ISO C99, with full C++ compliance enabled
 * conditionally and automatically, if compiled with a C++ compiler.
 *
 * Description is available at https://github.com/avaneev/lzav
 *
 * E-mail: aleksey.vaneev@gmail.com or info@voxengo.com
 *
 * LICENSE:
 *
 * Copyright (c) 2023-2025 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef LZAV_INCLUDED
#define LZAV_INCLUDED

#define LZAV_API_VER 0x109 ///< API version, unrelated to source code version.
#define LZAV_VER_STR "4.23" ///< LZAV source code version string.

/**
 * @def LZAV_FMT_MIN
 * @brief Minimal stream format id supported by the decompressor. A value of 2
 * can be defined externally, to reduce decompressor's code size.
 */

#if !defined( LZAV_FMT_MIN )
	#define LZAV_FMT_MIN 1
#endif // !defined( LZAV_FMT_MIN )

/**
 * @def LZAV_NS_CUSTOM
 * @brief If this macro is defined externally, all symbols will be placed
 * into the namespace specified by the macro, and won't be exported to the
 * global namespace. WARNING: if the defined value of the macro is empty, the
 * symbols will be placed into the global namespace anyway.
 */

/**
 * @def LZAV_NOEX
 * @brief Macro that defines the "noexcept" function specifier for C++
 * environment.
 */

/**
 * @def LZAV_NULL
 * @brief Macro that defines "nullptr" value, for C++ guidelines conformance.
 */

/**
 * @def LZAV_NS
 * @brief Macro that defines an actual implementation namespace in C++
 * environment, with export of relevant symbols to the global namespace
 * (if @ref LZAV_NS_CUSTOM is undefined).
 */

/**
 * @def LZAV_MALLOC
 * @brief Macro that defines the call to the memory allocation function. Can
 * be defined externally, if the standard `malloc` is unavailable.
 *
 * @param s Allocation size, in bytes.
 */

/**
 * @def LZAV_FREE
 * @brief Macro that defines the call to the memory free function. Can be
 * defined externally, if the standard `free` is unavailable.
 *
 * @param p Memory block pointer to free.
 */

#if defined( __cplusplus )

	#include <cstring>

	#if __cplusplus >= 201103L

		#include <cstdint>

		#define LZAV_NOEX noexcept
		#define LZAV_NULL nullptr

	#else // __cplusplus >= 201103L

		#include <stdint.h>

		#define LZAV_NOEX throw()
		#define LZAV_NULL NULL

	#endif // __cplusplus >= 201103L

	#if defined( LZAV_NS_CUSTOM )
		#define LZAV_NS LZAV_NS_CUSTOM
	#else // defined( LZAV_NS_CUSTOM )
		#define LZAV_NS lzav
	#endif // defined( LZAV_NS_CUSTOM )

	#if !defined( LZAV_MALLOC )
		#include <cstdlib>
		#define LZAV_MALLOC( s ) std :: malloc( s )
	#endif // !defined( LZAV_MALLOC )

	#if !defined( LZAV_FREE )
		#define LZAV_FREE( p ) std :: free( p )
	#endif // !defined( LZAV_FREE )

#else // defined( __cplusplus )

	#include <string.h>
	#include <stdint.h>

	#define LZAV_NOEX
	#define LZAV_NULL 0

	#if !defined( LZAV_MALLOC )
		#include <stdlib.h>
		#define LZAV_MALLOC( s ) malloc( s )
	#endif // !defined( LZAV_MALLOC )

	#if !defined( LZAV_FREE )
		#define LZAV_FREE( p ) free( p )
	#endif // !defined( LZAV_FREE )

#endif // defined( __cplusplus )

#if SIZE_MAX < 0xFFFFFFFFU

	#error LZAV: the platform or the compiler has incompatible size_t type.

#endif // size_t check

/**
 * @def LZAV_X86
 * @brief Macro is defined, if `x86` or `x86_64` platform was detected.
 */

#if defined( i386 ) || defined( __i386 ) || defined( __i386__ ) || \
	defined( _X86_ ) || defined( __x86_64 ) || defined( __x86_64__ ) || \
	defined( __amd64 ) || defined( __amd64__ ) || defined( _M_IX86 ) || \
	( defined( _M_AMD64 ) && !defined( _M_ARM64EC ))

	#define LZAV_X86

#endif // x86 platform check

/**
 * @def LZAV_LITTLE_ENDIAN
 * @brief Endianness definition macro, can be used as a logical constant.
 * Equals 0, if C++20 `endian` is in use.
 */

/**
 * @def LZAV_COND_EC( vl, vb )
 * @brief Macro that emits either `vl` or `vb`, depending on platform's
 * endianness.
 */

#if ( defined( __BYTE_ORDER__ ) && \
		__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ) || \
	( defined( __BYTE_ORDER ) && __BYTE_ORDER == __LITTLE_ENDIAN ) || \
	defined( __LITTLE_ENDIAN__ ) || defined( _LITTLE_ENDIAN ) || \
	defined( LZAV_X86 ) || defined( _WIN32 ) || defined( _M_ARM ) || \
	defined( _M_ARM64EC )

	#define LZAV_LITTLE_ENDIAN 1

#elif ( defined( __BYTE_ORDER__ ) && \
		__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ ) || \
	( defined( __BYTE_ORDER ) && __BYTE_ORDER == __BIG_ENDIAN ) || \
	defined( __BIG_ENDIAN__ ) || defined( _BIG_ENDIAN ) || \
	defined( __SYSC_ZARCH__ ) || defined( __zarch__ ) || \
	defined( __s390x__ ) || defined( __sparc ) || defined( __sparc__ )

	#define LZAV_LITTLE_ENDIAN 0
	#define LZAV_COND_EC( vl, vb ) ( vb )

#elif defined( __cplusplus ) && __cplusplus >= 202002L

	#include <bit>

	#define LZAV_LITTLE_ENDIAN 0
	#define LZAV_COND_EC( vl, vb ) ( std :: endian :: native == \
		std :: endian :: little ? vl : vb )

#else // defined( __cplusplus )

	#warning LZAV: cannot determine endianness, assuming little-endian.

	#define LZAV_LITTLE_ENDIAN 1

#endif // defined( __cplusplus )

/**
 * @def LZAV_PTR32
 * @brief Macro denotes that pointers are likely 32-bit (pointer overflow
 * checks are required).
 */

#if SIZE_MAX <= 0xFFFFFFFFU && \
	( !defined( UINTPTR_MAX ) || UINTPTR_MAX <= 0xFFFFFFFFU )

	#define LZAV_PTR32

#endif // 32-bit pointers check

/**
 * @def LZAV_ARCH64
 * @brief Macro that denotes availability of 64-bit instructions.
 */

#if defined( __LP64__ ) || defined( _LP64 ) || !defined( LZAV_PTR32 ) || \
	defined( __x86_64__ ) || defined( __aarch64__ ) || \
	defined( _M_AMD64 ) || defined( _M_ARM64 )

	#define LZAV_ARCH64

#endif // 64-bit availability check

/**
 * @def LZAV_GCC_BUILTINS
 * @brief Macro that denotes availability of GCC-style built-in functions.
 */

/**
 * @def LZAV_CPP_BIT
 * @brief Macro that denotes availability of C++20 `bit` functions.
 */

#if defined( __GNUC__ ) || defined( __clang__ ) || \
	defined( __IBMC__ ) || defined( __IBMCPP__ ) || \
	defined( __COMPCERT__ ) || ( defined( __INTEL_COMPILER ) && \
		__INTEL_COMPILER >= 1300 && !defined( _MSC_VER ))

	#define LZAV_GCC_BUILTINS

#elif defined( __cplusplus ) && __cplusplus >= 202002L

	#include <bit>

	#define LZAV_CPP_BIT

#elif defined( _MSC_VER )

	#include <intrin.h> // For _BitScanForward.

#endif // defined( _MSC_VER )

/**
 * @def LZAV_IEC32( x )
 * @brief In-place endianness-correction macro, for singular 32-bit variables.
 *
 * @param x Value to correct in-place.
 */

#if LZAV_LITTLE_ENDIAN

	#define LZAV_COND_EC( vl, vb ) ( vl )
	#define LZAV_IEC32( x ) (void) 0

#else // LZAV_LITTLE_ENDIAN

	#if defined( LZAV_GCC_BUILTINS )

		#define LZAV_IEC32( x ) x = LZAV_COND_EC( x, __builtin_bswap32( x ))

	#elif defined( _MSC_VER )

		#define LZAV_IEC32( x ) x = LZAV_COND_EC( x, _byteswap_ulong( x ))

	#elif defined( __cplusplus ) && __cplusplus >= 202302L

		#define LZAV_IEC32( x ) x = LZAV_COND_EC( x, std :: byteswap( x ))

	#else // defined( __cplusplus )

		#define LZAV_IEC32( x ) x = (uint32_t) LZAV_COND_EC( x, \
			x >> 24 | \
			( x & 0x00FF0000 ) >> 8 | \
			( x & 0x0000FF00 ) << 8 | \
			x << 24 )

	#endif // defined( __cplusplus )

#endif // LZAV_LITTLE_ENDIAN

/**
 * @def LZAV_LIKELY( x )
 * @brief Likelihood macro that is used for manually-guided
 * micro-optimization.
 *
 * @param x Expression that is likely to be evaluated to 1.
 */

/**
 * @def LZAV_UNLIKELY( x )
 * @brief Unlikelihood macro that is used for manually-guided
 * micro-optimization.
 *
 * @param x Expression that is unlikely to be evaluated to 1.
 */

#if defined( LZAV_GCC_BUILTINS )

	#define LZAV_LIKELY( x ) ( __builtin_expect( x, 1 ))
	#define LZAV_UNLIKELY( x ) ( __builtin_expect( x, 0 ))

#elif defined( __cplusplus ) && __cplusplus >= 202002L

	#define LZAV_LIKELY( x ) ( x ) [[likely]]
	#define LZAV_UNLIKELY( x ) ( x ) [[unlikely]]

#else // Likelihood macros

	#define LZAV_LIKELY( x ) ( x )
	#define LZAV_UNLIKELY( x ) ( x )

#endif // Likelihood macros

/**
 * @def LZAV_PREFETCH( a )
 * @brief Memory address prefetch macro, to preload some data into CPU cache.
 *
 * @param a Prefetch address.
 */

#if defined( LZAV_GCC_BUILTINS ) && !defined( __COMPCERT__ )

	#define LZAV_PREFETCH( a ) __builtin_prefetch( a, 0, 3 )

#elif defined( _MSC_VER ) && !defined( __INTEL_COMPILER ) && \
	defined( LZAV_X86 )

	#include <intrin.h>

	#define LZAV_PREFETCH( a ) _mm_prefetch( (const char*) ( a ), _MM_HINT_T0 )

#else // defined( _MSC_VER )

	#define LZAV_PREFETCH( a ) (void) 0

#endif // defined( _MSC_VER )

/**
 * @def LZAV_INLINE
 * @brief Macro that defines a function as inlinable at compiler's discretion.
 */

#if ( defined( __cplusplus ) && __cplusplus >= 201703L ) || \
	( defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 202311L )

	#define LZAV_INLINE [[maybe_unused]] static inline

#else // defined( __cplusplus )

	#define LZAV_INLINE static inline

#endif // defined( __cplusplus )

/**
 * @def LZAV_INLINE_F
 * @brief Macro to force code inlining.
 */

#if defined( LZAV_GCC_BUILTINS )

	#define LZAV_INLINE_F LZAV_INLINE __attribute__((always_inline))

#elif defined( _MSC_VER )

	#define LZAV_INLINE_F LZAV_INLINE __forceinline

#else // defined( _MSC_VER )

	#define LZAV_INLINE_F LZAV_INLINE

#endif // defined( _MSC_VER )

#if defined( LZAV_NS )

namespace LZAV_NS {

using std :: memcpy;
using std :: memset;
using std :: size_t;

#if __cplusplus >= 201103L

	using std :: intptr_t;
	using std :: uint16_t;
	using std :: uint32_t;
	using uint8_t = unsigned char; ///< For C++ type aliasing compliance.

	#if defined( LZAV_ARCH64 )
		using std :: uint64_t;
	#endif // defined( LZAV_ARCH64 )

#endif // __cplusplus >= 201103L

namespace enum_wrapper {

#endif // defined( LZAV_NS )

/**
 * @brief Decompression error codes.
 */

enum LZAV_ERROR
{
	LZAV_E_PARAMS = -1, ///< Incorrect function parameters.
	LZAV_E_SRCOOB = -2, ///< Source buffer OOB.
	LZAV_E_DSTOOB = -3, ///< Destination buffer OOB.
	LZAV_E_REFOOB = -4, ///< Back-reference OOB.
	LZAV_E_DSTLEN = -5, ///< Decompressed length mismatch.
	LZAV_E_UNKFMT = -6, ///< Unknown stream format.
	LZAV_E_PTROVR = -7 ///< Pointer overflow.
};

#if defined( LZAV_NS )

} // namespace enum_wrapper

using namespace enum_wrapper;

#endif // defined( LZAV_NS )

/**
 * @brief Compression algorithm's parameters.
 */

enum LZAV_PARAM
{
	LZAV_WIN_LEN = ( 1 << 23 ), ///< LZ77 window length, in bytes.
	LZAV_REF_LEN = ( 15 + 255 + 254 ), ///< Max ref length, minus `mref`.
	LZAV_LIT_FIN = 6, ///< The number of literals required at finish.
	LZAV_OFS_MIN = 8, ///< The minimal reference offset to use.
	LZAV_OFS_TH1 = (( 1 << 10 ) - 1 ), ///< Reference offset threshold 1.
	LZAV_OFS_TH2 = (( 1 << 18 ) - 1 ), ///< Reference offset threshold 2.
	LZAV_FMT_CUR = 2 ///< Stream format identifier used by the compressor.
};

/**
 * @brief Data match length finding function.
 *
 * Function finds the number of continuously-matching leading bytes between
 * two buffers. This function is well-optimized for a wide variety of
 * compilers and platforms.
 *
 * @param p1 Pointer to buffer 1.
 * @param p2 Pointer to buffer 2.
 * @param ml Maximal number of bytes to match.
 * @param o Initial offset, can be greater than `ml`.
 * @return The number of matching leading bytes, not less than `o`.
 */

LZAV_INLINE_F size_t lzav_match_len( const uint8_t* const p1,
	const uint8_t* const p2, const size_t ml, size_t o ) LZAV_NOEX
{
#if defined( LZAV_ARCH64 )

	size_t o2 = o + 7;

	while LZAV_LIKELY( o2 < ml )
	{
		uint64_t v1, v2, vd;
		memcpy( &v1, p1 + o, 8 );
		memcpy( &v2, p2 + o, 8 );
		vd = v1 ^ v2;

		if( vd != 0 )
		{
		#if defined( LZAV_GCC_BUILTINS )

			return( o + (size_t) ( LZAV_COND_EC(
				__builtin_ctzll( vd ), __builtin_clzll( vd )) >> 3 ));

		#elif defined( LZAV_CPP_BIT )

			return( o + (size_t) ( LZAV_COND_EC(
				std :: countr_zero( vd ), std :: countl_zero( vd )) >> 3 ));

		#elif defined( _MSC_VER )

			unsigned long i;
			_BitScanForward64( &i, (unsigned __int64) vd );

			return( o + ( i >> 3 ));

		#else // defined( _MSC_VER )

			#if !LZAV_LITTLE_ENDIAN
				const uint64_t sw = vd >> 32 | vd << 32;
				const uint64_t sw2 =
					( sw & (uint64_t) 0xFFFF0000FFFF0000 ) >> 16 |
					( sw & (uint64_t) 0x0000FFFF0000FFFF ) << 16;
				vd = LZAV_COND_EC( vd,
					( sw2 & (uint64_t) 0xFF00FF00FF00FF00 ) >> 8 |
					( sw2 & (uint64_t) 0x00FF00FF00FF00FF ) << 8 );
			#endif // !LZAV_LITTLE_ENDIAN

			const uint64_t m = (uint64_t) 0x0101010101010101;

			return( o + (((( vd ^ ( vd - 1 )) & ( m - 1 )) * m ) >> 56 ));

		#endif // defined( _MSC_VER )
		}

		o2 += 8;
		o += 8;
	}

	// At most 7 bytes left.

	if LZAV_LIKELY( o + 3 < ml )
	{

#else // defined( LZAV_ARCH64 )

	size_t o2 = o + 3;

	while LZAV_LIKELY( o2 < ml )
	{

#endif // defined( LZAV_ARCH64 )

		uint32_t v1, v2, vd;
		memcpy( &v1, p1 + o, 4 );
		memcpy( &v2, p2 + o, 4 );
		vd = v1 ^ v2;

		if( vd != 0 )
		{
		#if defined( LZAV_GCC_BUILTINS )

			return( o + (size_t) ( LZAV_COND_EC(
				__builtin_ctz( vd ), __builtin_clz( vd )) >> 3 ));

		#elif defined( LZAV_CPP_BIT )

			return( o + (size_t) ( LZAV_COND_EC(
				std :: countr_zero( vd ), std :: countl_zero( vd )) >> 3 ));

		#elif defined( _MSC_VER )

			unsigned long i;
			_BitScanForward( &i, (unsigned long) vd );

			return( o + ( i >> 3 ));

		#else // defined( _MSC_VER )

			LZAV_IEC32( vd );
			const uint32_t m = 0x01010101;

			return( o + (((( vd ^ ( vd - 1 )) & ( m - 1 )) * m ) >> 24 ));

		#endif // defined( _MSC_VER )
		}

		o2 += 4;
		o += 4;
	}

	// At most 3 bytes left.

	if( o < ml )
	{
		if( p1[ o ] != p2[ o ])
		{
			return( o );
		}

		if( ++o < ml )
		{
			if( p1[ o ] != p2[ o ])
			{
				return( o );
			}

			if( ++o < ml )
			{
				if( p1[ o ] != p2[ o ])
				{
					return( o );
				}
			}
		}
	}

	return( ml );
}

/**
 * @brief Data match length finding function, reverse direction.
 *
 * @param p1 Origin pointer to buffer 1.
 * @param p2 Origin pointer to buffer 2.
 * @param ml Maximal number of bytes to back-match.
 * @return The number of matching prior bytes, not including origin position.
 */

LZAV_INLINE_F size_t lzav_match_len_r( const uint8_t* p1, const uint8_t* p2,
	const size_t ml ) LZAV_NOEX
{
	if LZAV_UNLIKELY( ml == 0 )
	{
		return( 0 );
	}

	if( p1[ -1 ] != p2[ -1 ])
	{
		return( 0 );
	}

	if LZAV_UNLIKELY( ml != 1 )
	{
		const uint8_t* const p1s = p1;
		const uint8_t* const p1e = p1 - ml + 1;
		p1--;
		p2--;

		while LZAV_UNLIKELY( p1 > p1e )
		{
			uint16_t v1, v2;
			memcpy( &v1, p1 - 2, 2 );
			memcpy( &v2, p2 - 2, 2 );

			const uint32_t vd = (uint32_t) ( v1 ^ v2 );

			if( vd != 0 )
			{
				return( (size_t) ( p1s - p1 +
					( LZAV_COND_EC( vd & 0xFF00, vd & 0x00FF ) == 0 )));
			}

			p1 -= 2;
			p2 -= 2;
		}

		if( p1 + 1 > p1e && p1[ -1 ] != p2[ -1 ])
		{
			return( (size_t) ( p1s - p1 ));
		}
	}

	return( ml );
}

/**
 * @brief Internal LZAV block header writing function (stream format 2).
 *
 * Internal function writes a block to the output buffer. This function can be
 * used in custom compression algorithms.
 *
 * Stream format 2.
 *
 * "Raw" compressed stream consists of any quantity of unnumerated "blocks".
 * A block starts with a header byte, followed by several optional bytes.
 * Bits 4-5 of the header specify block's type.
 *
 * CC00LLLL: literal block (1-6 bytes). `LLLL` is literal length.
 *
 * OO01RRRR: 10-bit offset block (2-4 bytes). `RRRR` is reference length.
 *
 * OO10RRRR: 18-bit offset block (3-5 bytes).
 *
 * OO11RRRR: 23-bit offset block (4-6 bytes).
 *
 * If `LLLL` or `RRRR` equals 0, a value of 16 is assumed, and an additional
 * length byte follows. If in a literal block this additional byte's highest
 * bit is 1, one more length byte follows that defines higher bits of length
 * (up to 4 bytes). In a reference block, additional 1-2 length bytes follow
 * the offset bytes. `CC` is a reference offset carry value (additional 2
 * lowest bits of offset of the next reference block). Block type 3 includes 3
 * carry bits (highest bits of 4th byte).
 *
 * The overall compressed data is prefixed with a byte whose lower 4 bits
 * contain minimal reference length (mref), and the highest 4 bits contain
 * stream format identifier. Compressed data always finishes with
 * @ref LZAV_LIT_FIN literals. The lzav_write_fin_2() function should be used
 * to finalize compression.
 *
 * Except the last block, a literal block is always followed by a reference
 * block.
 *
 * @param op Output buffer pointer.
 * @param lc Literal length, in bytes.
 * @param rc Reference length, in bytes, not lesser than mref.
 * @param d Reference offset, in bytes. Should be lesser than
 * @ref LZAV_WIN_LEN, and not lesser than `rc` since fast copy on
 * decompression cannot provide consistency of copying of data that is not in
 * the output yet.
 * @param ipa Literals anchor pointer.
 * @param cbpp Pointer to the pointer to the latest offset carry block header.
 * Cannot be 0, but the contained pointer can be 0 (initial value).
 * @param cshp Pointer to offset carry shift.
 * @param mref Minimal reference length, in bytes, used by the compression
 * algorithm.
 * @return Incremented output buffer pointer.
 */

LZAV_INLINE_F uint8_t* lzav_write_blk_2( uint8_t* op, const size_t lc,
	size_t rc, size_t d, const uint8_t* const ipa, uint8_t** const cbpp,
	int* const cshp, const size_t mref ) LZAV_NOEX
{
	// Perform offset carry to a previous block (`csh` may be zero).

	const int csh = *cshp;
	rc = rc + 1 - mref;
	**cbpp |= (uint8_t) (( d << 8 ) >> csh );
	d >>= csh;

	if LZAV_UNLIKELY( lc != 0 )
	{
		// Write a literal block.

		const size_t cv = d << 6; // Offset carry value in literal block.
		d >>= 2;

		if LZAV_LIKELY( lc < 9 )
		{
			*op = (uint8_t) ( cv | lc );

			memcpy( op + 1, ipa, 8 );
			op += lc + 1;
		}
		else
		if LZAV_LIKELY( lc < 16 )
		{
			*op = (uint8_t) ( cv | lc );

			memcpy( op + 1, ipa, 16 );
			op += lc + 1;
		}
		else
		if( lc < 33 )
		{
			const uint16_t ov = (uint16_t) LZAV_COND_EC(
				( lc - 16 ) << 8 | ( cv & 0xFF ), cv << 8 | ( lc - 16 ));

			memcpy( op, &ov, 2 );

			memcpy( op + 2, ipa, 16 );
			memcpy( op + 18, ipa + 16, 16 );
			op += lc + 2;
		}
		else
		{
			op[ 0 ] = (uint8_t) cv;

			size_t lcw = lc - 16;

			while( lcw > 127 )
			{
				op[ 1 ] = (uint8_t) ( 0x80 | lcw );
				lcw >>= 7;
				op++;
			}

			op[ 1 ] = (uint8_t) lcw;
			op += 2;

			memcpy( op, ipa, lc );
			op += lc;
		}
	}

	// Write a reference block.

	static const int ocsh[ 4 ] = { 0, 0, 0, 3 };
	const size_t bt = (size_t) 1 + ( d > LZAV_OFS_TH1 ) + ( d > LZAV_OFS_TH2 );

	uint32_t ov = (uint32_t) ( d << 6 | bt << 4 );
	op += bt;
	*cshp = ocsh[ bt ];
	*cbpp = op;

	if LZAV_LIKELY( rc < 16 )
	{
		ov |= (uint32_t) rc;

		LZAV_IEC32( ov );
		memcpy( op - bt, &ov, 4 );

		return( op + 1 );
	}

	LZAV_IEC32( ov );
	memcpy( op - bt, &ov, 4 );

	if LZAV_LIKELY( rc < 16 + 255 )
	{
		op[ 1 ] = (uint8_t) ( rc - 16 );
		return( op + 2 );
	}

	op[ 1 ] = (uint8_t) 255;
	op[ 2 ] = (uint8_t) ( rc - 16 - 255 );
	return( op + 3 );
}

/**
 * @brief Internal LZAV finishing function (stream format 2).
 *
 * Internal function writes finishing literal block(s) to the output buffer.
 * This function can be used in custom compression algorithms.
 *
 * Stream format 2.
 *
 * @param op Output buffer pointer.
 * @param lc Literal length, in bytes. Not less than @ref LZAV_LIT_FIN.
 * @param ipa Literals anchor pointer.
 * @return Incremented output buffer pointer.
 */

LZAV_INLINE_F uint8_t* lzav_write_fin_2( uint8_t* op, const size_t lc,
	const uint8_t* const ipa ) LZAV_NOEX
{
	size_t lcw = lc;

	if( lc > 15 )
	{
		*op = 0;
		op++;

		lcw -= 16;

		while( lcw > 127 )
		{
			*op = (uint8_t) ( 0x80 | lcw );
			lcw >>= 7;
			op++;
		}
	}

	*op = (uint8_t) lcw;
	op++;

	memcpy( op, ipa, lc );
	return( op + lc );
}

/**
 * @brief Function returns buffer size required for LZAV compression.
 *
 * @param srcl The length of the source data to be compressed.
 * @return The required allocation size for destination compression buffer.
 * Always a positive value.
 */

LZAV_INLINE_F int lzav_compress_bound( const int srcl ) LZAV_NOEX
{
	if( srcl <= 0 )
	{
		return( 16 );
	}

	const int k = 16 + 127 + 1;
	const int l2 = srcl / ( k + 6 );

	return(( srcl - l2 * 6 + k - 1 ) / k * 2 - l2 + srcl + 16 );
}

/**
 * @brief Function returns buffer size required for the higher-ratio LZAV
 * compression.
 *
 * @param srcl The length of the source data to be compressed.
 * @return The required allocation size for destination compression buffer.
 * Always a positive value.
 */

LZAV_INLINE_F int lzav_compress_bound_hi( const int srcl ) LZAV_NOEX
{
	if( srcl <= 0 )
	{
		return( 16 );
	}

	const int l2 = srcl / ( 16 + 5 );

	return(( srcl - l2 * 5 + 15 ) / 16 * 2 - l2 + srcl + 16 );
}

/**
 * @brief Hash-table initialization function.
 *
 * Function initializes the hash-table by replicating the contents of the
 * specified tuple value.
 *
 * @param[out] ht Hash-table pointer.
 * @param htsize Hash-table size. The size should be a power of 2 value, not
 * lesser than 64 bytes.
 * @param[in] initv Pointer to initialized 8-byte tuple.
 */

LZAV_INLINE_F void lzav_ht_init( uint8_t* const ht, const size_t htsize,
	const uint32_t* const initv ) LZAV_NOEX
{
	memcpy( ht, initv, 8 );
	memcpy( ht + 8, initv, 8 );
	memcpy( ht + 16, ht, 16 );
	memcpy( ht + 32, ht, 16 );
	memcpy( ht + 48, ht, 16 );

	uint8_t* const hte = ht + htsize;
	uint8_t* htc = ht + 64;

	while LZAV_LIKELY( htc != hte )
	{
		memcpy( htc, ht, 16 );
		memcpy( htc + 16, ht, 16 );
		memcpy( htc + 32, ht, 16 );
		memcpy( htc + 48, ht, 16 );
		htc += 64;
	}
}

/**
 * @brief Calculates a hash value for the specified input words.
 *
 * @param iw1 Input word 1.
 * @param iw2 Input word 1.
 * @param sh Hash value shift, in bits. Should be chosen so that `32-sh` is
 * equal to hash-table's log2 size.
 * @param hmask Hash value mask.
 */

LZAV_INLINE_F uint32_t lzav_hash( const uint32_t iw1, const uint32_t iw2,
	const int sh, const uint32_t hmask ) LZAV_NOEX
{
	uint32_t Seed1 = 0x243F6A88;
	uint32_t hval = 0x85A308D3;

	Seed1 ^= iw1;
	hval ^= iw2;
	hval *= Seed1;
	hval >>= sh;

	return( hval & hmask );
}

/**
 * @brief LZAV compression function, with external buffer option.
 *
 * Function performs in-memory data compression using the LZAV compression
 * algorithm and stream format. The function produces a "raw" compressed data,
 * without a header containing data length nor identifier nor checksum.
 *
 * Note that compression algorithm and its output on the same source data may
 * differ between LZAV versions, and may differ between big- and little-endian
 * systems. However, the decompression of a compressed data produced by any
 * prior compressor version will remain possible.
 *
 * @param[in] src Source (uncompressed) data pointer, can be 0 if `srcl`
 * equals 0. Address alignment is unimportant.
 * @param[out] dst Destination (compressed data) buffer pointer. The allocated
 * size should be at least lzav_compress_bound() bytes large. Address
 * alignment is unimportant. Should be different to `src`.
 * @param srcl Source data length, in bytes, can be 0: in this case the
 * compressed length is assumed to be 0 as well.
 * @param dstl Destination buffer's capacity, in bytes.
 * @param ext_buf External buffer to use for hash-table, set to null for the
 * function to manage memory itself (via standard `malloc`). Supplying a
 * pre-allocated buffer is useful if compression is performed during
 * application's operation often: this reduces memory allocation overhead and
 * fragmentation. Note that the access to the supplied buffer is not
 * implicitly thread-safe. Buffer's address must be aligned to 32 bits.
 * @param ext_bufl The capacity of the `ext_buf`, in bytes, should be a
 * power-of-2 value. Set to 0 if `ext_buf` is null. The capacity should not be
 * lesser than `4*srcl`, and for default compression ratio should not be
 * greater than 1 MiB. Same `ext_bufl` value can be used for any smaller
 * source data. Using smaller `ext_bufl` values reduces the compression ratio
 * and, at the same time, increases compression speed. This aspect can be
 * utilized on memory-constrained and low-performance processors.
 * @return The length of compressed data, in bytes. Returns 0 if `srcl` is
 * lesser or equal to 0, or if `dstl` is too small, or if buffer pointers are
 * invalid, or if not enough memory.
 */

LZAV_INLINE int lzav_compress( const void* const src, void* const dst,
	const int srcl, const int dstl, void* const ext_buf,
	const int ext_bufl ) LZAV_NOEX
{
	if(( srcl <= 0 ) | ( src == LZAV_NULL ) | ( dst == LZAV_NULL ) |
		( src == dst ) | ( dstl < lzav_compress_bound( srcl )))
	{
		return( 0 );
	}

	const size_t mref = 6; // Minimal reference length, in bytes.
	const size_t mlen = (size_t) LZAV_REF_LEN + mref;

	uint8_t* op = (uint8_t*) dst; // Destination (compressed data) pointer.
	*op = (uint8_t) ( LZAV_FMT_CUR << 4 | mref ); // Write prefix byte.
	op++;

	if( srcl < 16 )
	{
		// Handle a very short source data.

		*op = (uint8_t) srcl;
		op++;

		memcpy( op, src, (size_t) srcl );

		if( srcl > LZAV_LIT_FIN - 1 )
		{
			return( 2 + srcl );
		}

		memset( op + srcl, 0, (size_t) ( LZAV_LIT_FIN - srcl ));
		return( 2 + LZAV_LIT_FIN );
	}

	uint32_t stack_buf[ 2048 ]; // On-stack hash-table.
	void* alloc_buf = LZAV_NULL; // Hash-table allocated on heap.
	uint8_t* ht = (uint8_t*) stack_buf; // The actual hash-table pointer.

	size_t htsize; // Hash-table's size in bytes (power-of-2).
	htsize = ( 1 << 7 ) * sizeof( uint32_t ) * 4;

	if( ext_buf == LZAV_NULL )
	{
		while( htsize != ( 1 << 20 ) && ( htsize >> 2 ) < (size_t) srcl )
		{
			htsize <<= 1;
		}

		if( htsize > sizeof( stack_buf ))
		{
			alloc_buf = LZAV_MALLOC( htsize );

			if( alloc_buf == LZAV_NULL )
			{
				return( 0 );
			}

			ht = (uint8_t*) alloc_buf;
		}
	}
	else
	{
		size_t htsizem;

		if( ext_bufl > (int) sizeof( stack_buf ))
		{
			htsizem = (size_t) ext_bufl;
			ht = (uint8_t*) ext_buf;
		}
		else
		{
			htsizem = sizeof( stack_buf );
		}

		while(( htsize >> 2 ) < (size_t) srcl )
		{
			const size_t htsize2 = htsize << 1;

			if( htsize2 > htsizem )
			{
				break;
			}

			htsize = htsize2;
		}
	}

	// Initialize the hash-table. Each hash-table item consists of 2 tuples
	// (4 initial match bytes; 32-bit source data offset).

	uint32_t initv[ 2 ] = { 0, 0 };
	memcpy( initv, src, 4 );

	lzav_ht_init( ht, htsize, initv );

	const uint32_t hmask = (uint32_t) (( htsize - 1 ) ^ 15 ); // Hash mask.
	const uint8_t* ip = (const uint8_t*) src; // Source data pointer.
	const uint8_t* const ipe = ip + srcl - LZAV_LIT_FIN; // End pointer.
	const uint8_t* const ipet = ipe - 15 + LZAV_LIT_FIN; // Hashing threshold,
		// avoids I/O OOB.
	const uint8_t* ipa = ip; // Literals anchor pointer.

	uint8_t* cbp = op; // Pointer to the latest offset carry block header.
	int csh = 0; // Offset carry shift.

	intptr_t mavg = 100 << 17; // Running average of hash match rate (*2^11).
		// Two-factor average: success (0-64) by average reference length.

	while LZAV_LIKELY( ip < ipet )
	{
		// Hash source data (endianness is minimally important for compression
		// efficiency).

		uint32_t iw1;
		uint16_t iw2, ww2;
		memcpy( &iw1, ip, 4 );
		memcpy( &iw2, ip + 4, 2 );

		// Hash-table access.

		uint32_t* hp = (uint32_t*) ( ht + lzav_hash( iw1, iw2, 12, hmask ));
		uint32_t ipo = (uint32_t) ( ip - (const uint8_t*) src );
		const uint32_t hw1 = hp[ 0 ]; // Tuple 1's match word.
		size_t wpo; // At window offset.
		const uint8_t* wp; // At window pointer.
		size_t d, ml, rc, lc;

		// Find source data in hash-table tuples.

		if LZAV_LIKELY( iw1 != hw1 )
		{
			if LZAV_LIKELY( iw1 != hp[ 2 ])
			{
				goto _no_match;
			}

			wpo = hp[ 3 ];
			memcpy( &ww2, (const uint8_t*) src + wpo + 4, 2 );

			if LZAV_UNLIKELY( iw2 != ww2 )
			{
				goto _no_match;
			}
		}
		else
		{
			wpo = hp[ 1 ];
			memcpy( &ww2, (const uint8_t*) src + wpo + 4, 2 );

			if LZAV_UNLIKELY( iw2 != ww2 )
			{
				if LZAV_LIKELY( iw1 != hp[ 2 ])
				{
					goto _no_match;
				}

				wpo = hp[ 3 ];
				memcpy( &ww2, (const uint8_t*) src + wpo + 4, 2 );

				if LZAV_UNLIKELY( iw2 != ww2 )
				{
					goto _no_match;
				}
			}
		}

		// Source data and hash-table entry matched.

		d = (size_t) ipo - wpo; // Reference offset (distance).
		ml = (size_t) ( ipe - ip ); // Max reference match length. Make sure
			// `LZAV_LIT_FIN` literals remain on finish.

		if LZAV_UNLIKELY( d - LZAV_OFS_MIN > LZAV_WIN_LEN - LZAV_OFS_MIN - 1 )
		{
			// Small offsets may be inefficient (wrap over 0 for efficiency).

			goto _d_oob;
		}

		// Disallow reference copy overlap by using `d` as max match length.

		ml = ( ml > mlen ? mlen : ml );
		ml = ( ml > d ? d : ml );
		wp = (const uint8_t*) src + wpo;

		LZAV_PREFETCH( ip - 2 );

		rc = lzav_match_len( ip, wp, ml, mref );

		if LZAV_LIKELY( iw1 == hw1 ) // Replace tuple, or insert.
		{
			// Update a matching entry only if it is not an adjacent
			// replication. Otherwise, source data consisting of same-byte
			// runs won't compress well.

			if LZAV_LIKELY( d != rc )
			{
				hp[ 1 ] = ipo;
			}
		}
		else
		{
			hp[ 2 ] = hw1;
			hp[ 3 ] = hp[ 1 ];
			hp[ 0 ] = iw1;
			hp[ 1 ] = ipo;
		}

		// Update hash-table with 1 skipped position.

		memcpy( &iw1, ip + 2, 4 );
		memcpy( &iw2, ip + 6, 2 );
		hp = (uint32_t*) ( ht + lzav_hash( iw1, iw2, 12, hmask ));
		ipo += 2;

		hp[ 2 ] = iw1;
		hp[ 3 ] = ipo;

		lc = (size_t) ( ip - ipa );

		if LZAV_UNLIKELY( lc != 0 )
		{
			// Try to consume literals by finding a match at a back-position.

			ml -= rc;

			if LZAV_LIKELY( ml > lc )
			{
				ml = lc;
			}

			ml = ( ml > wpo ? wpo : ml );

			ml = lzav_match_len_r( ip, wp, ml );

			if LZAV_UNLIKELY( ml != 0 )
			{
				rc += ml;
				ip -= ml;
				lc -= ml;
			}
		}

		op = lzav_write_blk_2( op, lc, rc, d, ipa, &cbp, &csh, mref );
		ip += rc;
		ipa = ip;
		mavg += ( (intptr_t) ( rc << 17 ) - mavg ) >> 10;
		continue;

	_d_oob:
		ip++;

		if LZAV_LIKELY( d < LZAV_WIN_LEN )
		{
			continue;
		}

		hp[( iw1 != hw1 ) * 2 + 1 ] = ipo;
		continue;

	_no_match:
		wp = ip;
		hp[ 2 ] = iw1;

		mavg -= mavg >> 11;
		ip++;

		hp[ 3 ] = ipo;

		if( mavg < ( 200 << 10 ) && wp != ipa ) // Speed-up threshold.
		{
			// Compression speed-up technique that keeps the number of hash
			// evaluations around 45% of compressed data length. In some cases
			// reduces the number of blocks by several percent.

			ip += 1 + ( ipo & 1 );

			if LZAV_UNLIKELY( mavg < ( 130 << 10 ))
			{
				ip++;

				if LZAV_UNLIKELY( mavg < ( 100 << 10 ))
				{
					ip += (intptr_t) 100 - ( mavg >> 10 ); // Gradually fast.
				}
			}
		}
	}

	op = lzav_write_fin_2( op, (size_t) ( ipe - ipa + LZAV_LIT_FIN ), ipa );

	if( alloc_buf != LZAV_NULL )
	{
		LZAV_FREE( alloc_buf );
	}

	return( (int) ( op - (uint8_t*) dst ));
}

/**
 * @brief Default LZAV compression function.
 *
 * Function performs in-memory data compression using the LZAV compression
 * algorithm, with the default settings.
 *
 * See the lzav_compress() function for a more detailed description.
 *
 * @param[in] src Source (uncompressed) data pointer.
 * @param[out] dst Destination (compressed data) buffer pointer. The allocated
 * size should be at least lzav_compress_bound() bytes large.
 * @param srcl Source data length, in bytes.
 * @param dstl Destination buffer's capacity, in bytes.
 * @return The length of compressed data, in bytes. Returns 0 if `srcl` is
 * lesser or equal to 0, or if `dstl` is too small, or if not enough memory.
 */

LZAV_INLINE_F int lzav_compress_default( const void* const src,
	void* const dst, const int srcl, const int dstl ) LZAV_NOEX
{
	return( lzav_compress( src, dst, srcl, dstl, LZAV_NULL, 0 ));
}

/**
 * @brief Calculates estimated LZAV stream block's size.
 *
 * @param lc Literal count, in bytes.
 * @param d Reference offset.
 * @param csh Carry shift bit count.
 */

LZAV_INLINE_F size_t lzav_est_blksize( const size_t lc, size_t d,
	const int csh ) LZAV_NOEX
{
	const int lb = ( lc != 0 );
	d >>= csh;
	d >>= lb * 2;

	return( lc + (size_t) lb + ( lc > 15 ) + 2 +
		( d > LZAV_OFS_TH1 ) + ( d > LZAV_OFS_TH2 ));
}

/**
 * @brief Inserts a tuple into hash-table item.
 *
 * @param hp Pointer to hash-table item.
 * @param ti0 Offset of tuple 0.
 * @param mti Maximal tuple offset.
 * @param iw1 Initial source bytes.
 * @param ipo Source bytes offset.
 */

LZAV_INLINE_F void lzav_ht_insert( uint32_t* const hp, size_t ti0,
	const size_t mti, const uint32_t iw1, const uint32_t ipo ) LZAV_NOEX
{
	ti0 = ( ti0 == 0 ? mti : ti0 - 2 );
	hp[ ti0 ] = iw1;
	hp[ ti0 + 1 ] = ipo;
	hp[ mti + 3 ] = (uint32_t) ti0;
}

/**
 * @brief Higher-ratio LZAV compression function (much slower).
 *
 * Function performs in-memory data compression using the higher-ratio LZAV
 * compression algorithm.
 *
 * @param[in] src Source (uncompressed) data pointer.
 * @param[out] dst Destination (compressed data) buffer pointer. The allocated
 * size should be at least lzav_compress_bound_hi() bytes large.
 * @param srcl Source data length, in bytes.
 * @param dstl Destination buffer's capacity, in bytes.
 * @return The length of compressed data, in bytes. Returns 0 if `srcl` is
 * lesser or equal to 0, or if `dstl` is too small, or if buffer pointers are
 * invalid, or if not enough memory.
 */

LZAV_INLINE int lzav_compress_hi( const void* const src, void* const dst,
	const int srcl, const int dstl ) LZAV_NOEX
{
	if(( srcl <= 0 ) | ( src == LZAV_NULL ) | ( dst == LZAV_NULL ) |
		( src == dst ) | ( dstl < lzav_compress_bound_hi( srcl )))
	{
		return( 0 );
	}

	const size_t mref = 5; // Minimal reference length, in bytes.
	const size_t mlen = (size_t) LZAV_REF_LEN + mref;

	uint8_t* op = (uint8_t*) dst; // Destination (compressed data) pointer.
	*op = (uint8_t) ( LZAV_FMT_CUR << 4 | mref ); // Write prefix byte.
	op++;

	if( srcl < 16 )
	{
		// Handle a very short source data.

		*op = (uint8_t) srcl;
		op++;

		memcpy( op, src, (size_t) srcl );

		if( srcl > LZAV_LIT_FIN - 1 )
		{
			return( 2 + srcl );
		}

		memset( op + srcl, 0, (size_t) ( LZAV_LIT_FIN - srcl ));
		return( 2 + LZAV_LIT_FIN );
	}

	size_t htsize; // Hash-table's size in bytes (power-of-2).
	htsize = ( 1 << 7 ) * sizeof( uint32_t ) * 2 * 8;

	while( htsize != ( 1 << 23 ) && ( htsize >> 2 ) < (size_t) srcl )
	{
		htsize <<= 1;
	}

	uint8_t* const ht = (uint8_t*) LZAV_MALLOC( htsize ); // Hash-table ptr.

	if( ht == LZAV_NULL )
	{
		return( 0 );
	}

	// Initialize the hash-table. Each hash-table item consists of 8 tuples
	// (4 initial match bytes; 32-bit source data offset). The last value of
	// the last tuple is used as head tuple offset (an even value).

	uint32_t initv[ 2 ] = { 0, 0 };
	memcpy( initv, src, 4 );

	lzav_ht_init( ht, htsize, initv );

	const size_t mti = 12; // Maximal tuple offset, inclusive.
	const uint32_t hmask = (uint32_t) (( htsize - 1 ) ^ 63 ); // Hash mask.
	const uint8_t* ip = (const uint8_t*) src; // Source data pointer.
	const uint8_t* const ipe = ip + srcl - LZAV_LIT_FIN; // End pointer.
	const uint8_t* const ipet = ipe - 15 + LZAV_LIT_FIN; // Hashing threshold,
		// avoids I/O OOB.
	const uint8_t* ipa = ip; // Literals anchor pointer.

	uint8_t* cbp = op; // Pointer to the latest offset carry block header.
	int csh = 0; // Offset carry shift.

	size_t prc = 0; // Length of a previously found match.
	size_t pd = 0; // Distance of a previously found match.
	const uint8_t* pip = ip; // Source pointer of a previously found match.

	while LZAV_LIKELY( ip < ipet )
	{
		// Hash source data (endianness is minimally important for compression
		// efficiency).

		uint32_t iw1;
		memcpy( &iw1, ip, 4 );

		// Hash-table access.

		uint32_t* hp = (uint32_t*) ( ht + lzav_hash( iw1, ip[ 4 ], 8, hmask ));
		const uint32_t ipo = (uint32_t) ( ip - (const uint8_t*) src );
		size_t ti0 = hp[ mti + 3 ]; // Head tuple offset.

		// Find source data in hash-table tuples, in up to 7 previous
		// positions.

		size_t mle = (size_t) ( ipe - ip ); // Match length bound.
		mle = ( mle > mlen ? mlen : mle );

		size_t rc = 1; // Best found match length-4, 1 - not found.
		size_t d = 0; // Best found reference offset (distance).
		size_t ti = ti0;
		int i;

		// Match-finder.

		for( i = 0; i < 7; i++ )
		{
			const uint8_t* const wp0 = (const uint8_t*) src + hp[ ti + 1 ];
			const uint32_t ww1 = hp[ ti ];
			size_t d0 = (size_t) ( ip - wp0 );
			ti = ( ti == mti ? 0 : ti + 2 );

			if( iw1 == ww1 )
			{
				// Disallow reference copy overlap by using `d0` as max
				// match length. Make sure `LZAV_LIT_FIN` literals remain
				// on finish.

				size_t ml = ( d0 > mle ? mle : d0 );
				ml = lzav_match_len( ip, wp0, ml, 4 );

				if( ml > rc )
				{
					d = d0;
					rc = ml;
				}
			}
		}

		if LZAV_LIKELY( d != rc )
		{
			// Update hash-table entry, if there was no match, or if the match
			// is not an adjacent replication.

			lzav_ht_insert( hp, ti0, mti, iw1, ipo );
		}

		if(( rc < mref + ( d > LZAV_OFS_TH2 )) |
			( d - LZAV_OFS_MIN > LZAV_WIN_LEN - LZAV_OFS_MIN - 1 ))
		{
			ip++;
			continue;
		}

		// Source data and hash-table entry match of suitable length.

		const uint8_t* wp = ip - d;
		const uint8_t* const ip1 = ip + 1;
		size_t lc = (size_t) ( ip - ipa );

		if LZAV_UNLIKELY( lc != 0 )
		{
			// Try to consume literals by finding a match at back-position.

			size_t ml = ( d > mle ? mle : d );
			ml -= rc;

			const size_t wpo = (size_t) ( wp - (const uint8_t*) src );

			if LZAV_LIKELY( ml > lc )
			{
				ml = lc;
			}

			ml = ( ml > wpo ? wpo : ml );

			ml = lzav_match_len_r( ip, wp, ml );

			if LZAV_UNLIKELY( ml != 0 )
			{
				rc += ml;
				ip -= ml;
				lc -= ml;
			}
		}

		if( prc == 0 )
		{
			// Save match for a later comparison.

			prc = rc;
			pd = d;
			pip = ip;
			ip = ip1;
			continue;
		}

		// Block size overhead estimation, and comparison with a previously
		// found match.

		const size_t plc = (size_t) ( pip - ipa );
		const size_t ov = lzav_est_blksize( lc, d, csh );
		const size_t pov = lzav_est_blksize( plc, pd, csh );

		if LZAV_LIKELY( prc * ov > rc * pov )
		{
			op = lzav_write_blk_2( op, plc, prc, pd, ipa, &cbp, &csh, mref );

			ipa = pip + prc;

			if LZAV_LIKELY( ipa > ip )
			{
				prc = 0;
				ip = ( ipa > ip1 ? ipa : ip1 );
				continue;
			}

			// A winning previous match does not overlap a current match.

			prc = rc;
			pd = d;
			pip = ip;
			ip = ip1;
			continue;
		}

		op = lzav_write_blk_2( op, lc, rc, d, ipa, &cbp, &csh, mref );

		// Update hash-table with 1 skipped position.

		memcpy( &iw1, ip + 4, 4 );
		hp = (uint32_t*) ( ht + lzav_hash( iw1, ip[ 8 ], 8, hmask ));

		lzav_ht_insert( hp, hp[ mti + 3 ], mti, iw1,
			(uint32_t) ( ip + 4 - (const uint8_t*) src ));

		ip += rc;
		prc = 0;
		ipa = ip;
	}

	if( prc != 0 )
	{
		op = lzav_write_blk_2( op, (size_t) ( pip - ipa ), prc, pd, ipa, &cbp,
			&csh, mref );

		ipa = pip + prc;
	}

	op = lzav_write_fin_2( op, (size_t) ( ipe - ipa + LZAV_LIT_FIN ), ipa );

	LZAV_FREE( ht );

	return( (int) ( op - (uint8_t*) dst ));
}

/**
 * @def LZAV_LOAD32( a )
 * @brief Defines `bv` and loads 32-bit unsigned value from memory, with
 * endianness-correction.
 *
 * @param a Memory address.
 */

/**
 * @def LZAV_SET_IPD_CV( x, v, sh )
 * @brief Defines `ipd` as pointer to back-reference, checks bounds,
 * updates carry bit variables.
 *
 * @param x Reference offset.
 * @param v Next `cv` value.
 * @param sh Next `csh` value.
 */

/**
 * @def LZAV_SET_IPD( x )
 * @brief Defines `ipd` as pointer to back-reference, checks bounds,
 * resets carry bit variables.
 *
 * @param x Reference offset.
 */

/**
 * @brief Internal LZAV decompression function (stream format 2).
 *
 * Function decompresses "raw" data previously compressed into the LZAV stream
 * format 2.
 *
 * This function should not be called directly since it does not check the
 * format identifier.
 *
 * @param[in] src Source (compressed) data pointer.
 * @param[out] dst Destination (decompressed data) buffer pointer.
 * @param srcl Source data length, in bytes.
 * @param dstl Expected destination data length, in bytes.
 * @param[out] pwl Pointer to variable that receives the number of bytes
 * written to the destination buffer (until error or end of buffer).
 * @return The length of decompressed data, in bytes, or any negative value if
 * some error happened.
 */

LZAV_INLINE int lzav_decompress_2( const void* const src, void* const dst,
	const int srcl, const int dstl, int* const pwl ) LZAV_NOEX
{
	const uint8_t* ip = (const uint8_t*) src; // Compressed data pointer.
	const uint8_t* const ipe = ip + srcl; // Compressed data boundary pointer.
	const uint8_t* const ipet = ipe - 6; // Block header read threshold.
	uint8_t* op = (uint8_t*) dst; // Destination (decompressed data) pointer.
	uint8_t* const ope = op + dstl; // Destination boundary pointer.
	uint8_t* const opet = ope - 63; // Threshold for fast copy to destination.
	*pwl = dstl;
	const size_t mref1 = (size_t) ( *ip & 15 ) - 1; // Minimal ref length - 1.
	size_t bh; // Current block header, updated in each branch.
	size_t cv = 0; // Reference offset carry value.
	int csh = 0; // Reference offset carry shift.

	#define LZAV_LOAD32( a ) \
		uint32_t bv; \
		memcpy( &bv, a, 4 ); \
		LZAV_IEC32( bv )

	#define LZAV_SET_IPD_CV( x, v, sh ) \
		const size_t d = ( x ) << csh | cv; \
		csh = ( sh ); \
		const size_t md = (size_t) ( op - (uint8_t*) dst ); \
		cv = ( v ); \
		ipd = op - d; \
		if LZAV_UNLIKELY( d > md ) \
			goto _err_refoob

	#define LZAV_SET_IPD( x ) \
		LZAV_SET_IPD_CV( x, 0, 0 )

	ip++; // Advance beyond prefix byte.

	if LZAV_UNLIKELY( ip >= ipet )
	{
		goto _err_srcoob;
	}

	bh = *ip;

	while LZAV_LIKELY( ip < ipet )
	{
		const uint8_t* ipd; // Source data pointer.
		size_t cc; // Byte copy count.
		size_t bt; // Block type.

		if LZAV_LIKELY(( bh & 0x30 ) != 0 ) // Block type != 0.
		{
		_refblk:
			bt = ( bh >> 4 ) & 3;
			ip++;
			const int bt8 = (int) ( bt << 3 );

			LZAV_LOAD32( ip );
			ip += bt;

		#if defined( LZAV_X86 )

			static const uint32_t om[ 4 ] = { 0, 0xFF, 0xFFFF, 0xFFFFFF };
			static const int ocsh[ 4 ] = { 0, 0, 0, 3 };

			const uint32_t o = bv & om[ bt ];
			bv >>= bt8;

			const int wcsh = ocsh[ bt ];

			LZAV_SET_IPD_CV(( bh >> 6 | o << 2 ) & 0x7FFFFF, o >> 21, wcsh );

		#else // defined( LZAV_X86 )

			// Memory accesses on RISC are less efficient here.

			const size_t o = bv & (( (uint32_t) 1 << bt8 ) - 1 );
			bv >>= bt8;

			LZAV_SET_IPD_CV(( bh >> 6 | o << 2 ) & 0x7FFFFF, o >> 21,
				( bt == 3 ? 3 : 0 ));

		#endif // defined( LZAV_X86 )

			LZAV_PREFETCH( ipd );

			cc = bh & 15;

			if LZAV_LIKELY( cc != 0 ) // True, if no additional length byte.
			{
				cc += mref1;
				bh = bv & 0xFF;

				if LZAV_LIKELY( op < opet )
				{
					if LZAV_LIKELY( d > 15 )
					{
						memcpy( op, ipd, 16 );
						memcpy( op + 16, ipd + 16, 4 );
						op += cc;
						continue;
					}

					if LZAV_LIKELY( d > 7 )
					{
						memcpy( op, ipd, 8 );
						memcpy( op + 8, ipd + 8, 8 );
						op += cc;
						continue;
					}

					if( d > 3 )
					{
						memcpy( op, ipd, 4 );
						memcpy( op + 4, ipd + 4, 4 );
						op += cc;
						continue;
					}

					goto _err_refoob;
				}

				if LZAV_UNLIKELY( cc > d )
				{
					goto _err_refoob;
				}

				uint8_t* const opcc = op + cc;

				if LZAV_UNLIKELY( opcc > ope )
				{
					goto _err_dstoob_ref;
				}

				memcpy( op, ipd, cc );
				op = opcc;
				continue;
			}
			else
			{
				bh = bv & 0xFF;
				ip++;
				cc = 16 + mref1 + bh;

				if LZAV_UNLIKELY( bh == 255 )
				{
					cc += *ip;
					ip++;
				}

				uint8_t* const opcc = op + cc;
				bh = *ip;

				if LZAV_LIKELY(( opcc < opet ) & ( d > 15 ))
				{
					do
					{
						memcpy( op, ipd, 16 );
						memcpy( op + 16, ipd + 16, 16 );
						memcpy( op + 32, ipd + 32, 16 );
						memcpy( op + 48, ipd + 48, 16 );
						op += 64;
						ipd += 64;
					} while LZAV_LIKELY( op < opcc );

					op = opcc;
					continue;
				}

				if LZAV_UNLIKELY( cc > d )
				{
					goto _err_refoob;
				}

				if LZAV_UNLIKELY( opcc > ope )
				{
					goto _err_dstoob_ref;
				}

				memcpy( op, ipd, cc );
				op = opcc;
				continue;
			}

		_err_dstoob_ref:
			memcpy( op, ipd, (size_t) ( ope - op ));
			return( LZAV_E_DSTOOB );
		}

		size_t ncv = bh >> 6; // Additional offset carry bits.
		ip++;
		cc = bh & 15;

		if LZAV_LIKELY( cc != 0 ) // True, if no additional length byte.
		{
			ipd = ip;
			ncv <<= csh;
			ip += cc;
			csh += 2;
			cv |= ncv;

			if LZAV_LIKELY(( op < opet ) & ( ipd < ipet - 16 ))
			{
				bh = *ip;
				memcpy( op, ipd, 16 );
				op += cc;

				goto _refblk; // Reference block follows, if not EOS.
			}
		}
		else
		{
			bh = *ip;
			ncv <<= csh;
			cc = bh & 0x7F;
			csh += 2;
			cv |= ncv;
			ip++;

			if LZAV_UNLIKELY(( bh & 0x80 ) != 0 )
			{
				int sh = 7;

				do
				{
					bh = *ip;
					ip++;
					cc |= ( bh & 0x7F ) << sh;

					if( sh == 28 ) // No more than 4 additional bytes.
					{
						break;
					}

					sh += 7;

				} while(( bh & 0x80 ) != 0 );

				cc &= 0x7FFFFFFF; // For malformed data.
			}

			cc += 16;
			ipd = ip;
			ip += cc;

			uint8_t* const opcc = op + cc;

			#if defined( LZAV_PTR32 )
			if LZAV_UNLIKELY(( ip < ipd ) | ( opcc < op ))
			{
				goto _err_ptrovr;
			}
			#endif // defined( LZAV_PTR32 )

			if LZAV_LIKELY(( opcc < opet ) & ( ip < ipet - 64 ))
			{
				do
				{
					memcpy( op, ipd, 16 );
					memcpy( op + 16, ipd + 16, 16 );
					memcpy( op + 32, ipd + 32, 16 );
					memcpy( op + 48, ipd + 48, 16 );
					op += 64;
					ipd += 64;
				} while LZAV_LIKELY( op < opcc );

				bh = *ip;
				op = opcc;

				goto _refblk; // Reference block follows, if not EOS.
			}
		}

		uint8_t* const opcc = op + cc;

		if LZAV_UNLIKELY( opcc > ope )
		{
			goto _err_dstoob_lit;
		}

		if LZAV_LIKELY( ip < ipe )
		{
			bh = *ip;
			memcpy( op, ipd, cc );
			op = opcc;
			continue;
		}

		if LZAV_UNLIKELY( ip != ipe )
		{
			goto _err_srcoob_lit;
		}

		memcpy( op, ipd, cc );
		op = opcc;
		break;

	_err_srcoob_lit:
		cc = (size_t) ( ipe - ipd );

		if( cc < (size_t) ( ope - op ))
		{
			memcpy( op, ipd, cc );
			*pwl = (int) ( op + cc - (uint8_t*) dst );
		}
		else
		{
			memcpy( op, ipd, (size_t) ( ope - op ));
		}

		return( LZAV_E_SRCOOB );

	_err_dstoob_lit:
		if LZAV_UNLIKELY( ip > ipe )
		{
			goto _err_srcoob_lit;
		}

		memcpy( op, ipd, (size_t) ( ope - op ));
		return( LZAV_E_DSTOOB );
	}

	if LZAV_UNLIKELY( op != ope )
	{
		goto _err_dstlen;
	}

	return( (int) ( op - (uint8_t*) dst ));

_err_srcoob:
	*pwl = (int) ( op - (uint8_t*) dst );
	return( LZAV_E_SRCOOB );

_err_refoob:
	*pwl = (int) ( op - (uint8_t*) dst );
	return( LZAV_E_REFOOB );

_err_dstlen:
	*pwl = (int) ( op - (uint8_t*) dst );
	return( LZAV_E_DSTLEN );

#if defined( LZAV_PTR32 )
_err_ptrovr:
	*pwl = (int) ( op - (uint8_t*) dst );
	return( LZAV_E_PTROVR );
#endif // defined( LZAV_PTR32 )
}

#if LZAV_FMT_MIN < 2

/**
 * @def LZAV_LOAD16( a )
 * @brief Defines `bv` and loads 16-bit unsigned value from memory, with
 * endianness-correction.
 *
 * @param a Memory address.
 */

/**
 * @def LZAV_MEMMOVE( d, s, c )
 * @brief Stack-based `memmove` function which gets optimized into SIMD
 * instructions.
 *
 * @param d Destination address.
 * @param s Source address.
 * @param c Byte copy count (must be a constant).
 */

/**
 * @brief Internal LZAV decompression function (stream format 1).
 *
 * Function decompresses "raw" data previously compressed into the LZAV stream
 * format 1.
 *
 * This function should not be called directly since it does not check the
 * format identifier.
 *
 * @param[in] src Source (compressed) data pointer.
 * @param[out] dst Destination (decompressed data) buffer pointer.
 * @param srcl Source data length, in bytes.
 * @param dstl Expected destination data length, in bytes.
 * @return The length of decompressed data, in bytes, or any negative value if
 * some error happened.
 */

LZAV_INLINE int lzav_decompress_1( const void* const src, void* const dst,
	const int srcl, const int dstl ) LZAV_NOEX
{
	const uint8_t* ip = (const uint8_t*) src; // Compressed data pointer.
	const uint8_t* const ipe = ip + srcl; // Compressed data boundary pointer.
	const uint8_t* const ipet = ipe - 5; // Block header read threshold.
	uint8_t* op = (uint8_t*) dst; // Destination (decompressed data) pointer.
	uint8_t* const ope = op + dstl; // Destination boundary pointer.
	uint8_t* const opet = ope - 63; // Threshold for fast copy to destination.
	const size_t mref1 = (size_t) ( *ip & 15 ) - 1; // Minimal ref length - 1.
	size_t bh = 0; // Current block header, updated in each branch.
	size_t cv = 0; // Reference offset carry value.
	int csh = 0; // Reference offset carry shift.

	#if LZAV_LITTLE_ENDIAN
		#define LZAV_LOAD16( a ) \
			uint16_t bv; \
			memcpy( &bv, a, 2 )
	#else // LZAV_LITTLE_ENDIAN
		#define LZAV_LOAD16( a ) \
			uint16_t bv = (uint16_t) ( *( a ) | *( a + 1 ) << 8 )
	#endif // LZAV_LITTLE_ENDIAN

	#define LZAV_MEMMOVE( d, s, c ) \
		{ uint8_t tmp[ c ]; memcpy( tmp, s, c ); memcpy( d, tmp, c ); } (void) 0

	ip++; // Advance beyond prefix byte.

	if LZAV_UNLIKELY( ip >= ipet )
	{
		goto _err_srcoob;
	}

	bh = *ip;

	while LZAV_LIKELY( ip < ipet )
	{
		const uint8_t* ipd; // Source data pointer.
		size_t cc; // Byte copy count.

		if LZAV_UNLIKELY(( bh & 0x30 ) == 0 ) // Block type 0.
		{
			cv = bh >> 6;
			csh = 2;
			ip++;
			cc = bh & 15;

			if LZAV_LIKELY( cc != 0 ) // True, if no additional length byte.
			{
				ipd = ip;
				ip += cc;

				if LZAV_LIKELY(( op < opet ) & ( ipd < ipe - 15 - 6 ))
				{
					bh = *ip;
					memcpy( op, ipd, 16 );
					op += cc;
					goto _refblk; // Reference block follows, if not EOS.
				}
			}
			else
			{
				LZAV_LOAD16( ip );

				const size_t l2 = (size_t) ( bv & 0xFF );
				cc = 16;
				ip++;
				const int lb = ( l2 == 255 );
				cc += l2 + (( bv >> 8 ) & ( 0x100 - lb ));
				ip += lb;

				ipd = ip;
				ip += cc;

				if LZAV_LIKELY(( op < opet ) & ( ipd < ipe - 63 - 1 ))
				{
					memcpy( op, ipd, 16 );
					memcpy( op + 16, ipd + 16, 16 );
					memcpy( op + 32, ipd + 32, 16 );
					memcpy( op + 48, ipd + 48, 16 );

					if LZAV_LIKELY( cc < 65 )
					{
						bh = *ip;
						op += cc;
						continue;
					}

					ipd += 64;
					op += 64;
					cc -= 64;
				}
			}

			if LZAV_LIKELY( ip < ipe )
			{
				bh = *ip;
			}
			else
			if LZAV_UNLIKELY( ip != ipe )
			{
				goto _err_srcoob;
			}

			if LZAV_UNLIKELY( op + cc > ope )
			{
				goto _err_dstoob;
			}

			// This and other alike copy-blocks are transformed into fast SIMD
			// instructions, by a modern compiler. Direct use of `memcpy` is
			// slower due to shortness of data remaining to copy, on average.

			while( cc != 0 )
			{
				*op = *ipd;
				ipd++;
				op++;
				cc--;
			}

			continue;
		}

	_refblk:
		cc = bh & 15;

		if LZAV_UNLIKELY(( bh & 32 ) == 0 ) // True, if block type 1.
		{
			LZAV_SET_IPD( bh >> 6 | (size_t) ip[ 1 ] << 2 );
			ip += 2;
			bh = *ip;
		}
		else // Block type 2 or 3.
		{
			if LZAV_LIKELY(( bh & 16 ) == 0 ) // True, if block type 2.
			{
				LZAV_LOAD16( ip + 1 );
				LZAV_SET_IPD( bh >> 6 | (size_t) bv << 2 );
				ip += 3;
				bh = *ip;
			}
			else // Block type 3.
			{
				LZAV_LOAD32( ip + 1 );
				LZAV_SET_IPD_CV( bv & 0xFFFFFF, bh >> 6, 2 );
				ip += 4;
				bh = bv >> 24;
			}
		}

		if LZAV_LIKELY( cc != 0 ) // True, if no additional length byte.
		{
			cc += mref1;

			if LZAV_LIKELY( op < opet )
			{
				LZAV_MEMMOVE( op, ipd, 16 );
				LZAV_MEMMOVE( op + 16, ipd + 16, 4 );

				op += cc;
				continue;
			}
		}
		else
		{
			cc = 16 + mref1 + bh;
			ip++;
			bh = *ip;

			if LZAV_LIKELY( op < opet )
			{
				LZAV_MEMMOVE( op, ipd, 16 );
				LZAV_MEMMOVE( op + 16, ipd + 16, 16 );
				LZAV_MEMMOVE( op + 32, ipd + 32, 16 );
				LZAV_MEMMOVE( op + 48, ipd + 48, 16 );

				if LZAV_LIKELY( cc < 65 )
				{
					op += cc;
					continue;
				}

				ipd += 64;
				op += 64;
				cc -= 64;
			}
		}

		if LZAV_UNLIKELY( op + cc > ope )
		{
			goto _err_dstoob;
		}

		while( cc != 0 )
		{
			*op = *ipd;
			ipd++;
			op++;
			cc--;
		}
	}

	if LZAV_UNLIKELY( op != ope )
	{
		goto _err_dstlen;
	}

	return( (int) ( op - (uint8_t*) dst ));

_err_srcoob:
	return( LZAV_E_SRCOOB );

_err_dstoob:
	return( LZAV_E_DSTOOB );

_err_refoob:
	return( LZAV_E_REFOOB );

_err_dstlen:
	return( LZAV_E_DSTLEN );
}

#undef LZAV_LOAD16
#undef LZAV_MEMMOVE

#endif // LZAV_FMT_MIN < 2

#undef LZAV_LOAD32
#undef LZAV_SET_IPD_CV
#undef LZAV_SET_IPD

/**
 * @brief LZAV decompression function (partial).
 *
 * Function decompresses "raw" data previously compressed into the LZAV stream
 * format, for partial or recovery decompression. For example, this function
 * can be used to decompress only an initial segment of a larger data block.
 *
 * @param[in] src Source (compressed) data pointer, can be 0 if `srcl` is 0.
 * Address alignment is unimportant.
 * @param[out] dst Destination (decompressed data) buffer pointer. Address
 * alignment is unimportant. Should be different to `src`.
 * @param srcl Source data length, in bytes, can be 0.
 * @param dstl Destination buffer length, in bytes, can be 0.
 * @return The length of decompressed data, in bytes. Always a non-negative
 * value (error codes are not returned).
 */

LZAV_INLINE_F int lzav_decompress_partial( const void* const src,
	void* const dst, const int srcl, const int dstl ) LZAV_NOEX
{
	if( srcl <= 0 || src == LZAV_NULL || dst == LZAV_NULL || src == dst ||
		dstl <= 0 )
	{
		return( 0 );
	}

	const int fmt = *(const uint8_t*) src >> 4;
	int dl = 0;

	if( fmt == 2 )
	{
		lzav_decompress_2( src, dst, srcl, dstl, &dl );
	}

	return( dl );
}

/**
 * @brief LZAV decompression function.
 *
 * Function decompresses "raw" data previously compressed into the LZAV stream
 * format.
 *
 * Note that while the function does perform checks to avoid OOB memory
 * accesses, and checks for decompressed data length equality, this is not a
 * strict guarantee of a valid decompression. In cases when the compressed
 * data is stored in a long-term storage without embedded data integrity
 * mechanisms (e.g., a database without RAID 1 guarantee, a binary container
 * without a digital signature nor CRC), then a checksum (hash) of the
 * original uncompressed data should be stored, and then evaluated against
 * that of the decompressed data. Also, a separate checksum (hash) of
 * application-defined header, which contains uncompressed and compressed data
 * lengths, should be checked before decompression. A high-performance
 * "komihash" hash function can be used to obtain a hash value of the data.
 *
 * @param[in] src Source (compressed) data pointer, can be 0 if `srcl` is 0.
 * Address alignment is unimportant.
 * @param[out] dst Destination (decompressed data) buffer pointer. Address
 * alignment is unimportant. Should be different to `src`.
 * @param srcl Source data length, in bytes, can be 0.
 * @param dstl Expected destination data length, in bytes, can be 0. Should
 * not be confused with the actual size of the destination buffer (which may
 * be larger).
 * @return The length of decompressed data, in bytes, or any negative value if
 * some error happened. Always returns a negative value if the resulting
 * decompressed data length differs from `dstl`. This means that error result
 * handling requires just a check for a negative return value (see the
 * LZAV_ERROR enum for possible values).
 */

LZAV_INLINE_F int lzav_decompress( const void* const src, void* const dst,
	const int srcl, const int dstl ) LZAV_NOEX
{
	if( srcl < 0 )
	{
		return( LZAV_E_PARAMS );
	}

	if( srcl == 0 )
	{
		return( dstl == 0 ? 0 : LZAV_E_PARAMS );
	}

	if( src == LZAV_NULL || dst == LZAV_NULL || src == dst || dstl <= 0 )
	{
		return( LZAV_E_PARAMS );
	}

	const int fmt = *(const uint8_t*) src >> 4;

	if( fmt == 2 )
	{
		int tmp;
		return( lzav_decompress_2( src, dst, srcl, dstl, &tmp ));
	}

#if LZAV_FMT_MIN < 2
	if( fmt == 1 )
	{
		return( lzav_decompress_1( src, dst, srcl, dstl ));
	}
#endif // LZAV_FMT_MIN < 2

	return( LZAV_E_UNKFMT );
}

#if defined( LZAV_NS )

} // namespace LZAV_NS

#if !defined( LZAV_NS_CUSTOM )

namespace {

using namespace LZAV_NS :: enum_wrapper;
using LZAV_NS :: lzav_compress_bound;
using LZAV_NS :: lzav_compress_bound_hi;
using LZAV_NS :: lzav_compress;
using LZAV_NS :: lzav_compress_default;
using LZAV_NS :: lzav_compress_hi;
using LZAV_NS :: lzav_decompress_partial;
using LZAV_NS :: lzav_decompress;

} // namespace

#endif // !defined( LZAV_NS_CUSTOM )

#endif // defined( LZAV_NS )

// Defines for Doxygen.

#if !defined( LZAV_NS_CUSTOM )
	#define LZAV_NS_CUSTOM
#endif // !defined( LZAV_NS_CUSTOM )

#undef LZAV_NS_CUSTOM
#undef LZAV_NOEX
#undef LZAV_NULL
#undef LZAV_MALLOC
#undef LZAV_FREE
#undef LZAV_X86
#undef LZAV_COND_EC
#undef LZAV_GCC_BUILTINS
#undef LZAV_CPP_BIT
#undef LZAV_IEC32
#undef LZAV_LIKELY
#undef LZAV_UNLIKELY
#undef LZAV_PREFETCH
#undef LZAV_INLINE
#undef LZAV_INLINE_F

#endif // LZAV_INCLUDED

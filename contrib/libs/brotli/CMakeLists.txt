# Decoder-only (c/common + c/dec). Prefer system libbrotlidec via Findbrotli.cmake.
set(SRCS
	common/constants.c
	common/context.c
	common/dictionary.c
	common/platform.c
	common/shared_dictionary.c
	common/transform.c
	dec/bit_reader.c
	dec/decode.c
	dec/huffman.c
	dec/prefix.c
	dec/state.c
	dec/static_init.c
)

set(LIB brotli)
engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
)

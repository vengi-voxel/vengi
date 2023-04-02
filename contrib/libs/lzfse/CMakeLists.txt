set(SRCS
	lzfse_decode.c
	lzfse_decode_base.c
	lzfse_encode.c
	lzfse_encode_base.c
	lzfse_fse.c
	lzvn_decode_base.c
	lzvn_encode_base.c
)

set(LIB lzfse)
engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
)

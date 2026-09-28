# Decoder + encoder (c/common + c/dec + c/enc). Prefer system libbrotli via Findbrotli.cmake.
# Skip enc/static_init_lazy.cc - static_init.c covers BROTLI_STATIC_INIT_EARLY.
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
	enc/backward_references.c
	enc/backward_references_hq.c
	enc/bit_cost.c
	enc/block_splitter.c
	enc/brotli_bit_stream.c
	enc/cluster.c
	enc/command.c
	enc/compound_dictionary.c
	enc/compress_fragment.c
	enc/compress_fragment_two_pass.c
	enc/dictionary_hash.c
	enc/encode.c
	enc/encoder_dict.c
	enc/entropy_encode.c
	enc/fast_log.c
	enc/histogram.c
	enc/literal_cost.c
	enc/memory.c
	enc/metablock.c
	enc/static_dict.c
	enc/static_dict_lut.c
	enc/static_init.c
	enc/utf8_util.c
)

set(LIB brotli)
engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
)
# Encoder .c files each define file-local Hash, IsMatch, EmitInsertLen, ...
# Unity builds concatenate them into one TU and fail (emscripten CI).
get_target_property(_brotli_type ${LIB} TYPE)
if (_brotli_type STREQUAL "STATIC_LIBRARY")
	set_target_properties(${LIB} PROPERTIES UNITY_BUILD OFF)
endif()

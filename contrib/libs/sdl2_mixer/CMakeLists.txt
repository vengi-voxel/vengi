set(SRCS
	compat.c
	effect_position.c
	effects_internal.c
	effect_stereoreverse.c
	mixer.c
	music.c
	codecs/load_aiff.c
	codecs/load_voc.c
	codecs/music_cmd.c
	codecs/music_ogg.c
	codecs/music_wav.c
	libogg-1.3.2/src/framing.c
	libogg-1.3.2/src/bitwise.c
	libvorbis-1.3.6/lib/mapping0.c
	libvorbis-1.3.6/lib/synthesis.c
	libvorbis-1.3.6/lib/mdct.c
	libvorbis-1.3.6/lib/lpc.c
	libvorbis-1.3.6/lib/lookup.c
	libvorbis-1.3.6/lib/info.c
	libvorbis-1.3.6/lib/block.c
	libvorbis-1.3.6/lib/lsp.c
	libvorbis-1.3.6/lib/vorbisfile.c
	libvorbis-1.3.6/lib/envelope.c
	libvorbis-1.3.6/lib/registry.c
	libvorbis-1.3.6/lib/smallft.c
	libvorbis-1.3.6/lib/bitrate.c
	libvorbis-1.3.6/lib/sharedbook.c
	libvorbis-1.3.6/lib/window.c
	libvorbis-1.3.6/lib/res0.c
	libvorbis-1.3.6/lib/codebook.c
	libvorbis-1.3.6/lib/vorbisenc.c
	libvorbis-1.3.6/lib/psy.c
	libvorbis-1.3.6/lib/analysis.c
	libvorbis-1.3.6/lib/floor0.c
	libvorbis-1.3.6/lib/floor1.c
)
set(LIB sdl2_mixer)

engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
	DEFINES MUSIC_OGG MUSIC_WAV)

if (NOT SDL2_MIXER_FOUND)
	if (MSVC)
		target_compile_options(${LIB} PRIVATE /wd4244)
	endif()
	target_include_directories(${LIB} PRIVATE "codecs")
	target_include_directories(${LIB} PRIVATE "libogg-1.3.2/include")
	target_include_directories(${LIB} PRIVATE "libvorbis-1.3.6/include")
	target_include_directories(${LIB} PRIVATE "libvorbis-1.3.6/lib")
	target_link_libraries(${LIB} sdl2)
	if (USE_CLANG)
		target_compile_options(${LIB} PRIVATE -Wno-implicit-fallthrough -Wno-cast-qual -Wno-shadow -Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable -Wno-missing-field-initializers -Wno-unused-label -Wno-format-zero-length)
	endif()
	if (USE_GCC)
		target_compile_options(${LIB} PRIVATE -Wno-discarded-qualifiers -Wno-misleading-indentation -Wno-implicit-fallthrough -Wno-cast-qual -Wno-shadow -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable -Wno-missing-field-initializers -Wno-unused-label -Wno-format-zero-length)
	endif()
endif()

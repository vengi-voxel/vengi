set(SRCS
	effect_position.c
	effects_internal.c
	effect_stereoreverse.c
	load_aiff.c
	load_voc.c
	mixer.c
	music.c
	music_cmd.c
	music_ogg.c
	music_wav.c
	libogg-1.3.1/src/framing.c
	libogg-1.3.1/src/bitwise.c
	libvorbis-1.3.3/lib/mapping0.c
	libvorbis-1.3.3/lib/synthesis.c
	libvorbis-1.3.3/lib/mdct.c
	libvorbis-1.3.3/lib/lpc.c
	libvorbis-1.3.3/lib/lookup.c
	libvorbis-1.3.3/lib/info.c
	libvorbis-1.3.3/lib/block.c
	libvorbis-1.3.3/lib/lsp.c
	libvorbis-1.3.3/lib/vorbisfile.c
	libvorbis-1.3.3/lib/envelope.c
	libvorbis-1.3.3/lib/registry.c
	libvorbis-1.3.3/lib/smallft.c
	libvorbis-1.3.3/lib/bitrate.c
	libvorbis-1.3.3/lib/sharedbook.c
	libvorbis-1.3.3/lib/window.c
	libvorbis-1.3.3/lib/res0.c
	libvorbis-1.3.3/lib/codebook.c
	libvorbis-1.3.3/lib/vorbisenc.c
	libvorbis-1.3.3/lib/psy.c
	libvorbis-1.3.3/lib/analysis.c
	libvorbis-1.3.3/lib/floor0.c
	libvorbis-1.3.3/lib/floor1.c
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
	target_include_directories(${LIB} PRIVATE "libogg-1.3.1/include")
	target_include_directories(${LIB} PRIVATE "libvorbis-1.3.3/include")
	target_include_directories(${LIB} PRIVATE "libvorbis-1.3.3/lib")
	target_link_libraries(${LIB} sdl2)
	if (USE_CLANG)
		target_compile_options(${LIB} PRIVATE -Wno-implicit-fallthrough -Wno-cast-qual -Wno-shadow -Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable -Wno-missing-field-initializers -Wno-unused-label -Wno-format-zero-length)
	endif()
	if (USE_GCC)
		target_compile_options(${LIB} PRIVATE -Wno-discarded-qualifiers -Wno-misleading-indentation -Wno-implicit-fallthrough -Wno-cast-qual -Wno-shadow -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-sign-compare -Wno-unused-variable -Wno-missing-field-initializers -Wno-unused-label -Wno-format-zero-length)
	endif()
endif()

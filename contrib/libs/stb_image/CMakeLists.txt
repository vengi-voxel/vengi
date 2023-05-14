set(SRCS
	image_DXT.c
	image_DXT.h
	image_helper.c
	image_helper.h
	pkm_helper.h
	pvr_helper.h
	stb_image_write.h
	stb_image.h
	stbi_DDS_c.h
	stbi_DDS.h
	stbi_ext_c.h
	stbi_ext.h
	stbi_pkm_c.h
	stbi_pkm.h
	stbi_pvr_c.h
	stbi_pvr.h
	stbi_qoi_c.h
	stbi_qoi_write.h
	stbi_qoi.h
	wfETC.c
	wfETC.h
)
add_library(stb_image STATIC ${SRCS})
target_include_directories(stb_image PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

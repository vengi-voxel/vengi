set(SRCS
	ClientEntity.h ClientEntity.cpp
	ClientEntityId.h
	Movement.h Movement.cpp
)

set(LIB frontend)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES network attrib)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

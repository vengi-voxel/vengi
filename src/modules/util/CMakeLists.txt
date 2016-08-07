set(SRCS
	IProgressMonitor.h
	PosLerp.h
	Dummy.cpp
)
set(LIB util)
add_library(${LIB} ${SRCS})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

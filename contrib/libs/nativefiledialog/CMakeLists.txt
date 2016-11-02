set(LIB nativefiledialog)
set(SRCS nfd_common.c nfd.h)

if (APPLE)
	set(SRCS ${SRCS} nfd_cocoa.mm)
endif( APPLE)

if (MSVC)
	set(SRCS ${SRCS} nfd_win.cpp)
endif( MSVC)

if (LINUX)
	set(SRCS ${SRCS} nfd_gtk.c)
endif(LINUX)

engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
)

if (LINUX)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
	target_include_directories(${LIB} PRIVATE ${GTK3_INCLUDE_DIRS})
	target_link_libraries(${LIB} ${GTK3_LIBRARIES})
	link_directories(${GTK3_LIBRARY_DIRS})
	add_definitions(${GTK3_CFLAGS_OTHER})
endif(LINUX)

if (APPLE)
	target_link_libraries(${LIB} AppKit)
endif(APPLE)

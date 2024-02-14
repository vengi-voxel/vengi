set(SRCS
	dictionary.cpp
	dictionary.hpp
	dictionary_manager.cpp
	dictionary_manager.hpp
	file_system.hpp
	iconv.cpp
	iconv.hpp
	language.cpp
	language.hpp
	log.cpp
	log.hpp
	log_stream.hpp
	plural_forms.cpp
	plural_forms.hpp
	po_parser.cpp
	po_parser.hpp
	tinygettext.cpp
	tinygettext.hpp
	unix_file_system.cpp
	unix_file_system.hpp
)
engine_add_library(LIB tinygettext SRCS ${SRCS})
target_include_directories(tinygettext PRIVATE ..)
target_compile_definitions(tinygettext PRIVATE -DTINYGETTEXT_WITH_SDL)
target_link_libraries(tinygettext PRIVATE sdl2)

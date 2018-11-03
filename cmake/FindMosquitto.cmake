if (NOT MOSQUITTO_INCLUDE_DIR)
	find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
endif()

if (NOT MOSQUITTO_LIBRARY)
	find_library(MOSQUITTO_LIBRARY NAMES mosquitto)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(MOSQUITTO DEFAULT_MSG MOSQUITTO_LIBRARY MOSQUITTO_INCLUDE_DIR)

message(STATUS "mosquitto library: ${MOSQUITTO_LIBRARY}")
message(STATUS "mosquitto include dir: ${MOSQUITTO_INCLUDE_DIR}")
set(MOSQUITTO_LIBRARIES ${MOSQUITTO_LIBRARY})
mark_as_advanced(MOSQUITTO_INCLUDE_DIR MOSQUITTO_LIBRARY)

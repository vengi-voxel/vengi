set(DOCKER_BUILD_ARGS "--pull" CACHE STRING "Docker cli arguments for building an image")
set(DOCKER_RUN_ARGS "-it" CACHE STRING "Docker cli arguments for running an image")
set(DOCKER_DELETE_IUMAGE_ARGS "" CACHE STRING "Docker cli arguments for deleting an image")
set(DOCKER_PUSH_ARGS "" CACHE STRING "Docker cli arguments for pushing an image")

#
# Adds a docker target to a project if a Dockerfile (copied) or Dockerfile.in (template) is in the directory
# The working directory is the root dir of the project.
#
# The DOCKER_IMAGE_NAME_TAG variable that is set here, must be in sync with the kubernetes manifests
#
macro(engine_docker NAME)
	set(DOCKERFILE_SRC)
	set(DOCKERFILE_TARGET ${CMAKE_CURRENT_BINARY_DIR}/Dockerfile)
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile.in)
		set(DOCKERFILE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile.in)
		configure_file(${DOCKERFILE_SRC} ${DOCKERFILE_TARGET} @ONLY)
	elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile)
		set(DOCKERFILE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile)
		configure_file(${DOCKERFILE_SRC} ${DOCKERFILE_TARGET} COPYONLY)
	endif()

	if (DOCKER_REGISTRY)
		set(DOCKER_IMAGE_NAME_TAG "${DOCKER_REGISTRY}/${NAME}:${ROOT_PROJECT_VERSION}")
	else()
		set(DOCKER_IMAGE_NAME_TAG "${NAME}:${ROOT_PROJECT_VERSION}")
	endif()

	if (DOCKERFILE_SRC AND DOCKER_EXECUTABLE)
		add_custom_target(${NAME}-docker
			COMMAND
				${DOCKER_EXECUTABLE}
				build
				${DOCKER_BUILD_ARGS}
				-t ${DOCKER_IMAGE_NAME_TAG}
				.
				-f - < ${DOCKERFILE_TARGET}
			DEPENDS ${DOCKERFILE_TARGET}
			SOURCES ${DOCKERFILE_SRC}
			COMMENT "Docker image name of ${NAME} is ${DOCKER_IMAGE_NAME_TAG}"
			VERBATIM
			WORKING_DIRECTORY "${ROOT_DIR}"
		)

		add_custom_target(${NAME}-docker-run
			COMMAND
				${DOCKER_EXECUTABLE}
				run
				${DOCKER_RUN_ARGS}
				${DOCKER_IMAGE_NAME_TAG}
			VERBATIM
			DEPENDS ${NAME}-docker
		)

		add_custom_target(${NAME}-docker-push
			COMMAND
				${DOCKER_EXECUTABLE}
				push
				${DOCKER_PUSH_ARGS}
				${DOCKER_IMAGE_NAME_TAG}
			COMMAND
				${DOCKER_EXECUTABLE}
				rmi
				${DOCKER_DELETE_IUMAGE_ARGS}
				${DOCKER_IMAGE_NAME_TAG}
			VERBATIM
		)
	endif()
endmacro()

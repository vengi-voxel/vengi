#
# Replace variables in the templates to create openshift/kubernetes manifests
# Variables:
# * DOCKER_IMAGE_NAME_TAG: The name of the docker image that is created or used in the deployment
# * FQDN: The domain name where all the services are running with
# * PROJECT_NAME: The project() variable of the service to create the manifests for
#
# There can be other variables of course that are service dependent.
#
# The DOCKER_IMAGE_NAME_TAG variable that is set here, must be in sync with the docker target
#
macro(engine_manifests NAME)
	set(DOCKER_IMAGE_NAME_TAG "${NAME}:${ROOT_PROJECT_VERSION}")
	foreach (FILEPATH ${ARGN})
		get_filename_component(FILENAME ${FILEPATH} NAME_WE)
		configure_file(${FILEPATH} ${CMAKE_CURRENT_BINARY_DIR}/deployment/${FILENAME}.yaml)
	endforeach()
endmacro()

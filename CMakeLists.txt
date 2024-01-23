
# This way of making shared library (not even including static one) must be replaced in the future

cmake_minimum_required(VERSION 3.28.1)

project(
	RPGE
	VERSION 	1.0.0
	DESCRIPTION "Raycaster Plus Game Engine"
	LANGUAGES 	CXX
)

##############################
###### SET UP VARIABLES ######
##############################

set(
	RPGE_SOURCES
	${CMAKE_SOURCE_DIR}/source/RPGE_camera.cpp
	${CMAKE_SOURCE_DIR}/source/RPGE_engine.cpp
	${CMAKE_SOURCE_DIR}/source/RPGE_math.cpp
	${CMAKE_SOURCE_DIR}/source/RPGE_texture.cpp
	${CMAKE_SOURCE_DIR}/source/RPGE_dda.cpp
	${CMAKE_SOURCE_DIR}/source/RPGE_globals.cpp
	${CMAKE_SOURCE_DIR}/source/RPGE_scene.cpp
)
set(RPGE_SHARED ${CMAKE_PROJECT_NAME}-shared)

###################################
###### CREATE SHARED LIBRARY ######
###################################

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)
file(COPY ${CMAKE_SOURCE_DIR}/include DESTINATION ${CMAKE_BINARY_DIR})

add_library(${RPGE_SHARED} SHARED ${RPGE_SOURCES})
set_target_properties(
	${RPGE_SHARED} PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
	OUTPUT_NAME ${CMAKE_PROJECT_NAME}
)
target_include_directories(${RPGE_SHARED} PUBLIC ${CMAKE_BINARY_DIR}/include)

install(TARGETS ${RPGE_STATIC} ${RPGE_SHARED} DESTINATION /usr/lib)

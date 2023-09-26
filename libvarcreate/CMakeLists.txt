cmake_minimum_required(VERSION 3.10)

project(varcreate
	VERSION ${VARCREATE_VERSION}
	DESCRIPTION "Variable Creation Interface Library"
)

include(GNUInstallDirs)

add_library( ${PROJECT_NAME} SHARED
	src/libvarcreate.c
	src/cJSON.c
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION 1
)

set(VARCREATE_HEADERS inc/varcreate/libvarcreate.h)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${VARCREATE_HEADERS}")

target_compile_options( ${PROJECT_NAME}
	PRIVATE
	-Wall
	-Wextra
	-Wpedantic
	-Werror
)

target_include_directories( ${PROJECT_NAME}
	PRIVATE
		.
	PUBLIC
		inc
)

target_link_libraries( ${PROJECT_NAME}
	PRIVATE
		varserver
)

install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/varcreate)


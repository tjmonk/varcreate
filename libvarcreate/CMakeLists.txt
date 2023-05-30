cmake_minimum_required(VERSION 3.10)

project(varcreate
	VERSION 0.1
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

set(VARCREATE_HEADERS inc/libvarcreate.h)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${VARCREATE_HEADERS}")

target_include_directories( ${PROJECT_NAME} PRIVATE . )

target_include_directories( ${PROJECT_NAME} PUBLIC inc )

install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/varcreate)


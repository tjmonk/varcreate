cmake_minimum_required(VERSION 3.10)

include(GNUInstallDirs)

project(varcreate-bin
	VERSION ${VARCREATE_VERSION}
	DESCRIPTION "Variable Creation Utility"
)

find_package( Threads REQUIRED )

add_executable( ${PROJECT_NAME}
	src/varcreate.c
)

target_include_directories( ${PROJECT_NAME}
	PRIVATE inc
)

target_link_libraries( ${PROJECT_NAME}
	varserver
	varcreate
)

set_target_properties( ${PROJECT_NAME}
		       PROPERTIES OUTPUT_NAME varcreate
)

target_compile_options( ${PROJECT_NAME}
	PRIVATE
	-Wall
	-Wextra
	-Wpedantic
	-Werror
)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

project(FreqResp)
cmake_minimum_required(VERSION 2.8)

include(GNUInstallDirs)

set(SRC_LIST
	analogdiscovery.cpp
	gpio.cpp
	measurement.cpp
	main.cpp
	default.h
	specialkeyboard.cpp
	tests.cpp
	debug.cpp
	encoder.cpp
	types.cpp
	volume.cpp
	speaker.cpp
	descriptiveexception.cpp
)
set(GPIOCTLD_SRC_LIST
	analogdiscovery.cpp
	descriptiveexception.cpp
	gpio.cpp
	gpioctld.cpp
	debug.cpp
	measurement.cpp
	types.cpp
)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=c++14)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

if(NOT DEFINED SYSTEMD_CONFIGURATION_FILES_DIR)
  set(SYSTEMD_CONFIGURATION_FILES_DIR "lib/systemd/system")
endif()


link_directories(/usr/lib64/)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} dwf)
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} boost_program_options)

add_executable(gpioctld ${GPIOCTLD_SRC_LIST})
target_link_libraries(gpioctld dwf)
target_link_libraries(gpioctld pthread)

configure_file(${CMAKE_SOURCE_DIR}/systemd/gpioctld.service.in
               ${PROJECT_BINARY_DIR}/systemd/gpioctld.service @ONLY)

INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION sbin
)
INSTALL(TARGETS gpioctld
	RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
)
INSTALL(PROGRAMS gpioctl
	DESTINATION bin
)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/systemd/gpioctld.socket ${PROJECT_BINARY_DIR}/systemd/gpioctld.service
        DESTINATION ${SYSTEMD_CONFIGURATION_FILES_DIR})

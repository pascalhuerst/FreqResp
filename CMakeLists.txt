project(FreqResp)
cmake_minimum_required(VERSION 2.8)

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
)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=c++14)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

link_directories(/usr/lib64/)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} dwf)
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} boost_program_options)

INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION sbin 
)

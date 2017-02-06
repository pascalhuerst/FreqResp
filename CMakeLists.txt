project(FreqResp)
cmake_minimum_required(VERSION 2.8)

set(SRC_LIST analogdiscovery.cpp gpio.cpp measurement.cpp main.cpp)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=c++11)
endif()

link_directories(/usr/lib64/)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} dwf)
target_link_libraries(${PROJECT_NAME} pthread)

INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION sbin 
)

cmake_minimum_required(VERSION 3.16)
project(screen_capture_example_opencv)

find_package(screen_capture_lite_shared REQUIRED)

find_package(OpenCV REQUIRED videoio highgui)

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()


set(${PROJECT_NAME}_PLATFORM_LIBS ${screen_capture_lite_shared_LIBS} ${OpenCV_LIBS})

include_directories(
	../include 
	${screen_capture_lite_shared_INCLUDE_DIRS}
)

option(BUILD_SHARED_LIBS OFF) 
option(USE_MSVC_RUNTIME_LIBRARY_DLL OFF) 

if (MSVC OR CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(${PROJECT_NAME}
	Screen_Capture_Example_OpenCV.cpp
)

message(STATUS "screen_capture_lite_LIBS=${screen_capture_lite_shared_LIBS}")

target_link_libraries(${PROJECT_NAME} ${${PROJECT_NAME}_PLATFORM_LIBS})  

install (TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION Examples
)

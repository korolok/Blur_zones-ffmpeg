# cmake_minimum_required(VERSION 3.8)
# project(blur)

# set(TARGET_NAME "${PROJECT_NAME}")
# set(CMAKE_CXX_STANDARD 20)
# set(INCLUDE_CV /usr/include/opencv4)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# find_package(OpenCV REQUIRED)
# find_package(PkgConfig REQUIRED)

# pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
#     libavdevice
#     libavfilter
#     libavformat
#     libavcodec
#     libswresample
#     libswscale
#     libavutil
# )

# add_executable(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/blur.cpp)

# include_directories( ${INCLUDE_CV} )
# target_link_libraries(${TARGET_NAME} PkgConfig::LIBAV)
# target_link_libraries(${TARGET_NAME} ${OpenCV_LIBS})\


cmake_minimum_required(VERSION 3.8)
project(blur)

set(TARGET_NAME "${PROJECT_NAME}")
set(TARGET_OBJS "blur-objs")
set(CMAKE_CXX_STANDARD 20)
set(INCLUDE_CV /usr/include/opencv4)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

file(GLOB_RECURSE SOURCE_CORE
${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
${CMAKE_CURRENT_SOURCE_DIR}/src/*.h )

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

add_library(${TARGET_OBJS} STATIC ${SOURCE_CORE})
add_executable(${TARGET_NAME} src/blur.cpp)

include_directories( ${INCLUDE_DIR} )
include_directories( ${INCLUDE_CV} )
include_directories( ${INCLUDE_LIB} )

target_link_libraries(${TARGET_OBJS} PkgConfig::LIBAV)
target_link_libraries(${TARGET_NAME} ${OpenCV_LIBS})
target_link_libraries(${TARGET_NAME} ${TARGET_OBJS})
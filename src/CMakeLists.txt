#
# handmade hero
#

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

add_definitions(-DWIN32_LEAN_AND_MEAN)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(GAME_SRCS ${GAME_SRCS} platform.h)
set(GAME_SRCS ${GAME_SRCS} memory.h)
set(GAME_SRCS ${GAME_SRCS} asset.h asset.cpp)
set(GAME_SRCS ${GAME_SRCS} handmade.h handmade.cpp)

add_library(handmade SHARED ${GAME_SRCS})

set(PLATFORM_SRCS ${PLATFORM_SRCS} platform.h)
set(PLATFORM_SRCS ${PLATFORM_SRCS} platformcore.h platformcore.cpp)
set(PLATFORM_SRCS ${PLATFORM_SRCS} handmade-qt.cpp)

add_executable(handmade-qt ${PLATFORM_SRCS})

target_link_libraries(handmade-qt Qt5::Gui)

add_executable(assetpackbuilder assetpack.h assetpackbuilder.cpp)

target_link_libraries(assetpackbuilder Qt5::Gui)

if(WIN32)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-subsystem,windows")
endif(WIN32)


#
# install
#

INSTALL(TARGETS handmade DESTINATION bin)
INSTALL(TARGETS handmade-qt DESTINATION bin)
INSTALL(TARGETS assetpackbuilder DESTINATION bin)

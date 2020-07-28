# CMakeList.txt : CMake project for ArtnetUdp, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(project_name ArtnetUdpSend)
project(${project_name})

if(MSVC)
set(MSVC_LIBS ws2_32)
else(MSVC)
endif(MSVC)
# Add source to this project's executable.
add_executable (${project_name} "ArtnetUdpSend.cpp" "ArtnetUdpSend.h")

target_link_libraries(${project_name}
PRIVATE ${MSVC_LIBS}
)
# TODO: Add tests and install targets if needed.

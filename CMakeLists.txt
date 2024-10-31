# This file handles building each task as sub-project.
#
# Each task becomes an independent project in which you can experiment.
#
# Tasks are added as separate sub-projects so as compilation errors in
# one task don't affect another task.

cmake_minimum_required(VERSION 3.30)

# 필요한 파일, 리소스 설정
file(GLOB src_file
        "${CMAKE_SOURCE_DIR}/src/*.h"
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.rc"
)

file(GLOB_RECURSE res_file
        "${CMAKE_SOURCE_DIR}/Res/**"
)

# 리소스 폴더 설정
set(CMAKE_VS_SDK_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/res")

project(jjump_king)

add_executable(jjump_king_debug WIN32)

set_target_properties(jjump_king_debug
        PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_ISO_COMPLIANT ON
)

# 컴파일러 설정
target_compile_definitions(jjump_king_debug PRIVATE
        "$<IF:$<CXX_COMPILER_ID:MSVC>,_UNICODE,UNICODE>"

)

add_compile_options(
        "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
        /permissive-
)


target_sources(jjump_king_debug PRIVATE "${src_file}")

# 리소스 파일 실행할 때 복사하는 용도
add_custom_command(
        TARGET jjump_king_debug POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<TARGET_PROPERTY:jjump_king_debug,SOURCE_DIR>/Res
        $<TARGET_FILE_DIR:jjump_king_debug>/
)

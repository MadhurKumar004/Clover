cmake_minimum_required(VERSION 3.20)
project(clover LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB PARSER_SRC src/parser/*.cpp )
file(GLOB CHECK_SRC src/check/*.cpp )
file(GLOB CODEGEN_SRC src/codegen/*.cpp)

add_executable(clover
    src/main.cpp
    src/lex.cpp
    src/util.cpp
    ${PARSER_SRC}
    ${CHECK_SRC}
    ${CODEGEN_SRC}
)

target_include_directories(clover PRIVATE include)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")

list (APPEND CMAKE_MODULE_PATH "${LLVM_DIR}/cmake")
target_include_directories(clover SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_definitions(clover PRIVATE ${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(LLVM_LIBS
    core
    support
    irreader
)

# Fall back to shared LLVM if static component libs not found
if(LLVM_LINK_LLVM_DYLIB)
    set(LLVM_LIBS LLVM)
endif()

target_link_libraries(clover PRIVATE ${LLVM_LIBS})
install(TARGETS clover RUNTIME DESTINATION bin)

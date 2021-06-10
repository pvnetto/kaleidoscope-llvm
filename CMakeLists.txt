cmake_minimum_required(VERSION 3.10)

project(Kaleidoscope VERSION 1.0
					DESCRIPTION "Kaleidoscope LLVM"
					LANGUAGES C CXX)

# Adds LLVM
set(LLVM_DIR "C:/Program Files/llvm-project/build")

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(llvm-libs Support Core irreader ExecutionEngine OrcJIT native)
message(STATUS "LLVM Libs: ${llvm-libs}")

# Adds source
add_subdirectory(src)
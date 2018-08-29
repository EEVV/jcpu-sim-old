cmake_minimum_required(VERSION 3.11)

project(jcpu-sim)

add_executable(
	jcpu-sim

	${PROJECT_BINARY_DIR}/src/main.c
	${PROJECT_BINARY_DIR}/src/cpu.c
)

set_target_properties(jcpu-sim PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/)

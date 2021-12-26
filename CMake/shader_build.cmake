find_package(Vulkan COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

function(compile_shader target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "ENV;FORMAT" "SOURCES")
    set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    foreach(source ${arg_SOURCES})
        add_custom_command(
            OUTPUT ${OUTPUT_DIR}/${source}.${arg_FORMAT}
            DEPENDS ${source}
            DEPFILE ${OUTPUT_DIR}/${source}.d
            COMMAND
                ${glslc_executable}
                $<$<BOOL:${arg_ENV}>:--target-env=${arg_ENV}>
                $<$<BOOL:${arg_FORMAT}>:-mfmt=${arg_FORMAT}>
                -MD -MF ${OUTPUT_DIR}/${source}.d
                -o ${OUTPUT_DIR}/${source}.${arg_FORMAT}
                ${CMAKE_CURRENT_SOURCE_DIR}/${source}
        )
        message(STATUS "Shader source file ${source}")
        target_sources(${target} PRIVATE ${OUTPUT_DIR}/${source}.${arg_FORMAT})
    endforeach()
endfunction()
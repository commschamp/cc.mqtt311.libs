if (NOT CC_MQTT311_CLIENT_AFL_FUZZ)
    return ()
endif ()

set (SRC_TEMPL ${CMAKE_CURRENT_SOURCE_DIR}/AflFuzz.cpp.templ)
set (TEMPL_PROCESS_SCRIPT ${PROJECT_SOURCE_DIR}/cmake/ProcessTemplate.cmake)

######################################################################

function (gen_mqtt311_client_afl_fuzz config_file)
    if (NOT "${config_file}" STREQUAL "")
        include (${config_file})
    endif()

    set (name "${CC_MQTT311_CLIENT_CUSTOM_NAME}")
    if ("${CC_MQTT311_CLIENT_CUSTOM_NAME}" STREQUAL "")
        set (dir ${DEFAULT_CLIENT_DIR_NAME})
        set (lib_name "cc_mqtt311_client")
        set (app_name "cc_mqtt311_client_afl_fuzz")
    else ()
        set (dir "${CC_MQTT311_CLIENT_CUSTOM_NAME}")
        set (name "${name}_")
        set (lib_name "cc_mqtt311_${name}client")
        set (app_name "cc_mqtt311_${name}client_afl_fuzz")
    endif ()
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${dir})    

    set (src_output ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${name}AflFuzz.cpp)

    add_custom_command(
        OUTPUT "${src_output}"
        COMMAND ${CMAKE_COMMAND} 
            -DIN_FILE="${SRC_TEMPL}"
            -DOUT_FILE="${src_output}"
            -DNAME="${name}"
            -P ${TEMPL_PROCESS_SCRIPT}
        DEPENDS ${SRC_TEMPL} ${TEMPL_PROCESS_SCRIPT}
    )
    
    set_source_files_properties(
        ${src_output}
        PROPERTIES GENERATED TRUE
    )
    
    set (src_tgt_name "${name}main.cpp.tgt") 
    add_custom_target(
        ${src_tgt_name}
        DEPENDS "${src_output}" ${SRC_TEMPL} ${TEMPL_PROCESS_SCRIPT}
    )

    set (src
        ${src_output}
        Generator.cpp
        Logger.cpp
        ProgramOptions.cpp
        main.cpp
    )

    add_executable(${app_name} ${src})
    target_link_libraries(${app_name} PRIVATE cc::${lib_name} cc::cc_mqtt311 cc::comms Boost::program_options)
    target_include_directories(${app_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    add_dependencies(${app_name} ${src_tgt_name})
    
    install (
        TARGETS ${app_name}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endfunction()

######################################################################

find_package (Boost REQUIRED COMPONENTS program_options)

if (CC_MQTT311_CLIENT_DEFAULT_LIB) 
    gen_mqtt311_client_afl_fuzz("")
endif ()

if (NOT "${CC_MQTT311_CUSTOM_CLIENT_CONFIG_FILES}" STREQUAL "")
    foreach (custom_config ${CC_MQTT311_CUSTOM_CLIENT_CONFIG_FILES})
        gen_mqtt311_client_afl_fuzz("${custom_config}")
    endforeach ()
endif ()
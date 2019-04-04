function(flawfinder_add_target TARGET DIRECTORY)
    add_custom_target(
        flawfinder-${TARGET}
        COMMAND ${FLAWFINDER} -m 1 -C -c -D -Q -- ${DIRECTORY}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    add_dependencies(${TARGET} flawfinder-${TARGET})
endfunction()
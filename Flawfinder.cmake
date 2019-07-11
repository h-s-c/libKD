function(flawfinder_add_target TARGET DIRECTORY)
    file(GLOB FILES "${DIRECTORY}/*.c")
    add_custom_target(
        flawfinder-${TARGET}
        COMMAND ${FLAWFINDER} -F -m 1 -C -c -D -Q -S -- ${FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    add_dependencies(${TARGET} flawfinder-${TARGET})
endfunction()
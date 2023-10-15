set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_INSTALL_PREFIX}/Canvas3D)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_INSTALL_PREFIX}/Canvas3D)
set(TARGET_NAME canvas3d)

set(HEADERS
    canvas3d_export.h
    plugin.h
    canvas3d.h
    render_info.h
    customized_manipulator.h
    drawable.h
    shader_manager.h
    operation.h
)

set(SRCS
    plugin.cpp
    canvas3d.cpp
    render_info.cpp
    customized_manipulator.cpp
    drawable.cpp
    shader_manager.cpp
    operation.cpp
)

add_library(${TARGET_NAME} SHARED ${HEADERS} ${SRCS})

add_custom_command(TARGET ${TARGET_NAME}
	POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/qmldir
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${TARGET_NAME}>
        ${CMAKE_INSTALL_PREFIX}
)

set(QML_MODULE_NAME Canvas3D)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/qmldir.in" "${CMAKE_CURRENT_SOURCE_DIR}/qmldir")

target_compile_definitions(canvas3d PRIVATE CANVAS_LIBRARY)
target_link_libraries(canvas3d PRIVATE Qt6::Qml Qt6::Quick OSG common)

set(qml_dir ${CMAKE_CURRENT_SOURCE_DIR}/qmldir)

set_target_properties(${TARGET_NAME} PROPERTIES
    QMLDIR ${qml_dir}
    QML_MODULE ${QML_MODULE_NAME}
    FOLDER "core"
)
set(TARGET_NAME Creator)

#set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TARGET_NAME})
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${TARGET_NAME})

set(HEADERS
	interface.h
	deferred_rendering.h
	object.h
	node.h
	lights.h
)
set(SRCS
	main.cpp
	interface.cpp
	deferred_rendering.cpp
	object.cpp
	node.cpp
	lights.cpp
)
set(QMLS
	main.qml
	MyButton.qml
	ModelListPanel.qml
	ModelPropertyPanel.qml
	ScenePropertyPanel.qml
)

add_executable(${TARGET_NAME} ${HEADERS} ${SRCS} ${QMLS} resource.qrc)
target_include_directories(${TARGET_NAME} PRIVATE $<TARGET_PROPERTY:canvas3d,SOURCE_DIR>)
target_link_libraries(${TARGET_NAME}
	OSG
	Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Concurrent
	common
	canvas3d
	physical
)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${_qt_deploy_exe_} --qmldir ${CMAKE_CURRENT_SOURCE_DIR} $<TARGET_FILE:${TARGET_NAME}>
										  WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET_NAME}>)

set(DEBUG_BINS
	${OSG_ROOT}/bin/osg161-osgd.dll
	${OSG_ROOT}/bin/ot21-OpenThreadsd.dll
	${OSG_ROOT}/bin/osg161-osgViewerd.dll
	${OSG_ROOT}/bin/osg161-osgGAd.dll
	${OSG_ROOT}/bin/osg161-osgUtild.dll
	${OSG_ROOT}/bin/osg161-osgTextd.dll
	${OSG_ROOT}/bin/osg161-osgDBd.dll
	${OSG_ROOT}/bin/osg161-osgManipulatord.dll
	${OSG_ROOT}/bin/zlibd.dll
	${OSG_ROOT}/bin/libpng16d.dll
)

set(RELEASE_BINS
	${OSG_ROOT}/bin/osg161-osg.dll
	${OSG_ROOT}/bin/ot21-OpenThreads.dll
	${OSG_ROOT}/bin/osg161-osgViewer.dll
	${OSG_ROOT}/bin/osg161-osgGA.dll
	${OSG_ROOT}/bin/osg161-osgUtil.dll
	${OSG_ROOT}/bin/osg161-osgText.dll
	${OSG_ROOT}/bin/osg161-osgDB.dll
	${OSG_ROOT}/bin/osg161-osgManipulator.dll
	${OSG_ROOT}/bin/zlib.dll
	${OSG_ROOT}/bin/libpng16.dll
)

install(FILES ${DEBUG_BINS} CONFIGURATIONS DEBUG DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}>)
install(FILES ${RELEASE_BINS} CONFIGURATIONS RELEASE DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}>)
#install(DIRECTORY ${OSG_BIN_DIR}/ CONFIGURATIONS RELEASE DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}>)
#install(DIRECTORY ${OCC_LIB_DIR}/bin/ CONFIGURATIONS RELEASE DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}>)
#install(DIRECTORY ${TBB_ROOT}/bin/ CONFIGURATIONS RELEASE DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}>)
#install(DIRECTORY ${OSG_BIN_DIR}/  DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}> PATTERN "*.exe" EXCLUDE)
install(DIRECTORY ${OCC_LIB_DIR}/bin/  DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}> PATTERN "*.exe" EXCLUDE)
install(DIRECTORY ${TBB_ROOT}/bin/  DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}>)
install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}> OPTIONAL)

#install(FILES $<TARGET_PROPERTY:canvas3d,QMLDIR> DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}>/$<TARGET_PROPERTY:canvas3d,QML_MODULE>)
#install(FILES $<TARGET_FILE:canvas3d> DESTINATION $<TARGET_FILE_DIR:${TARGET_NAME}>/$<TARGET_PROPERTY:canvas3d,QML_MODULE>)

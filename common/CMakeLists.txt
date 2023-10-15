set(TARGET_NAME common)
set(HEADERS
	common_export.h
	model_data.h
	io/read_model_file.h
	io/read_stl.h
	io/read_stp.h
)
set(SRCS
	io/read_model_file.cpp
	io/read_stl.cpp
	io/read_stp.cpp
)
add_library(${TARGET_NAME} SHARED ${HEADERS} ${SRCS})
target_include_directories(${TARGET_NAME}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${OCC_ROOT}/inc
)
target_link_directories(${TARGET_NAME} PRIVATE ${OCC_LIB_DIR}/lib)

set(OCCLIBS
	TKMath
	TKGeomBase
	TKTopAlgo
	TKBRep
	TKMesh
	TKernel
	TKPrim
	TKXSBase
	TKSTEP
)
target_link_libraries(${TARGET_NAME} PRIVATE
	OSG
	Qt6::Core
	${OCCLIBS}
	)
target_compile_definitions(${TARGET_NAME} PRIVATE COMMON_LIBRARY)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "core")
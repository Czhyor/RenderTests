set(TARGET_NAME physical)

set(HEADERS
	pnode.h
	pnode.cpp
	physical_export.h
)

add_library(${TARGET_NAME} SHARED ${HEADERS})
target_link_libraries(${TARGET_NAME} PUBLIC OSG Qt6::Core)
target_compile_definitions(${TARGET_NAME} PRIVATE PHYSICAL_LIBRARY)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "engine")
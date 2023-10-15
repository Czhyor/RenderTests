#pragma once

#include "common/common_export.h"
#include "common/model_data.h"
#include <osg/Geometry>
#include <osg/Program>
#include <osg/BufferIndexBinding>
#include <fstream>

extern ModelData COMMON_EXPORT readSTL(const std::string& fileName);

extern void COMMON_EXPORT convertToIndexed(ModelData& data);
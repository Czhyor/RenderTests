#pragma once

#include "common_export.h"
#include <osg/PrimitiveSet>
#include <osg/Array>
#include <osg/Image>

struct COMMON_EXPORT ModelData {
	osg::ref_ptr<osg::Vec3Array> m_vertexArray;
	osg::ref_ptr<osg::Vec3Array> m_normalArray;
	osg::ref_ptr<osg::Vec4Array> m_colorArray;
	osg::ref_ptr<osg::ByteArray> m_stateArray;
	osg::ref_ptr<osg::Vec2Array> m_uvArray;
	osg::ref_ptr<osg::FloatArray> m_weightArray;
	osg::ref_ptr<osg::Image> m_image;
	osg::ref_ptr<osg::DrawElementsUInt> m_drawElement;
};
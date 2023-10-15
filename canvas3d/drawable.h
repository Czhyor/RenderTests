#pragma once

#include "canvas3d_export.h"
#include <osg/Geometry>
#include <common/model_data.h>

class CANVAS_EXPORT Drawable
{
public:
	Drawable();
	virtual ~Drawable();

	enum Index {
		Vertex,
		Normal,
		Color,
		State,
		UV,
		Weight
	};

	virtual osg::ref_ptr<osg::Geometry> createGeometry();

	void setModelData(const ModelData& data);
	const ModelData& getModelData() const;
protected:
	ModelData m_data;
};

class CANVAS_EXPORT Mesh : public Drawable
{
public:
	virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
};

class CANVAS_EXPORT Line : public Drawable
{
public:
	virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
};

class CANVAS_EXPORT Point : public Drawable
{
public:
	virtual osg::ref_ptr<osg::Geometry> createGeometry() override;
};
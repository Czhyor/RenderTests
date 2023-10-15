#pragma once

#include "canvas3d_export.h"
#include <osgGA/OrbitManipulator>
#include <osg/MatrixTransform>
class CANVAS_EXPORT CustomizedManipulator : public osgGA::OrbitManipulator
{
public:
	CustomizedManipulator();
	~CustomizedManipulator();

	virtual bool handleMouseDrag(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;

protected:
	virtual bool performMovement() override;
	virtual bool performMovementLeftMouseButton(const double eventTimeDelta, const double dx, const double dy) override;

	virtual void rotateTrackball(const float px0, const float py0,
		const float px1, const float py1, const float scale) override;

	virtual bool handleMouseWheel(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;
};

class CANVAS_EXPORT FirstPersionManipulator : public osgGA::CameraManipulator
{
public:
	FirstPersionManipulator(const osg::Vec3& front, const osg::Vec3& up);
	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;

	virtual void setByMatrix(const osg::Matrixd& matrix) override;
	virtual void setByInverseMatrix(const osg::Matrixd& matrix) override;

	virtual osg::Matrixd getMatrix() const override;
	virtual osg::Matrixd getInverseMatrix() const override;

protected:
	void handleFrame(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);
	void handleKeyDown(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);
	void handleMouseMove(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

	void rotate(float dx, float dy);

private:
	osg::Vec3 m_defaultFront;
	osg::Vec3 m_defaultUp;

	osg::ref_ptr<const osgGA::GUIEventAdapter> m_lastMouseEvent;

	osg::Vec3d m_center;
	osg::Quat m_rotation;
	double m_distance;
};
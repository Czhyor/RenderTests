#pragma once

#include "canvas3d_export.h"
#include <osgViewer/CompositeViewer>
#include <QObject>
#include <QOpenGLFramebufferObject>

enum ShaderMode
{
	PreviewMaterial,
	PhongMaterial,
	PBR
};

#define DIRECTIONAL_LIGHTS_MAX 10
#define POINT_LIGHTS_MAX 10
#define SPOT_LIGHTS_MAX 10

struct DirectionalLightMaterial
{
	osg::Vec4 color;
	osg::Vec4 direction;
};
struct DirectionalLightUBuffer
{
	osg::Vec4i count;
	DirectionalLightMaterial mats[DIRECTIONAL_LIGHTS_MAX];
};

struct PointLightMaterial
{
	osg::Vec4 color;
	osg::Vec4 position;
	osg::Vec4 param;
};
struct PointLightUBuffer
{
	osg::Vec4i count;
	PointLightMaterial mats[POINT_LIGHTS_MAX];
};

struct SpotLightMaterial
{
	osg::Vec4 color;
	osg::Vec4 position;
	osg::Vec4 direction;
	osg::Vec4 cutOff;	// x: inner cutOff(cos(angle)); y: outer cutOff(cos(angle))
};
struct SpotLightUBuffer
{
	osg::Vec4i count;
	SpotLightMaterial mats[SPOT_LIGHTS_MAX];
};

class CANVAS_EXPORT ViewInfo
{
public:
	static osg::ref_ptr<osgViewer::View> createView();
	static osg::Switch* getRoot(osgViewer::View* view);
	static osg::Switch* getModelGroup(osgViewer::View* view);
	static osg::Switch* getOtherGroup(osgViewer::View* view);
	static void setQtFBO(osgViewer::View* view, QOpenGLFramebufferObject* qtFBO);
	static QOpenGLFramebufferObject* getQtFBO(osgViewer::View* view);
};

class CANVAS_EXPORT RenderInfo : public QObject
{
	Q_OBJECT
public:
	RenderInfo();
	RenderInfo(const RenderInfo& other);
	RenderInfo(const RenderInfo&& other);
	~RenderInfo();

	RenderInfo operator=(const RenderInfo& other) const;

	Q_INVOKABLE void getInfo();

	void addOperation(osg::ref_ptr<osg::Operation> op);

	osg::ref_ptr<osgViewer::CompositeViewer> m_compositeViewer;
	osg::ref_ptr<osgViewer::View> m_mainView;
	osg::ref_ptr<osgGA::EventQueue> m_eventQueue;
};
Q_DECLARE_METATYPE(RenderInfo*)
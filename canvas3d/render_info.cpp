#include "render_info.h"
#include <osgViewer/ViewerEventHandlers>
#include <osg/BufferIndexBinding>

class ViewUserData : public osg::Referenced
{
public:
	osg::ref_ptr<osg::Switch> m_root;
	osg::ref_ptr<osg::Switch> m_model;
	osg::ref_ptr<osg::Switch> m_other;
	QOpenGLFramebufferObject* m_qtFBO = nullptr;
};

osg::ref_ptr<osgViewer::View> ViewInfo::createView()
{
	osg::ref_ptr<osgViewer::View> view = new osgViewer::View;
	osg::ref_ptr<osg::Switch> root = new osg::Switch;
	root->setName("root");
	osg::ref_ptr<osg::Switch> model = new osg::Switch;
	model->setName("model");
	osg::ref_ptr<osg::Switch> other = new osg::Switch;
	other->setName("other");
	root->addChild(model);
	root->addChild(other);
	view->setSceneData(root);

	ViewUserData* vud = new ViewUserData;
	vud->m_root = root;
	vud->m_model = model;
	vud->m_other = other;
	view->setUserData(vud);

	// vieport uniform
	osg::Uniform* viewportUniform = new osg::Uniform("viewport", osg::Vec4());
	class ViewportUniformUpdateCallback : public osg::UniformCallback
	{
	public:
		virtual void operator () (osg::Uniform* uniform, osg::NodeVisitor* nv) override {
			osgUtil::UpdateVisitor* uv = nv->asUpdateVisitor();
			const osg::NodePath& nodePath = nv->getNodePath();
			const osg::Camera* camera = nodePath.front()->asCamera();
			const osg::Viewport* viewport = camera->getViewport();
			osg::Vec4 vec(viewport->x(), viewport->y(), viewport->width(), viewport->height());
			uniform->set(vec);
		}
	};
	viewportUniform->setUpdateCallback(new ViewportUniformUpdateCallback);
	view->getCamera()->getOrCreateStateSet()->addUniform(viewportUniform);

	// shader mode uniform
	view->getCamera()->getOrCreateStateSet()->addUniform(new osg::Uniform("shaderMode", static_cast<int>(ShaderMode::PreviewMaterial)));
	
	// directional light ubo
	osg::FloatArray* directionalLightData = new osg::FloatArray;
	directionalLightData->resize(sizeof(DirectionalLightUBuffer), 0.0f);
	osg::UniformBufferBinding* directionalLightUBuffer = new osg::UniformBufferBinding(0, directionalLightData);
	view->getCamera()->getOrCreateStateSet()->setAttributeAndModes(directionalLightUBuffer);
	// point light ubo
	osg::FloatArray* pointLightData = new osg::FloatArray;
	pointLightData->resize(sizeof(PointLightUBuffer), 0.0f);
	osg::UniformBufferBinding* pointLightUBuffer = new osg::UniformBufferBinding(1, pointLightData);
	view->getCamera()->getOrCreateStateSet()->setAttributeAndModes(pointLightUBuffer);
	// spot light ubo
	osg::FloatArray* spotLightData = new osg::FloatArray;
	spotLightData->resize(sizeof(SpotLightUBuffer), 0.0f);
	osg::UniformBufferBinding* spotLightUBuffer = new osg::UniformBufferBinding(2, spotLightData);
	view->getCamera()->getOrCreateStateSet()->setAttributeAndModes(spotLightUBuffer);

	// shadow uniform
	view->getCamera()->getOrCreateStateSet()->addUniform(new osg::Uniform("useShadow", true));

	// other mode
	view->getCamera()->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::OFF);

	//view->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	//view->addEventHandler(new osgViewer::StatsHandler);

	return view;
}

void ViewInfo::setQtFBO(osgViewer::View* view, QOpenGLFramebufferObject* qtFBO)
{
	if (view == nullptr) {
		return;
	}

	auto vud = dynamic_cast<ViewUserData*>(view->getUserData());
	if (vud == nullptr) {
		return;
	}
	vud->m_qtFBO = qtFBO;
}

QOpenGLFramebufferObject* ViewInfo::getQtFBO(osgViewer::View* view)
{
	if (view == nullptr) {
		return nullptr;
	}

	auto vud = dynamic_cast<ViewUserData*>(view->getUserData());
	if (vud == nullptr) {
		return nullptr;
	}
	return vud->m_qtFBO;
}

osg::Switch* ViewInfo::getRoot(osgViewer::View* view)
{
	if (view == nullptr) {
		return nullptr;
	}
	auto vud = dynamic_cast<ViewUserData*>(view->getUserData());
	if (vud == nullptr) {
		return nullptr;
	}
	return vud->m_root;
}

osg::Switch* ViewInfo::getModelGroup(osgViewer::View* view)
{
	if (view == nullptr) {
		return nullptr;
	}
	auto vud = dynamic_cast<ViewUserData*>(view->getUserData());
	if (vud == nullptr) {
		return nullptr;
	}
	return vud->m_model;
}

osg::Switch* ViewInfo::getOtherGroup(osgViewer::View* view)
{
	if (view == nullptr) {
		return nullptr;
	}
	auto vud = dynamic_cast<ViewUserData*>(view->getUserData());
	if (vud == nullptr) {
		return nullptr;
	}
	return vud->m_other;
}

RenderInfo::RenderInfo()
{

}

RenderInfo::RenderInfo(const RenderInfo& other) :
	m_compositeViewer(other.m_compositeViewer),
	m_mainView(other.m_mainView),
	m_eventQueue(other.m_eventQueue)
{

}

RenderInfo::RenderInfo(const RenderInfo&& other) :
	m_compositeViewer(other.m_compositeViewer),
	m_mainView(other.m_mainView),
	m_eventQueue(other.m_eventQueue)
{

}

RenderInfo::~RenderInfo()
{

}

void RenderInfo::getInfo()
{
	qDebug() << "Info";
}

RenderInfo RenderInfo::operator=(const RenderInfo& other) const
{
	RenderInfo renderInfo;
	renderInfo.m_compositeViewer = other.m_compositeViewer;
	renderInfo.m_mainView = other.m_mainView;
	renderInfo.m_eventQueue = other.m_eventQueue;
	return renderInfo;
}

void RenderInfo::addOperation(osg::ref_ptr<osg::Operation> op)
{
	m_compositeViewer->getUpdateOperations()->add(op);
}

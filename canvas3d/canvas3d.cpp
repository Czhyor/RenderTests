#include "canvas3d.h"
#include <common/io/read_stl.h>
#include "customized_manipulator.h"
#include <QOpenGLFunctions>
#include <QVersionNumber>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>
#include <osgGA/OrbitManipulator>

MyRenderer::MyRenderer()
{
	m_renderInfo.reset(new RenderInfo);
	m_renderInfo->m_compositeViewer = new osgViewer::CompositeViewer;
	m_renderInfo->m_eventQueue = new osgGA::EventQueue;
	m_renderInfo->m_eventQueue->getCurrentEventState()->setWindowRectangle(0, 0, 100, 100);
	osgViewer::GraphicsWindow* gw = new osgViewer::GraphicsWindowEmbedded(0, 0, 100, 100);
	gw->setEventQueue(m_renderInfo->m_eventQueue);
	gw->getState()->setUseModelViewAndProjectionUniforms(true);
	gw->getState()->setGlobalDefaultModeValue(GL_BLEND, true);

	osg::ref_ptr<osgViewer::View> view = ViewInfo::createView();
	view->getCamera()->setProjectionMatrixAsPerspective(30.0, 100.0 / 100.0, 1.0, 1000.0);
	view->getCamera()->setViewport(0, 0, 100, 100);
	view->getCamera()->setGraphicsContext(gw);
	m_renderInfo->m_compositeViewer->addView(view);
	m_renderInfo->m_mainView = view;
	m_renderInfo->m_compositeViewer->setUpdateOperations(new osg::OperationQueue);

	osgGA::OrbitManipulator* om = new osgGA::OrbitManipulator;
	om->setAllowThrow(false);
	view->setCameraManipulator(new CustomizedManipulator);
}

QOpenGLFunctions_4_5_Core* MyRenderer::getOpenGLFunc() {
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
	return QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>();
#else
	QOpenGLContext* cxt = QOpenGLContext::currentContext();
	const unsigned char* pVer = cxt->functions()->glGetString(GL_VERSION);
	printf("opengl version:%s\n", pVer);
	qDebug() << "OpenGL Version:" << pVer;
	return cxt->versionFunctions<QOpenGLFunctions_4_5_Core>();
#endif
}

void MyRenderer::render() {
	// render with OpenGL
	//qDebug() << "render";

	//auto func = getOpenGLFunc();
	////qDebug() << "func:" << func;
	//if (func) {
	//	func->glClearColor(1.0, 0.0, 0.0, 1.0);
	//	func->glClearDepth(1.0);
	//	func->glClear(GL_COLOR_BUFFER_BIT);
	//	func->glUseProgram(0);
	//}
	m_renderInfo->m_compositeViewer->frame();

	auto gc = m_renderInfo->m_mainView->getCamera()->getGraphicsContext();
	gc->getState()->lazyDisablingOfVertexAttributes();
	gc->getState()->applyDisablingOfVertexAttributes();

	QQuickOpenGLUtils::resetOpenGLState();

	update();
}

QOpenGLFramebufferObject* MyRenderer::createFramebufferObject(const QSize& size) {

	QOpenGLFramebufferObject* fbo = new QOpenGLFramebufferObject(size, QOpenGLFramebufferObject::Attachment::Depth);
	ViewInfo::setQtFBO(m_renderInfo->m_mainView, fbo);
	return fbo;
}

Canvas3d::Canvas3d() {
	qDebug() << "Canvas3d's construction";
	setAcceptedMouseButtons(Qt::MouseButton::AllButtons);
	setAcceptHoverEvents(true);
	setMirrorVertically(true);
	m_renderer = new MyRenderer;
}

Canvas3d::~Canvas3d() {
	qDebug() << "Canvas3d's destruction";
}

QQuickFramebufferObject::Renderer* Canvas3d::createRenderer() const {
	qDebug() << "createRenderer";
	
	return m_renderer;
}

RenderInfo* Canvas3d::getRenderInfo()
{
	return m_renderer->m_renderInfo.get();
}

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
void Canvas3d::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
#else
void Canvas3d::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
#endif
{
	if (newGeometry.width() == 0 || newGeometry.height() == 0) {
		return;
	}

	qreal fwidth = static_cast<qreal>(newGeometry.width());
	qreal fheight = static_cast<qreal>(newGeometry.height());
	if (window()) {
		qreal dpr = window()->devicePixelRatio();
		fwidth *= dpr;
		fheight *= dpr;
	}
	int width = static_cast<int>(fwidth);
	int height = static_cast<int>(fheight);

	auto camera = m_renderer->m_renderInfo->m_mainView->getCamera();
	//camera->setViewport(0, 0, newGeometry.width(), newGeometry.height());
	camera->getGraphicsContext()->resized(0, 0, width, height);
	m_renderer->m_renderInfo->m_eventQueue->windowResize(0, 0, width, height, 0.0);

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
	QQuickFramebufferObject::geometryChange(newGeometry, oldGeometry);
#else
	QQuickFramebufferObject::geometryChanged(newGeometry, oldGeometry);
#endif
	//m_renderer->m_eventQueue->getCurrentEventState()->setWindowRectangle(0, 0, newGeometry.width(), newGeometry.height());
}

void Canvas3d::keyPressEvent(QKeyEvent* event)
{
	int key = getOsgKey(event);
	m_renderer->m_renderInfo->m_eventQueue->keyPress(key);
}

void Canvas3d::keyReleaseEvent(QKeyEvent* event)
{
	int key = getOsgKey(event);
	m_renderer->m_renderInfo->m_eventQueue->keyRelease(key);
}

int Canvas3d::getOsgButton(Qt::MouseButton qButton)
{
	switch (qButton)
	{
	case Qt::MouseButton::LeftButton:
		return 1;
	case Qt::MouseButton::MiddleButton:
		return 2;
	case Qt::MouseButton::RightButton:
		return 3;
	default:
		return 0;
	}
}

int Canvas3d::getOsgKey(QKeyEvent* ke)
{
	switch (ke->key())
	{
	case Qt::Key::Key_S:
		return osgGA::GUIEventAdapter::KeySymbol::KEY_S;
	case Qt::Key::Key_W:
		return osgGA::GUIEventAdapter::KeySymbol::KEY_W;
	case Qt::Key::Key_A:
		return osgGA::GUIEventAdapter::KeySymbol::KEY_A;
	case Qt::Key::Key_D:
		return osgGA::GUIEventAdapter::KeySymbol::KEY_D;
	default:
		return -1;
	}
}

void Canvas3d::mousePressEvent(QMouseEvent* event)
{
	m_renderer->m_renderInfo->m_eventQueue->mouseButtonPress(event->x(), event->y(), getOsgButton(event->button()));
}

void Canvas3d::mouseMoveEvent(QMouseEvent* event)
{
	m_renderer->m_renderInfo->m_eventQueue->mouseMotion(event->x(), event->y());
}

void Canvas3d::mouseReleaseEvent(QMouseEvent* event)
{
	m_renderer->m_renderInfo->m_eventQueue->mouseButtonRelease(event->x(), event->y(), getOsgButton(event->button()));
}

void Canvas3d::mouseDoubleClickEvent(QMouseEvent* event)
{

}

void Canvas3d::wheelEvent(QWheelEvent* event)
{
	qDebug() << "pixel data:" << event->pixelDelta();
	qDebug() << "angle data:" << event->angleDelta();
	QPoint angleDelta = event->angleDelta();
	osgGA::GUIEventAdapter::ScrollingMotion sm = osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_UP;
	if (angleDelta.y() < 0) {
		sm = osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_DOWN;
	}
	m_renderer->m_renderInfo->m_eventQueue->mouseScroll(sm);
}

void Canvas3d::focusInEvent(QFocusEvent*)
{
	//qDebug() << "focusIn";
}
void Canvas3d::focusOutEvent(QFocusEvent*)
{
	//qDebug() << "focusOut";
}

void Canvas3d::hoverEnterEvent(QHoverEvent* event)
{
	//qDebug() << "hoverEnter";
	forceActiveFocus();
}
void Canvas3d::hoverMoveEvent(QHoverEvent* event)
{
	//qDebug() << "hoverMove";
}
void Canvas3d::hoverLeaveEvent(QHoverEvent* event)
{
	//qDebug() << "hoverLeave";
}
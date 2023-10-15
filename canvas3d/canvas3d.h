#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions_4_5_Core>

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
#include <QOpenGLVersionFunctionsFactory>
#endif

#include "render_info.h"

class MyRenderer : public QQuickFramebufferObject::Renderer
{
public:
	MyRenderer();
	QOpenGLFunctions_4_5_Core* getOpenGLFunc();

	virtual void render() override;
	virtual QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;
public:
	std::shared_ptr<RenderInfo> m_renderInfo;
};

class Canvas3d : public QQuickFramebufferObject
{
	Q_OBJECT
    Q_PROPERTY(QString description READ getDescription CONSTANT)
	Q_PROPERTY(RenderInfo* renderInfo READ getRenderInfo CONSTANT)
public:
	Canvas3d();
	~Canvas3d();
	virtual Renderer* createRenderer() const override;
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
	virtual void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
#else
	virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;
#endif

	QString getDescription() const { return QString("This is Canvas3D!!!"); }

signals:
	void renderInfoChanged();

protected:
	RenderInfo* getRenderInfo();

protected:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);

	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);

	virtual void wheelEvent(QWheelEvent* event);

	virtual void focusInEvent(QFocusEvent*);
	virtual void focusOutEvent(QFocusEvent*);

	virtual void hoverEnterEvent(QHoverEvent* event);
	virtual void hoverMoveEvent(QHoverEvent* event);
	virtual void hoverLeaveEvent(QHoverEvent* event);


private:
	int getOsgButton(Qt::MouseButton qButton);
	int getOsgKey(QKeyEvent* ke);

	mutable MyRenderer* m_renderer;
};

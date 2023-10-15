#pragma once
#include <QObject>
#include <QThread>
#include <QtConcurrent>
#include <QQmlEngine>
#include <QVector3D>

#include <vector>
#include <memory>

#include <render_info.h>
#include "node.h"
#include "lights.h"

namespace Physical {
	class PhysicalEngine;
}

class Interface : public QObject
{
	Q_OBJECT
public:
	Interface();
	~Interface();

	template<class T>
	T* createObject() {
		T* obj = new T;
		obj->setRenderInfo(m_renderInfo.get());
		return obj;
	}

	Q_INVOKABLE void setRenderInfo(QObject* renderInfo);

	Q_INVOKABLE void run();
	Q_INVOKABLE void stop();
	Q_INVOKABLE void reset();

	Q_INVOKABLE void addMap(const QString& filePath);
	Q_INVOKABLE void addModel(const QString& filePath);
	Q_INVOKABLE void addBillboard(const QString& filePath);

	Q_INVOKABLE void addCustomized();

	Q_INVOKABLE void setCameraFollowNode(Node* node);
	Q_INVOKABLE void enableManipulate(Node* node);

	Q_INVOKABLE void enableViewDatum();

	Q_INVOKABLE void enableIntersect(bool enable);

	Q_INVOKABLE void setDeferredRendering();
	Q_INVOKABLE void setForwardRendering();

	Q_INVOKABLE void addCubeMap();

	Q_INVOKABLE void addDirectionalLight();
	Q_INVOKABLE void addPointLight();
	Q_INVOKABLE void addSpotLight();

	Q_INVOKABLE void enableDrag(Object* obj);

	Q_INVOKABLE void setShaderMode(int mode);
	Q_INVOKABLE void showFrustum();
	Q_INVOKABLE void useShadow(bool bUsed);


signals:
	void nodeAdded(Node* node);
	void lightAdded(Light* light);

protected:
	std::shared_ptr<RenderInfo> m_renderInfo;
	QVector<QSharedPointer<Node>> m_nodes;
	QVector<QSharedPointer<Light>> m_lights;

	std::shared_ptr<Physical::PhysicalEngine> m_physicalEngine;
};
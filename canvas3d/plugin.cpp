#include "plugin.h"
#include "canvas3d.h"

Canvas3dPlugin::Canvas3dPlugin()
{
	qDebug() << "Canvas3dPlugin construction";
}

void Canvas3dPlugin::registerTypes(const char* uri) {
	qDebug() << "canvas3d::registertypoes:" << uri;
	qmlRegisterType<Canvas3d>(uri, 1, 0, "Canvas3d");
	qmlRegisterType<RenderInfo>(uri, 1, 0, "RenderInfo");
}
void Canvas3dPlugin::initializeEngine(QQmlEngine* engine, const char* uri) {
	qDebug() << "canvas3d::initializeEngine:" << engine << "uri:" << uri;
}
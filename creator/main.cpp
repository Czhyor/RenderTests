#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQmlContext>
#include "interface.h"

int main(int argc, char** argv)
{
	QCoreApplication::setAttribute(Qt::ApplicationAttribute::AA_UseDesktopOpenGL);
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
	QQuickWindow::setGraphicsApi(QSGRendererInterface::GraphicsApi::OpenGL);
#endif
	QGuiApplication app(argc, argv);
	QQmlApplicationEngine engine;
	qmlRegisterUncreatableType<Node>("Engine.Node", 1, 0, "Node", "Can't Create Node");
	engine.rootContext()->setContextProperty("$Interface", new Interface);
	engine.load("qrc:/main.qml");
	qDebug() << "importPath:" << engine.importPathList();

	return app.exec();
}
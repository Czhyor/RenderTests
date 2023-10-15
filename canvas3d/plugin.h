#pragma once

#include "canvas3d_export.h"
#include <QQmlExtensionPlugin>
#include <QQmlEngine>

class CANVAS_EXPORT Canvas3dPlugin : public QQmlExtensionPlugin
{
	Q_OBJECT
		Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
	Canvas3dPlugin();
	void registerTypes(const char* uri) override;
	void initializeEngine(QQmlEngine* engine, const char* uri) override;
};
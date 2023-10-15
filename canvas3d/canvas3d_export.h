#ifndef CANVAS_EXPORT_H
#define CANVAS_EXPORT_H

#include <qcompilerdetection.h>
#include <QDebug>
#ifdef CANVAS_LIBRARY
#define CANVAS_EXPORT Q_DECL_EXPORT
#else
#define CANVAS_EXPORT Q_DECL_IMPORT
#endif

#endif
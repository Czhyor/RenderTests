#ifndef PHYSICAL_EXPORT_H
#define PHYSICAL_EXPORT_H

#include <qcompilerdetection.h>
#include <QDebug>

#ifdef PHYSICAL_LIBRARY
#define PHYSICAL_EXPORT Q_DECL_EXPORT
#else
#define PHYSICAL_EXPORT Q_DECL_IMPORT
#endif

#endif
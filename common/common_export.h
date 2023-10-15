#ifndef COMMON_EXPORT_H
#define COMMON_EXPORT_H

#include <qcompilerdetection.h>
#include <QDebug>
#ifdef COMMON_LIBRARY
#define COMMON_EXPORT Q_DECL_EXPORT
#else
#define COMMON_EXPORT Q_DECL_IMPORT
#endif

#endif
#pragma once

#include "canvas3d_export.h"
#include <osg/OperationThread>
#include <functional>

class CANVAS_EXPORT LambdaOperation : public osg::Operation
{
public:
	LambdaOperation(std::function<void(void)> func);
	~LambdaOperation();
	virtual void operator () (osg::Object* obj) override;
protected:
	std::function<void()> m_func;
};
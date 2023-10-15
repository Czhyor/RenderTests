#include "operation.h"

LambdaOperation::LambdaOperation(std::function<void()> func):
	m_func(func)
{

}

LambdaOperation::~LambdaOperation()
{

}

void LambdaOperation::operator () (osg::Object* obj)
{
	m_func();
}
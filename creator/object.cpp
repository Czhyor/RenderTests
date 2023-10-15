#include "object.h"

Object::Object() : m_renderInfo(nullptr), m_bAddedToScene(false)
{

}

Object::~Object()
{

}

void Object::setRenderInfo(RenderInfo* renderInfo)
{
	m_renderInfo = renderInfo;
}
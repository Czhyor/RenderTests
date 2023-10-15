#ifndef MY_RENDER_OBJECT_H
#define MY_RENDER_OBJECT_H

#include <QObject>
#include <render_info.h>

//class ObjContext
//{
//public:
//	RenderInfo* m_renderInfo = nullptr;
//
//};

class Object : public QObject
{
	Q_OBJECT
public:
	Object();
	~Object();

	RenderInfo* getRenderInfo() { return m_renderInfo; }

	virtual void addToScene() = 0;

protected:
	bool m_bAddedToScene;

private:
	void setRenderInfo(RenderInfo* renderInfo);
	friend class Interface;
	RenderInfo* m_renderInfo;
};


#endif
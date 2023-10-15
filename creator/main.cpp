#include <osg/Geode>
#include <osgViewer/Viewer>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osg/Array>
#include <osg/BufferIndexBinding>
#include <osg/Texture2D>
#include <common/io/read_model_file.h>
#include "interface.h"

class FollowMouseEventHandler : public osgGA::GUIEventHandler
{
public:
	FollowMouseEventHandler(osg::MatrixTransform* mt) : m_mt(mt) {}
	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override {
		if (m_mt.valid()) {
			const osg::Vec3 pos(ea.getX(), ea.getY(), 0.0);
			m_mt->setMatrix(osg::Matrix::translate(pos));

			if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH && ea.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON) {
				printf("sculpt-start\n");
				return true;
			}
			if (ea.getEventType() == osgGA::GUIEventAdapter::DRAG && ea.getButtonMask() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON) {
				printf("sculpt\n");
				return true;
			}
			if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE && ea.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON) {
				printf("sculpt-end\n");
			}
		}

		return false;
	}
	osg::ref_ptr<osg::MatrixTransform> m_mt;
};

osg::Camera* createHudCamera()
{
	osg::Camera* camera = new osg::Camera;
	camera->setRenderOrder(osg::Camera::RenderOrder::POST_RENDER);
	camera->setReferenceFrame(osg::Transform::ReferenceFrame::ABSOLUTE_RF);
	camera->setClearColor(osg::Vec4(1.0, 1.0, 0.0, 1.0));
	//camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	camera->setClearMask(GL_DEPTH_BUFFER_BIT);
	camera->setViewMatrix(osg::Matrix::identity());
	camera->setProjectionMatrixAsOrtho2D(0, 640, 0, 480);
	return camera;
}

float createRandom()
{
	return (double)std::rand() / (double)RAND_MAX;
}

void readSSBO(osg::RenderInfo* renderInfo, osg::Geometry* geom)
{
	osg::GLExtensions* ext = osg::GLExtensions::Get(renderInfo->getContextID(), true);

	const osg::StateAttribute* tmplastSA = renderInfo->getState()->getLastAppliedAttribute(osg::StateAttribute::SHADERSTORAGEBUFFERBINDING);

	osg::StateAttribute* sa = geom->getOrCreateStateSet()->getAttribute(osg::StateAttribute::SHADERSTORAGEBUFFERBINDING);
	osg::ShaderStorageBufferBinding* ssbb = dynamic_cast<osg::ShaderStorageBufferBinding*>(sa);
	ssbb->getBufferData()->getGLBufferObject(renderInfo->getContextID())->bindBuffer();
	float* p = (float*)ext->glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY_ARB);
	// read ssbo
	ext->glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	const osg::StateAttribute* lastSA = renderInfo->getState()->getLastAppliedAttribute(osg::StateAttribute::SHADERSTORAGEBUFFERBINDING);
	if (lastSA) {
		const osg::ShaderStorageBufferBinding* lastSSBB = dynamic_cast<const osg::ShaderStorageBufferBinding*>(lastSA);
		lastSSBB->getBufferData()->getGLBufferObject(renderInfo->getContextID())->bindBuffer();
	}
	else {
		ssbb->getBufferData()->getGLBufferObject(renderInfo->getContextID())->unbindBuffer();
	}
	//renderInfo->getState()->applyAttribute(ssbb);
}


class MyDrawCallback : public osg::Geometry::DrawCallback
{
public:
	virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const override {

		const osg::StateAttribute* tmplastSA = renderInfo.getState()->getLastAppliedAttribute(osg::StateAttribute::SHADERSTORAGEBUFFERBINDING);

		// do custom thing before draw
		osg::GLExtensions* ext = osg::GLExtensions::Get(renderInfo.getContextID(), true);
		const osg::StateAttribute* sa = drawable->getStateSet()->getAttribute(osg::StateAttribute::SHADERSTORAGEBUFFERBINDING);
		const osg::ShaderStorageBufferBinding* ssbb = dynamic_cast<const osg::ShaderStorageBufferBinding*>(sa);
		//ssbb->getBufferData()->getGLBufferObject(renderInfo.getContextID())->bindBuffer();
		float* p = (float*)ext->glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY_ARB);
		// read ssbo

		ext->glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		drawable->drawImplementation(renderInfo);

		// do custom thing after draw
	}
};

class PostDrawCallback : public osg::Camera::DrawCallback
{
public:
	PostDrawCallback(osg::Geometry* geom) : m_geom(geom) {}
	virtual void operator () (osg::RenderInfo& renderInfo) const override {
		readSSBO(&renderInfo, m_geom);
		// do custom thing
	}
private:
	osg::ref_ptr<osg::Geometry> m_geom;
};

class MyVisitor : public osg::NodeVisitor
{
public:
	MyVisitor() {}
	~MyVisitor() {}

	//virtual void apply(osg::Drawable& drawable) {
	//
	//}
	//virtual void apply(osg::Geometry& geometry);
	//
	virtual void apply(osg::Node& node) {
		printf("apply node: %s\n", node.getName().data());
	}
	//
	//virtual void apply(osg::Geode& node);
	//
	virtual void apply(osg::Group& node) {
		printf("apply group: %s\n", node.getName().data());
		
		node.traverse(*this);
	}
	//
	virtual void apply(osg::Transform& node) {
		printf("apply node: %s\n", node.getName().data());
		traverse(node);
	}
	//virtual void apply(osg::Camera& node);
	//
	virtual void apply(osg::MatrixTransform& node) {
		printf("MATRIXTRANSFORM\n");
		node.traverse(*this);
	}
	//
	//virtual void apply(osg::AutoTransform& node);
	//
	//virtual void apply(osg::Switch& node);
};

void testVisitor() {
	osg::Group* g = new osg::Group;
	g->setName("grandParent");

	osg::MatrixTransform* m = new osg::MatrixTransform;
	m->setName("mt");

	osg::Node* n0 = new osg::Node;
	n0->setName("n0");

	osg::Node* n1 = new osg::Node;
	n1->setName("n1");

	g->addChild(m);
	m->addChild(n0);
	m->addChild(n1);

	MyVisitor v;
	g->accept(v);
}


osg::Camera* createRTTCamera(osg::Texture2D* colorTexture, osg::Texture2D* depthTexture)
{
	osg::Camera* camera = new osg::Camera;
	camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
	camera->attach(osg::Camera::BufferComponent::COLOR_BUFFER, colorTexture);
	camera->attach(osg::Camera::BufferComponent::DEPTH_BUFFER, depthTexture);

	camera->resize(0, 0, osg::Camera::ResizeMask::RESIZE_VIEWPORT);

	return camera;
}


osg::Geometry* createRect(osg::Texture2D* colorTexture) {
	auto geom = new osg::Geometry;
	geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, colorTexture);
	return geom;
}


osg::Camera* createDeferCamera(osg::Texture2D* colorTexture) {
	auto camera = new osg::Camera;
	camera->setRenderOrder(osg::Camera::PRE_RENDER);
	camera->addChild(createRect(colorTexture));
	return camera;
}

#include <osg/Depth>
#include <osg/Vec3>
#include <customized_manipulator.h>
#include <drawable.h>
#include <shader_manager.h>


void runOSG() {
	osg::Node* node;

	ReadModelFile modelFile("C:/Users/Administrator/Desktop/NewFolder/model/mokey.stl");
	modelFile.read();

	ModelData data = modelFile.getModelData();

	Mesh mesh;
	mesh.setModelData(data);
#if 1
	auto geom = mesh.createGeometry();

	//auto program = ShaderMgr::instance()->getShader(ShaderMgr::s_meshProgram);
	//geom->getOrCreateStateSet()->setAttributeAndModes(program);

	//geom->setDrawCallback(new MyDrawCallback);
	geom->getOrCreateStateSet()->setRenderBinDetails(INT_MAX, "RenderBin");
	geom->getOrCreateStateSet()->setRenderingHint(osg::StateSet::RenderingHint::TRANSPARENT_BIN);
	//geom->getOrCreateStateSet()->setAttributeAndModes(new osg::Depth(osg::Depth::ALWAYS));
	geom->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

	auto geomGreen = mesh.createGeometry();
	auto mtGreen = new osg::MatrixTransform;
	mtGreen->setMatrix(osg::Matrix::translate(2.0, 0.0, 0.0));
	mtGreen->addChild(geomGreen);

	osg::ShapeDrawable* shape = new osg::ShapeDrawable(new osg::Box(osg::Vec3(), 100.0));
	shape->setCullingActive(false);

	auto hudCamera = createHudCamera();
	auto mt = new osg::MatrixTransform;
	mt->addChild(shape);
	hudCamera->addChild(mt);

	osg::Group* root = new osg::Group;
	//root->addChild(shape);
	root->addChild(hudCamera);
	root->addChild(geom);
	root->addChild(mtGreen);

	osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(100, 100, 640, 480);
	viewer->setThreadingModel(osgViewer::ViewerBase::ThreadingModel::DrawThreadPerContext);
	viewer->setCameraManipulator(new CustomizedManipulator);
	viewer->getCamera()->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(true);
	viewer->setSceneData(root);

	//viewer->getCamera()->addPostDrawCallback(new PostDrawCallback(geom));

	viewer->addEventHandler(new FollowMouseEventHandler(mt));

	viewer->run();
#endif
}

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQmlContext>
#include "interface.h"

#if 0
typedef void* CudaPtr;
/*

class Co
{
public:
	virtual void accessCudaPtr(std::function<void(CudaPtr*)> func) {
		QMutexLocker locker(&m_mutex);
		func(m_cudaPtr);
	}

private:
	CudaPtr* m_cudaPtr = nullptr;
	QMutex m_mutex;
};

/*!
* @brief 生成网格类
* @note 构造后必须调用\ref buildData 构造数据，并且override getData获取真正的数据
*/
class GenerateMesh
{
public:
	bool buildData() {
		getData();
		// process data
		// compute bound
		// compute vertex num
	}

	virtual void getData() {

	}
};

class MyGenerateMesh : public GenerateMesh
{
public:
	virtual void getData() override {
		// getdata from algo
	}
};
#endif

#if 0 // shared ptr
template<typename T>
class SharedPtr
{
public:
	SharedPtr(){}
	SharedPtr(T* data) {		
		if (data)
		{
			_data = data;
			_cnt = new int;
			*_cnt = 1;
		}			
	}
	SharedPtr(SharedPtr<T>& rhs) {
		if (this != &rhs && rhs._data) {
			_data = rhs._data;
			++*(rhs._cnt);
			_cnt = rhs._cnt;
		}
	}
	SharedPtr& operator = (SharedPtr<T>&rhs){
		if (this != &rhs) {
			if (_cnt)
			{
				if (*_cnt == 1)
				{
					delete _data;
					delete _cnt;
				}
				else 
					--* _cnt;
			}
			if (rhs._data)
			{
				_data = rhs._data;
				++* (rhs._cnt);
				_cnt = rhs._cnt;
			}
			else
				*_cnt = 0;
		}
	return *this;
	}
	~SharedPtr() {
		if (*_cnt == 1) {
			delete _data;
			_cnt = 0;
		}
		else
			--*_cnt;
	}

	int count() const {
		return *_cnt;
	}
private:
	T* _data = nullptr;
	int* _cnt = nullptr;
};

class Model
{
public:
	Model() {
		qDebug() << "default construction";
	}
	Model(const Model& model) {
		qDebug() << "copy construction";
	}
	Model(Model&& model) {
		qDebug() << "move construction";
	}

	void setVertices(const std::vector<int>& vertices) {
		qDebug() << "left ref";
		m_vertices = vertices;
	}
	void setVertices(const std::vector<int>&& vertices) {
		qDebug() << "right ref";
		m_vertices = std::move(vertices);
	}
private:
	std::vector<int> m_vertices;
};

void change(int&& data)
{
	data = 1;
	printf("changed data: %d", data);
}

#endif

#include <thread>
#include <iostream>
#include <Windows.h>

static std::mutex s_mutex;
int main(int argc, char** argv)
{
	Sleep(500);

	clock_t start = clock();

	std::vector<int> data;
	data.resize(100000000);

	std::vector<int> datacpy;

	qDebug() << "data ptr:" << data.data();
	qDebug() << "datacpy ptr:" << datacpy.data();

	std::vector<int>&& rref = std::move(data);
	datacpy = rref;
	datacpy = std::move(data);
	clock_t end1 = clock();
	qDebug() << "just move:" << end1 - start;

	qDebug() << "data ptr:" << data.data();
	qDebug() << "datacpy ptr:" << datacpy.data();

#if 0
	Model model;

	qDebug() << "--------------------------";
	Model other(Model());
	qDebug() << "dfsfsf:" << (void*)&other;
	//model.setVertices(std::move(data));
	model.setVertices(data);

	clock_t end = clock();
	qDebug() << "time:" << end - start;

	int a1 = 5;
	change(std::move(a1));
	a1 = 2;
	////////////////////
	{
		SharedPtr<int> pA(new int(5));
		SharedPtr<int> pB;
		pB = pA;

		printf("pA cnt is : %d", pA.count());
	}

	int a = 1;
	int b = 1;
	auto f = [=](int i)->int {
		int valueA = a;
		int valueB = b;
		s_mutex.lock();
		std::cout << valueA + valueB + i << std::endl;
		s_mutex.unlock();
		return a + b;
	};

	for (int i = 0; i < 10; ++i) {
		std::thread t(f, i);
		t.detach();
	}
	QThread::currentThread()->msleep(100000);
	return 0;
#endif

#if 0
	Co c;
	c.accessCudaPtr([](CudaPtr* ptr) {
		setPtrToAlgo();
		});

	MyGenerateMesh gm;
	gm.buildData();
#endif

	//runOSG();
	//return 0;
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
	
	osg::Matrix matrix = osg::Matrix::rotate(osg::PI_2 * 0.5, osg::X_AXIS) * osg::Matrix::translate(10, 5, 2);
		//* osg::Matrix::perspective(30, 1.5, 0.1, 10000);
	osg::Vec3 pos(1, 2, 3);
	osg::Vec3 final = pos * matrix;
	osg::Vec4 ff = osg::Vec4(pos, 1.0) * matrix;
	ff = ff / ff.w();

	return app.exec();
}
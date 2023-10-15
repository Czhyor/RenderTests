#include "interface.h"
#include "deferred_rendering.h"
#include <drawable.h>
#include <shader_manager.h>
#include <operation.h>
#include <common/io/read_model_file.h>
#include <engine/physical/pnode.h>
#include <customized_manipulator.h>
#include <osg/PolygonMode>
#include <osg/LineWidth>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/BufferIndexBinding>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgManipulator/Translate1DDragger>
#include <osgManipulator/RotateCylinderDragger>
#include <osgManipulator/Scale1DDragger>

#include <QImage>
#include <QOpenGLFunctions_4_5_Core>
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
#include <QQuickOpenGLUtils>
#include <QOpenGLVersionFunctionsFactory>
#endif

QDebug& operator<<(QDebug& dbg, const osg::Vec3d& v)
{
	dbg << "(" << v.x() << "," << v.y() << "," << v.z() << ")";
	return dbg;
}

static double GRAVITY_G = 9.807;



Interface::Interface()
{
	m_physicalEngine.reset(new Physical::PhysicalEngine);
}

Interface::~Interface()
{

}

void Interface::setRenderInfo(QObject* renderInfo)
{
	m_renderInfo.reset(new RenderInfo(*dynamic_cast<RenderInfo*>(renderInfo)));
}

void Interface::run()
{
	qDebug() << "Creator run";
}

void Interface::stop()
{
	qDebug() << "Creator stop";
}


void Interface::reset()
{
	qDebug() << "Creator reset";
}

void Interface::addMap(const QString& filePath)
{

}

osg::Geometry* createBillboardRectForTexture(osg::Texture2D* texture)
{
	osg::Geometry* geometry = new osg::Geometry;
	geometry->setUseDisplayList(false);
	geometry->setUseVertexBufferObjects(true);

	osg::Vec3Array* vArray = new osg::Vec3Array;
	vArray->push_back(osg::Vec3(0, 0, 0));

	class MyComputeBoundCallback : public osg::Geometry::ComputeBoundingBoxCallback
	{
	public:
		virtual osg::BoundingBox computeBound(const osg::Drawable&) const override {
			osg::BoundingSphere bs;
			bs.set(osg::Vec3(0, 0, 0), 3);
			osg::BoundingBox bb;
			bb.expandBy(bs);
			return bb;
		}
	};
	geometry->setComputeBoundingBoxCallback(new MyComputeBoundCallback);

	geometry->setVertexAttribArray(0, vArray, osg::Array::BIND_PER_VERTEX);

	geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
	osg::Uniform* texUniform = new osg::Uniform(osg::Uniform::SAMPLER_2D, "tex");
	texUniform->set(0);
	geometry->getOrCreateStateSet()->addUniform(texUniform);

	geometry->setCullingActive(false);
	geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vArray->getNumElements()));

	class MyDrawCallback : public osg::Drawable::DrawCallback
	{
	public:
		virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const override {
			auto camera = renderInfo.getCurrentCamera();
			osg::Matrixf vp = camera->getViewMatrix() * camera->getProjectionMatrix();
			auto view = dynamic_cast<osgViewer::View*>(renderInfo.getView());
			auto cm = dynamic_cast<osgGA::OrbitManipulator*>(view->getCameraManipulator());
			osg::Vec3d eye;
			osg::Quat rotation;
			cm->getTransformation(eye, rotation);

			osg::StateSet* ss = const_cast<osg::StateSet*>(drawable->getStateSet());
			ss->getUniform("vp")->set(vp);
			ss->getUniform("cameraPos")->set(osg::Vec3(eye));

			drawable->drawImplementation(renderInfo);
		}
	};
	osg::Uniform* vpUniform = new osg::Uniform("vp", osg::Matrixf());
	osg::Uniform* cameraPosUniform = new osg::Uniform("cameraPos", osg::Vec3f());
	geometry->getOrCreateStateSet()->addUniform(vpUniform);
	geometry->getOrCreateStateSet()->addUniform(cameraPosUniform);
	geometry->setDrawCallback(new MyDrawCallback);


	const char* vs = R"(
#version 330 core
layout(location = 0) in vec3 vPosition;

void main()
{
	gl_Position = vec4(vPosition, 1.0);
}
)";
	const char* gs = R"(
#version 330 core

layout(points) in; 
layout(triangle_strip) out;                                                                                                                           
layout(max_vertices = 4) out;  

uniform mat4 vp;
uniform vec3 cameraPos;

out vec2 texCoord;

void main()
{
	vec3 pos = gl_in[0].gl_Position.xyz;
	vec3 cameraToPos = normalize(pos - cameraPos);
	vec3 up = vec3(0.0, 0.0, 1.0);
	vec3 right = cross(up, cameraToPos);

	// bottom left
	gl_Position = vp * vec4(pos, 1.0);
	texCoord = vec2(0.0, 0.0);
	EmitVertex();	

	// top left
	pos.z += 1;
	gl_Position = vp * vec4(pos, 1.0);
	texCoord = vec2(0.0, 1.0);
	EmitVertex();

	// bottom right
	pos.z -= 1;
	pos += right;
	gl_Position = vp * vec4(pos, 1.0);
	texCoord = vec2(1.0, 0.0);
	EmitVertex();

	// top right
	pos.z += 1;
	gl_Position = vp * vec4(pos, 1.0);
	texCoord = vec2(1.0, 1.0);
	EmitVertex();

	EndPrimitive();
}
)";

	const char* fs = R"(
#version 330 core
uniform sampler2D colorMap;                                                        
                                                                                    
in vec2 texCoord;                                                                   
out vec4 FragColor;  

void main()
{
	FragColor = texture2D(colorMap, texCoord); 
}
)";
	osg::Program* program = new osg::Program;
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
	program->addShader(new osg::Shader(osg::Shader::GEOMETRY, gs));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
	geometry->getOrCreateStateSet()->setAttributeAndModes(program, osg::StateAttribute::ON);
	return geometry;
}

void Interface::addBillboard(const QString& filePath)
{
	osg::Image* image = osgDB::readImageFile("C:/Users/Administrator/Desktop/NewFolder/osgcmaketest/res/image/tree.png");
	osg::Texture2D* tex = new osg::Texture2D(image);
	auto geom = createBillboardRectForTexture(tex);

	Node* node = createObject<Node>();
	node->setObjectName("tree");
	node->addGeometry(geom);
	node->addToScene();
}

void Interface::addModel(const QString& filePath)
{
	ReadModelFile modelFile(filePath);
	modelFile.read();

	ModelData data = modelFile.getModelData();
	Mesh mesh;
	mesh.setModelData(data);
	auto geom = mesh.createGeometry();

	class TestDrawCallback : public osg::Drawable::DrawCallback
	{
	public:
		virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const override {
			auto func = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>();
			GLuint programID = renderInfo.getState()->getLastAppliedProgramObject()->getHandle();

			std::string str("pLightShadowVPs[1]");
			GLint loc = func->glGetUniformLocation(programID, str.data());

			//GLenum err = func->glGetError();
			//if (loc >= 0) {
			//	osg::Matrixf matrix = osg::Matrix::translate(10, 20, 30);
			//	std::vector<osg::Matrixf> vec;
			//	vec.resize(10, matrix);
			//	func->glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)vec.data());
			//}

			//err = func->glGetError();
			drawable->drawImplementation(renderInfo);
		}
	};
	geom->setDrawCallback(new TestDrawCallback);

	auto program = ShaderMgr::instance()->getShader(ShaderMgr::s_meshProgram);
	geom->getOrCreateStateSet()->setAttributeAndModes(program);

	Node* node = createObject<Node>();
	node->setObjectName(modelFile.getModelFileName());
	node->addGeometry(geom);
	node->setMaterial({
		osg::Vec3(0.5, 0.5, 0.5),
		osg::Vec3(0.5, 0.5, 0.5),
		osg::Vec3(0.5, 0.5, 0.5),
		32.0f
		});

	node->addToScene();

	std::shared_ptr<Physical::Mesh> pmesh(new Physical::Mesh);
	pmesh->m_pVec3 = data.m_vertexArray->asVector().data();
	pmesh->m_numPoints = data.m_vertexArray->getNumElements();
	const auto& osgBB = geom->getBoundingBox();
	pmesh->m_box.m_min = osgBB._min;// = Physical::Box(osgBB._min, osgBB._max);
	pmesh->m_box.m_max = osgBB._max;// = Physical::Box(osgBB._min, osgBB._max);
	qDebug() << "model's bound sphere:" << osgBB.center() << osgBB.radius();

	std::shared_ptr<Physical::Object> phyNode(new Physical::Object);
	phyNode->m_shape = pmesh;
	m_physicalEngine->addObject(phyNode);
	node->setPhysicalObject(phyNode);

	static int s_index = 0;
	if (s_index == 0) {
		phyNode->m_collideDetectLevel = Physical::CollideDetectLevel::Primitive;
	}
	s_index++;

	m_nodes.push_back(QSharedPointer<Node>(node));
	emit nodeAdded(node);
}

osg::Program* createSimpleProgram()
{
	const char* vs = R"(
#version 430 core
layout(location = 0) in vec4 Position;
layout(location = 1) in vec3 Normal;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat3 osg_NormalMatrix;

out vec3 vNormal;

void main()
{
	vNormal = osg_NormalMatrix * Normal;
	gl_Position = osg_ModelViewProjectionMatrix * Position;
}
)";

	const char* fs = R"(
#version 430 core
in vec3 vNormal;
out vec4 FragColor;
void main()
{
	vec3 lightColor = vec3(0.5, 0.5, 0.5) * (0.3 + dot(vNormal, vec3(0, 0, 1)));
	FragColor = vec4(lightColor, 1.0);
}
)";
	osg::Program* program = new osg::Program;
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
	return program;
}

osg::Program* createGeomProgram()
{
	const char* vs = R"(
#version 430 core
layout(location = 0) in vec4 Position;
layout(location = 1) in vec3 Normal;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat3 osg_NormalMatrix;

out VS_OUT {
	vec3 vNormal;
} vs_out;

void main()
{
	gl_Position = osg_ModelViewProjectionMatrix * Position;
	vs_out.vNormal = osg_NormalMatrix * Normal;
}
)";
	const char* gs = R"(
#version 430 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in VS_OUT {
	vec3 vNormal;
} gs_in[];

out vec3 vNormal;

void main() {
	vNormal = gs_in[0].vNormal;
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();

	vNormal = gs_in[1].vNormal;
	gl_Position = gl_in[1].gl_Position;
	EmitVertex();

	vNormal = gs_in[2].vNormal;
	gl_Position = gl_in[2].gl_Position;
	EmitVertex();

	EndPrimitive();
}
)";

	const char* fs = R"(
#version 430 core
in vec3 vNormal;
out vec4 FragColor;
void main()
{
	vec3 lightColor = vec3(1.0, 0.5, 0.5) * (0.3 + dot(vNormal, vec3(0, 0, 1)));
	FragColor = vec4(lightColor, 1.0);
}
)";
	osg::Program* program = new osg::Program;
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
	program->addShader(new osg::Shader(osg::Shader::GEOMETRY, gs));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
	return program;
}

void Interface::addCustomized()
{
	osg::Geometry* geom = new osg::Geometry;
	geom->setUseDisplayList(false);
	geom->setUseVertexBufferObjects(true);

	osg::Vec3Array* vArray = new osg::Vec3Array;
	osg::Vec3Array* nArray = new osg::Vec3Array;
	int width = 4500;
	int height = 3000;
	int depth = 1;
	for (int z = 0; z < depth; ++z) {

		osg::ref_ptr<osg::Vec3Array> tmpVArray = new osg::Vec3Array;
		const auto& vecTmpV = tmpVArray->asVector();

		for (int j = 0; j < height; ++j) {
			for (int i = 0; i < width; ++i) {
				osg::Vec3 pos(i, j, z);
				tmpVArray->push_back(pos);
			}
		}

		for (int j = 0; j < (height - 1); ++j) {
			for (int i = 0; i < (width - 1); ++i) {
				vArray->push_back(vecTmpV[j * width + i]);
				vArray->push_back(vecTmpV[j * width + (i + 1)]);
				vArray->push_back(vecTmpV[(j + 1) * width + i]);

				vArray->push_back(vecTmpV[(j + 1) * width + i]);
				vArray->push_back(vecTmpV[j * width + (i + 1)]);
				vArray->push_back(vecTmpV[(j + 1) * width + (i + 1)]);

				nArray->push_back(osg::Vec3(0, 0, 1));
				nArray->push_back(osg::Vec3(0, 0, 1));
				nArray->push_back(osg::Vec3(0, 0, 1));
				nArray->push_back(osg::Vec3(0, 0, 1));
				nArray->push_back(osg::Vec3(0, 0, 1));
				nArray->push_back(osg::Vec3(0, 0, 1));
			}
		}

	}
	geom->setVertexAttribArray(0, vArray, osg::Array::BIND_PER_VERTEX);
	geom->setVertexAttribArray(1, nArray, osg::Array::BIND_PER_VERTEX);
	geom->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vArray->getNumElements()));

	//geom->getOrCreateStateSet()->setAttributeAndModes(createSimpleProgram());
	geom->getOrCreateStateSet()->setAttributeAndModes(createGeomProgram());
	//geom->getOrCreateStateSet()->setAttributeAndModes(
	//	new osg::PolygonMode(osg::PolygonMode::Face::BACK, osg::PolygonMode::Mode::LINE));
	auto view = m_renderInfo->m_mainView;
	m_renderInfo->addOperation(new LambdaOperation([view, geom]() {
		auto modelGroup = ViewInfo::getModelGroup(view);
		modelGroup->addChild(geom);
		view->home();

		auto cm = dynamic_cast<osgGA::OrbitManipulator*>(view->getCameraManipulator());
		cm->setRotation(osg::Quat(osg::PI_2f, osg::X_AXIS));
		cm->setRotation(osg::Quat());
		}));

}

void Interface::setCameraFollowNode(Node* node)
{
	auto view = m_renderInfo->m_mainView;
	m_renderInfo->addOperation(new LambdaOperation([view]() {
		view->setCameraManipulator(new FirstPersionManipulator(osg::Vec3(), osg::Vec3()));
		}));
}

static osg::Vec3d s_defaultFrontDir = osg::Vec3d(0.0, -1.0, 0.0);
static osg::Vec3d s_defaultLeftDir = osg::Vec3d(1.0, 0.0, 0.0);
static osg::Vec3d s_defaultUpDir = osg::Vec3d(0.0, 0.0, 1.0);

void Interface::enableManipulate(Node* node)
{
	class ManipulateNodeEventHandler : public osgGA::GUIEventHandler
	{
	public:
		ManipulateNodeEventHandler(std::shared_ptr<Physical::PhysicalEngine> engine) :
			m_phyEngine(engine)
		{

		}

		void setNodeInfo(osg::MatrixTransform* mt, std::shared_ptr<Physical::Object> obj) {
			m_mt = mt;
			m_phyObj = obj;
		}

		virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override {
			if (ea.getHandled()) {
				return false;
			}

			if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN) {
				switch (ea.getKey())
				{
				case osgGA::GUIEventAdapter::KeySymbol::KEY_W: {
					handleMoveForward();
					break;
				}
				case osgGA::GUIEventAdapter::KeySymbol::KEY_A: {
					handleMoveLeft();
					break;
				}
				case osgGA::GUIEventAdapter::KeySymbol::KEY_S: {
					handleMoveBack();
					break;
				}
				case osgGA::GUIEventAdapter::KeySymbol::KEY_D: {
					handleMoveRight();
					break;
				}
				default:
					return false;
				}
				return true;
			}
			return false;
		}

	protected:
		void handleMoveForward()
		{
			osg::Matrix matrix = m_mt->getMatrix();
			osg::Vec3d trans = matrix.getTrans();
			osg::Vec3 offset = s_defaultFrontDir * m_step;

			m_phyEngine->testObject(m_phyObj, offset);

			trans += offset;
			matrix.setTrans(trans);
			m_mt->setMatrix(matrix);
			m_phyObj->m_matrix = matrix;
		}

		void handleMoveBack()
		{
			osg::Matrix matrix = m_mt->getMatrix();
			osg::Vec3d trans = matrix.getTrans();
			osg::Vec3 offset = -s_defaultFrontDir * m_step;

			m_phyEngine->testObject(m_phyObj, offset);

			trans += offset;
			matrix.setTrans(trans);
			m_mt->setMatrix(matrix);
			m_phyObj->m_matrix = matrix;
		}

		void handleMoveLeft()
		{
			osg::Matrix matrix = m_mt->getMatrix();
			osg::Vec3d trans = matrix.getTrans();
			osg::Vec3 offset = s_defaultLeftDir * m_step;

			m_phyEngine->testObject(m_phyObj, offset);

			trans += offset;
			matrix.setTrans(trans);
			m_mt->setMatrix(matrix);
			m_phyObj->m_matrix = matrix;
		}

		void handleMoveRight()
		{
			osg::Matrix matrix = m_mt->getMatrix();
			osg::Vec3d trans = matrix.getTrans();
			osg::Vec3 offset = -s_defaultLeftDir * m_step;

			m_phyEngine->testObject(m_phyObj, offset);

			trans += offset;
			matrix.setTrans(trans);
			m_mt->setMatrix(matrix);
			m_phyObj->m_matrix = matrix;
		}

	protected:
		osg::ref_ptr<osg::MatrixTransform> m_mt;
		std::shared_ptr<Physical::Object> m_phyObj;

		double m_step = 0.1;

		std::shared_ptr<Physical::PhysicalEngine> m_phyEngine;
	};

	auto mt = node->getMatrixTransform();
	auto phyNode = node->getPhysicalObject();
	auto phyEngine = m_physicalEngine;
	auto view = m_renderInfo->m_mainView;
	m_renderInfo->addOperation(new LambdaOperation([mt, phyNode, view, phyEngine]() {
		auto& handlers = view->getEventHandlers();
		
		for (auto& handler : handlers) {
			auto mneh = dynamic_cast<ManipulateNodeEventHandler*>(handler.get());
			if (mneh) {
				mneh->setNodeInfo(mt, phyNode);
				return;
			}
		}

		auto h = new ManipulateNodeEventHandler(phyEngine);
		h->setNodeInfo(mt, phyNode);
		view->addEventHandler(h);
		}));
}

osg::Geode* createZUnitAxisWithArrow(const osg::Vec4& color, bool useLineForCylinder = true, float cylinderRadiusRatio = 0.03)
{
	float height = 1.0;
	float coneHeightRatio = 0.2;
	float coneHeight = height * coneHeightRatio;
	osg::ShapeDrawable* cone = new osg::ShapeDrawable(new osg::Cone(osg::Vec3(0.0, 0.0, height * (1 - coneHeightRatio * 0.5)), coneHeight * 0.3, coneHeight));
	cone->setColor(color);
	cone->getOrCreateStateSet()->setAttributeAndModes(
		new osg::PolygonMode(osg::PolygonMode::Face::FRONT_AND_BACK, osg::PolygonMode::Mode::LINE));

	osg::Geode* geode = new osg::Geode;
	geode->addDrawable(cone);

	if (useLineForCylinder) {
		osg::Geometry* lineGeometry = new osg::Geometry;
		lineGeometry->setUseDisplayList(false);
		lineGeometry->setUseVertexBufferObjects(true);
		osg::Vec3Array* lineVertexArray = new osg::Vec3Array;
		lineVertexArray->push_back(osg::Vec3());
		lineVertexArray->push_back(osg::Vec3(0.0, 0.0, 1.0));
		osg::Vec4Array* lineColorArray = new osg::Vec4Array;
		lineColorArray->push_back(color);
		lineGeometry->setVertexArray(lineVertexArray);
		lineGeometry->setColorArray(lineColorArray, osg::Array::BIND_OVERALL);
		lineGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, lineVertexArray->getNumElements()));
		lineGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		geode->addDrawable(lineGeometry);
	}
	else {
		osg::ShapeDrawable* lineDrawable = new osg::ShapeDrawable(new osg::Cylinder(osg::Vec3(0.0, 0.0, 0.5), cylinderRadiusRatio, 1));
		lineDrawable->setColor(color);
		lineDrawable->setUseDisplayList(false);
		lineDrawable->setUseVertexBufferObjects(true);
		lineDrawable->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		geode->addDrawable(lineDrawable);
	}

	return geode;
}


void Interface::setDeferredRendering()
{
	auto view = m_renderInfo->m_mainView;
	std::vector<osg::ref_ptr<osg::Geometry>> geoms;
	for (auto node : m_nodes) {
		geoms.push_back(node->getGeometry());
	}

	m_renderInfo->addOperation(new LambdaOperation([view, geoms]() {
		osg::Camera* viewCamera = view->getCamera();
		osg::Viewport* viewport = viewCamera->getViewport();

		osg::ref_ptr<osg::Camera> camera = createRTTCamera(viewport->width(), viewport->height());
		osg::ref_ptr<osg::Texture2D> baseColorTex = createTexture(viewport->width(), viewport->height());
		osg::ref_ptr<osg::Texture2D> normalTex = createTexture(viewport->width(), viewport->height());
		osg::ref_ptr<osg::Texture2D> posTex = createTexture(viewport->width(), viewport->height());
		osg::ref_ptr<osg::Texture2D> testTex = createTexture(viewport->width(), viewport->height());
		osg::ref_ptr<osg::Texture2D> depthTex = createDepthTexture(viewport->width(), viewport->height());
		camera->attach(osg::Camera::BufferComponent::COLOR_BUFFER0, baseColorTex);
		camera->attach(osg::Camera::BufferComponent::COLOR_BUFFER1, normalTex);
		camera->attach(osg::Camera::BufferComponent::COLOR_BUFFER2, posTex);
		camera->attach(osg::Camera::BufferComponent::COLOR_BUFFER3, testTex);
		camera->attach(osg::Camera::BufferComponent::DEPTH_BUFFER, depthTex);

		osg::Geometry* rectGeometry = createRectForTexture(baseColorTex, normalTex, depthTex);

		osg::ref_ptr<osg::Switch> modelGroup = ViewInfo::getModelGroup(view);
		auto root = ViewInfo::getRoot(view);
		root->removeChild(modelGroup);

		camera->addChild(modelGroup);
		root->addChild(camera);
		root->addChild(rectGeometry);

		auto deferedProgram = ShaderMgr::instance()->getShader(ShaderMgr::s_deferedMeshProgram);
		for (auto& geom : geoms) {
			geom->getOrCreateStateSet()->setAttributeAndModes(deferedProgram, osg::StateAttribute::ON);
		}

		}));

}

void Interface::setForwardRendering()
{
	auto view = m_renderInfo->m_mainView;
	std::vector<osg::ref_ptr<osg::Geometry>> geoms;
	for (auto node : m_nodes) {
		geoms.push_back(node->getGeometry());
	}

	m_renderInfo->addOperation(new LambdaOperation([view, geoms]() {
		osg::Camera* viewCamera = view->getCamera();
		osg::Viewport* viewport = viewCamera->getViewport();

		osg::ref_ptr<osg::Switch> modelGroup = ViewInfo::getModelGroup(view);
		auto root = ViewInfo::getRoot(view);

		while (root->getNumChildren() > 0) {
			root->removeChild(0, 1);
		}
		root->addChild(modelGroup);

		auto program = ShaderMgr::instance()->getShader(ShaderMgr::s_meshProgram);
		for (auto& geom : geoms) {
			geom->getOrCreateStateSet()->setAttributeAndModes(program, osg::StateAttribute::ON);
		}
		}));
	
}


#define VIEW_DATUM_NODE_NAME "viewDatumNode"
osg::Group* createViewDatumNode()
{
	osg::Geode* xAxis = createZUnitAxisWithArrow(osg::Vec4(1.0, 0.0, 0.0, 1.0), false);
	osg::Geode* yAxis = createZUnitAxisWithArrow(osg::Vec4(0.0, 1.0, 0.0, 1.0));
	osg::Geode* zAxis = createZUnitAxisWithArrow(osg::Vec4(0.0, 0.0, 1.0, 1.0));

	osg::MatrixTransform* xMt = new osg::MatrixTransform;
	xMt->setMatrix(osg::Matrix::rotate(osg::PI_2f, osg::Y_AXIS));
	xMt->addChild(xAxis);
	osg::MatrixTransform* yMt = new osg::MatrixTransform;
	yMt->setMatrix(osg::Matrix::rotate(-osg::PI_2f, osg::X_AXIS));
	yMt->addChild(yAxis);

	osg::Group* group = new osg::Group;
	group->addChild(xMt);
	group->addChild(yMt);
	group->addChild(zAxis);

	class FollowViewMatrixCamera : public osg::Camera
	{
	public:
		virtual void accept(osg::NodeVisitor& nv) override {
			if (nv.asCullVisitor()) {
				const auto& nodePath = nv.getNodePath();
				const auto& viewMatrix = nodePath.front()->getParent(0)->asCamera()->getViewMatrix();
				osg::Quat rot = viewMatrix.getRotate();
				setViewMatrix(osg::Matrix::rotate(rot));
			}
			osg::Camera::accept(nv);
		}
	};

	osg::Camera* camera = new FollowViewMatrixCamera;
	camera->setAllowEventFocus(false);
	camera->setReferenceFrame(osg::Transform::ReferenceFrame::ABSOLUTE_RF);
	float range = 1.2;
	camera->setProjectionMatrixAsOrtho(-range, range, -range, range, -range, range);
	camera->setViewport(0, 0, 100, 100);
	camera->setCullingMode(osg::CullSettings::CullingModeValues::NO_CULLING);
	camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

	camera->setClearColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));
	camera->setClearDepth(1.0);
	camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	camera->addChild(group);
	return camera;
}


void Interface::enableViewDatum()
{
	auto view = m_renderInfo->m_mainView;
	m_renderInfo->addOperation(new LambdaOperation([view]() {
		auto otherGroup = ViewInfo::getOtherGroup(view);
		for (int i = 0; i < otherGroup->getNumChildren(); ++i) {
			if (otherGroup->getChild(i)->getName() == VIEW_DATUM_NODE_NAME) {
				return;
			}
		}
		auto viewDatumNode = createViewDatumNode();
		viewDatumNode->setName(VIEW_DATUM_NODE_NAME);
		otherGroup->addChild(viewDatumNode);
		}));
}

void Interface::enableIntersect(bool enable)
{
	class IntersectEventHandler : public osgGA::GUIEventHandler
	{
	public:
		virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, osg::Object* obj, osg::NodeVisitor* nv) {
			if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH) {
				osgUtil::LineSegmentIntersector* intersector = new osgUtil::LineSegmentIntersector(
					osgUtil::Intersector::CoordinateFrame::WINDOW, ea.getX(), ea.getY());
				class MyIntersectionVisitor : public osgUtil::IntersectionVisitor
				{
				public:
					MyIntersectionVisitor(osgUtil::Intersector* intersector = 0, ReadCallback* readCallback = 0):
						osgUtil::IntersectionVisitor(intersector, readCallback) {}
					void apply(osg::Camera* camera);
				};
				MyIntersectionVisitor iv(intersector);
				aa.asView()->getCamera()->accept(iv);
				if (intersector->containsIntersections()) {
					osgUtil::LineSegmentIntersector::Intersection firstIntersectoin =  intersector->getFirstIntersection();
					qDebug() << "intersect drawable:" << firstIntersectoin.drawable;
					qDebug() << "intersect primitiveIndex:" << firstIntersectoin.primitiveIndex;
				}

			}

			return false;
		}
	};
	auto view = m_renderInfo->m_mainView;
	m_renderInfo->addOperation(new LambdaOperation([enable, view]() {
		if (enable) {
			view->addEventHandler(new IntersectEventHandler);
		}
		else {
			const auto handlers = view->getEventHandlers();
			for (auto& handler : handlers) {
				auto intersectHandler = dynamic_cast<IntersectEventHandler*>(handler.get());
				if (intersectHandler) {
					view->removeEventHandler(intersectHandler);
				}
			}
		}
		}));
}

osg::Image* convertQImage2OsgImage(const QString& filePath)
{
	QImage qImage(filePath);
	if (qImage.isNull()) {
		return nullptr;
	}

	GLenum pixelFormat = 0;
	GLenum pixelType = 0;

	qImage = qImage.convertToFormat(QImage::Format_RGBA8888);
	qImage.mirror(false, true);
	QImage::Format format = qImage.format();
	switch (format)
	{
	case QImage::Format::Format_RGB888: {
		pixelFormat = GL_RGB;
		pixelType = GL_UNSIGNED_BYTE;
		break;
	}
	case QImage::Format::Format_RGB32: {
		pixelFormat = GL_RGB;
		pixelType = GL_FLOAT;
		break;
	}
	case QImage::Format::Format_RGBA8888: {
		pixelFormat = GL_RGBA;
		pixelType = GL_UNSIGNED_BYTE;
		break;
	}
	default:
		qDebug() << "unknown QImage format:" << format;
		return nullptr;
	}

	osg::Image* osgImage = new osg::Image;
	osgImage->allocateImage(qImage.width(), qImage.height(), 1, pixelFormat, pixelType);
	memcpy(osgImage->data(), qImage.bits(), qImage.sizeInBytes());

	return osgImage;
}

osg::Geometry* createRectShowTexture(osg::Texture2D* texture)
{
	osg::Geometry* geometry = new osg::Geometry;
	geometry->setUseDisplayList(false);
	geometry->setUseVertexBufferObjects(true);

	osg::Vec3Array* vArray = new osg::Vec3Array;
	vArray->push_back(osg::Vec3(-1, -1, 0));
	vArray->push_back(osg::Vec3(1, -1, 0));
	vArray->push_back(osg::Vec3(-1, 1, 0));
	vArray->push_back(osg::Vec3(1, 1, 0));

	osg::Vec2Array* uvArray = new osg::Vec2Array;
	uvArray->push_back(osg::Vec2(0, 0));
	uvArray->push_back(osg::Vec2(1, 0));
	uvArray->push_back(osg::Vec2(0, 1));
	uvArray->push_back(osg::Vec2(1, 1));

	geometry->setVertexAttribArray(0, vArray, osg::Array::BIND_PER_VERTEX);
	geometry->setVertexAttribArray(1, uvArray, osg::Array::BIND_PER_VERTEX);
	
	geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
	osg::Uniform* texUniform = new osg::Uniform(osg::Uniform::SAMPLER_2D, "tex");
	texUniform->set(0);
	geometry->getOrCreateStateSet()->addUniform(texUniform);

	geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vArray->getNumElements()));


	const char* vs = R"(
#version 330 core
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vUV;

uniform mat4 osg_ModelViewProjectionMatrix;
out vec2 uv;

void main()
{
	gl_Position = osg_ModelViewProjectionMatrix * vPosition;
	uv = vUV;
}
)";
	const char* fs = R"(
#version 330 core
uniform sampler2D tex;

in vec2 uv;
out vec4 FragColor;

void main()
{
	vec4 color = texture(tex, uv).rgba;
	FragColor = vec4(color.rgb, 1.0);

}
)";
	osg::Program* program = new osg::Program;
	program->setName("showTexture");
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
	geometry->getOrCreateStateSet()->setAttributeAndModes(program, osg::StateAttribute::ON);
	return geometry;
}

osg::Geode* createCubeMapRect(osg::Texture2D* negx, osg::Texture2D* negy, osg::Texture2D* negz,
	osg::Texture2D* posx, osg::Texture2D* posy, osg::Texture2D* posz)
{
	auto createGeometry = [](osg::Vec3Array* vArray, osg::Texture2D* texture) {
		osg::Geometry* geometry = new osg::Geometry;
		geometry->setUseDisplayList(false);
		geometry->setUseVertexBufferObjects(true);

		osg::Vec2Array* uvArray = new osg::Vec2Array;
		uvArray->push_back(osg::Vec2(0, 0));
		uvArray->push_back(osg::Vec2(1, 0));
		uvArray->push_back(osg::Vec2(0, 1));
		uvArray->push_back(osg::Vec2(1, 1));

		geometry->setVertexAttribArray(0, vArray, osg::Array::BIND_PER_VERTEX);
		geometry->setVertexAttribArray(1, uvArray, osg::Array::BIND_PER_VERTEX);

		geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
		osg::Uniform* texUniform = new osg::Uniform(osg::Uniform::SAMPLER_2D, "tex");
		texUniform->set(0);
		geometry->getOrCreateStateSet()->addUniform(texUniform);

		class CubeMapVUpdateCallback : public osg::UniformCallback
		{
		public:
			virtual void operator () (osg::Uniform* uniform, osg::NodeVisitor* nv) override {
				auto camera = nv->getNodePath().front()->getParent(0)->asCamera();
				auto view = dynamic_cast<osgViewer::View*>(camera->getView());
				auto cm = dynamic_cast<osgGA::OrbitManipulator*>(view->getCameraManipulator());
				osg::Matrixf vMatrix = cm->getInverseMatrix();
				vMatrix.setTrans(0, 0, 0);
				uniform->set(vMatrix);
			}
		};
		osg::Uniform* cubeMapV = new osg::Uniform("cubeMapV", osg::Matrixf());
		cubeMapV->setUpdateCallback(new CubeMapVUpdateCallback);
		geometry->getOrCreateStateSet()->addUniform(cubeMapV);

		geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vArray->getNumElements()));

		class EmptyBoundbox : public osg::Drawable::ComputeBoundingBoxCallback
		{
		public:
			virtual osg::BoundingBox computeBound(const osg::Drawable&) const { return osg::BoundingBox(); }
		};
		geometry->setComputeBoundingBoxCallback(new EmptyBoundbox);

		return geometry;
	};

	osg::Geode* geode = new osg::Geode;
	{ // left
		osg::Vec3Array* vArray = new osg::Vec3Array;
		vArray->push_back(osg::Vec3(-1, -1, -1));
		vArray->push_back(osg::Vec3(-1, 1, -1));
		vArray->push_back(osg::Vec3(-1, -1, 1));
		vArray->push_back(osg::Vec3(-1, 1, 1));
		geode->addDrawable(createGeometry(vArray, negx));
	}
	{ // back
		osg::Vec3Array* vArray = new osg::Vec3Array;
		vArray->push_back(osg::Vec3(1, -1, -1));
		vArray->push_back(osg::Vec3(-1, -1, -1));
		vArray->push_back(osg::Vec3(1, -1, 1));
		vArray->push_back(osg::Vec3(-1, -1, 1));
		geode->addDrawable(createGeometry(vArray, negz));
	}
	{ // front
		osg::Vec3Array* vArray = new osg::Vec3Array;
		vArray->push_back(osg::Vec3(-1, 1, -1));
		vArray->push_back(osg::Vec3(1, 1, -1));
		vArray->push_back(osg::Vec3(-1, 1, 1));
		vArray->push_back(osg::Vec3(1, 1, 1));
		geode->addDrawable(createGeometry(vArray, posz));
	}
	{ // right
		osg::Vec3Array* vArray = new osg::Vec3Array;
		vArray->push_back(osg::Vec3(1, 1, -1));
		vArray->push_back(osg::Vec3(1, -1, -1));
		vArray->push_back(osg::Vec3(1, 1, 1));
		vArray->push_back(osg::Vec3(1, -1, 1));
		geode->addDrawable(createGeometry(vArray, posx));
	}
	{ // top
		osg::Vec3Array* vArray = new osg::Vec3Array;
		vArray->push_back(osg::Vec3(-1, 1, 1));
		vArray->push_back(osg::Vec3(1, 1, 1));
		vArray->push_back(osg::Vec3(-1, -1, 1));
		vArray->push_back(osg::Vec3(1, -1, 1));
		geode->addDrawable(createGeometry(vArray, posy));
	}
	{ // bottom
		osg::Vec3Array* vArray = new osg::Vec3Array;
		vArray->push_back(osg::Vec3(-1, -1, -1));
		vArray->push_back(osg::Vec3(1, -1, -1));
		vArray->push_back(osg::Vec3(-1, 1, -1));
		vArray->push_back(osg::Vec3(1, 1, -1));
		geode->addDrawable(createGeometry(vArray, negy));
	}

	const char* vs = R"(
#version 330 core
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vUV;

uniform mat4 cubeMapV;
uniform mat4 osg_ProjectionMatrix;
out vec2 uv;

void main()
{
	vec4 pos = osg_ProjectionMatrix * cubeMapV * vPosition;
	gl_Position = pos.xyww;
	uv = vUV;
}
)";
	const char* fs = R"(
#version 330 core
uniform sampler2D tex;

in vec2 uv;
out vec4 FragColor;

void main()
{
	vec4 color = texture(tex, uv).rgba;
	FragColor = vec4(color.rgb, 1.0);

}
)";
	osg::Program* program = new osg::Program;
	program->setName("cubemap");
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
	geode->getOrCreateStateSet()->setAttributeAndModes(program, osg::StateAttribute::ON);
	geode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
	return geode;
}

void Interface::addCubeMap()
{
	qDebug() << "Call addCubeMap";
	osg::Image* negx = convertQImage2OsgImage(":/cubemap/Yokohama2/negx.jpg");
	osg::Image* negy = convertQImage2OsgImage(":/cubemap/Yokohama2/negy.jpg");
	osg::Image* negz = convertQImage2OsgImage(":/cubemap/Yokohama2/negz.jpg");
	osg::Image* posx = convertQImage2OsgImage(":/cubemap/Yokohama2/posx.jpg");
	osg::Image* posy = convertQImage2OsgImage(":/cubemap/Yokohama2/posy.jpg");
	osg::Image* posz = convertQImage2OsgImage(":/cubemap/Yokohama2/posz.jpg");

	auto createCubeTex = [](osg::Image* image) {
		osg::Texture2D* texture = new osg::Texture2D;
		texture->setImage(image);
		texture->setInternalFormat(GL_RGBA);
		texture->setWrap(osg::Texture::WRAP_S, osg::Texture::WrapMode::CLAMP_TO_EDGE);
		texture->setWrap(osg::Texture::WRAP_T, osg::Texture::WrapMode::CLAMP_TO_EDGE);
		return texture;
	};

	osg::Texture2D* tex_negx = createCubeTex(negx);
	osg::Texture2D* tex_negy = createCubeTex(negy);
	osg::Texture2D* tex_negz = createCubeTex(negz);
	osg::Texture2D* tex_posx = createCubeTex(posx);
	osg::Texture2D* tex_posy = createCubeTex(posy);
	osg::Texture2D* tex_posz = createCubeTex(posz);

	//bool isWrite = osgDB::writeImageFile(*negx, "./negx.png");

	osg::Geode* geode = createCubeMapRect(tex_negx, tex_negy, tex_negz, tex_posx, tex_posy, tex_posz);

	auto view = m_renderInfo->m_mainView;

	m_renderInfo->addOperation(new LambdaOperation([view, geode]() {
		auto otherGroup = ViewInfo::getModelGroup(view);
		otherGroup->addChild(geode);
		view->home();
		}));

}

void Interface::addDirectionalLight()
{
	DirectionalLight* light = createObject<DirectionalLight>();
	light->setObjectName("directionalLight");
	light->setEmissionColor(osg::Vec3(1.0, 1.0, 1.0));
	light->addToScene();

	m_lights.push_back(QSharedPointer<Light>(light));
	emit lightAdded(light);
}

void Interface::addPointLight()
{
	PointLight* light = createObject<PointLight>();
	light->setObjectName("pointLight");
	light->setEmissionColor(osg::Vec3(0.0, 1.0, 0.0));
	light->addToScene();

	m_lights.push_back(QSharedPointer<Light>(light));
	emit lightAdded(light);
}

void Interface::addSpotLight()
{
	SpotLight* light = createObject<SpotLight>();
	light->setObjectName("spotLight");
	light->setEmissionColor(osg::Vec3(0.0, 1.0, 1.0));
	light->addToScene();

	m_lights.push_back(QSharedPointer<Light>(light));
	emit lightAdded(light);
}

class DragEventHandler : public osgGA::GUIEventHandler
{
public:
	DragEventHandler(osg::MatrixTransform* mt) : m_mt(mt) {}

	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) {
		return false;
	}

protected:
	osg::ref_ptr<osg::MatrixTransform> m_mt;
};

// create fan with radius range between minR and maxR, with angle range between 0(posY) and endAngle(clock order)
osg::Geometry* createRangeFan(float minR, float maxR, float endAngle, const osg::Vec4& color)
{
	float stepAngle = 1;
	osg::Vec3 startMin = osg::Vec3(0, minR, 0);
	osg::Vec3 startMax = osg::Vec3(0, maxR, 0);

	osg::Vec3Array* vArray = new osg::Vec3Array;
	vArray->push_back(startMax);
	vArray->push_back(startMin);
	osg::DrawElementsUShort* drawElement = new osg::DrawElementsUShort(GL_TRIANGLE_STRIP);
	drawElement->push_back(0);
	drawElement->push_back(1);
	for (float curAngle = 0; curAngle <= endAngle; curAngle += stepAngle) {
		osg::Matrix rotate = osg::Matrix::rotate(-osg::DegreesToRadians(curAngle), osg::Z_AXIS);
		osg::Vec3 nextMax = startMax * rotate;
		osg::Vec3 nextMin = startMin * rotate;
		vArray->push_back(nextMax);
		drawElement->push_back(vArray->size() - 1);
		vArray->push_back(nextMin);
		drawElement->push_back(vArray->size() - 1);
	}
	osg::Vec4Array* cArray = new osg::Vec4Array;
	cArray->push_back(color);

	osg::Geometry* geometry = new osg::Geometry;
	geometry->setUseDisplayList(false);
	geometry->setUseVertexBufferObjects(true);
	geometry->setVertexArray(vArray);
	geometry->setColorArray(cArray, osg::Array::Binding::BIND_OVERALL);
	geometry->addPrimitiveSet(drawElement);
	geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

	return geometry;
}

class NodeDragger : public osgManipulator::CompositeDragger
{
public:
	NodeDragger(osg::MatrixTransform* mt, osg::BoundingBox bb = osg::BoundingBox()) : m_mt(mt), m_bb(bb) {

	}

	virtual void setupDefaultGeometry() override {
		if (!m_bb.valid()) {
			m_bb.expandBy(m_mt->getBound());
			if (!m_bb.valid()) {
				m_bb.set(osg::Vec3(-1, -1, -1), osg::Vec3(1, 1, 1));
			}
		}
		// crate translate dragger
		osg::Geode* xAxis = createZUnitAxisWithArrow(osg::Vec4(1.0, 0.0, 0.0, 1.0), false, 0.02);
		osg::Geode* yAxis = createZUnitAxisWithArrow(osg::Vec4(0.0, 1.0, 0.0, 1.0), false, 0.02);
		osg::Geode* zAxis = createZUnitAxisWithArrow(osg::Vec4(0.0, 0.0, 1.0, 1.0), false, 0.02);
		osg::MatrixTransform* xMt = new osg::MatrixTransform;
		xMt->setMatrix(osg::Matrix::rotate(osg::PI_2f, osg::Y_AXIS));
		xMt->addChild(xAxis);
		osg::MatrixTransform* yMt = new osg::MatrixTransform;
		yMt->setMatrix(osg::Matrix::rotate(osg::PI_2f, osg::Y_AXIS));
		yMt->addChild(yAxis);
		osg::MatrixTransform* zMt = new osg::MatrixTransform;
		zMt->setMatrix(osg::Matrix::rotate(osg::PI_2f, osg::Y_AXIS));
		zMt->addChild(zAxis);

		osg::ref_ptr<osgManipulator::Translate1DDragger> transXDragger = new osgManipulator::Translate1DDragger;
		transXDragger->setHandleEvents(true);
		transXDragger->addChild(xMt);

		osg::ref_ptr<osgManipulator::Translate1DDragger> transYDragger = new osgManipulator::Translate1DDragger;
		transYDragger->setHandleEvents(true);
		transYDragger->setMatrix(osg::Matrix::rotate(osg::PI_2f, osg::Z_AXIS));
		transYDragger->addChild(yMt);

		osg::ref_ptr<osgManipulator::Translate1DDragger> transZDragger = new osgManipulator::Translate1DDragger;
		transZDragger->setHandleEvents(true);
		transZDragger->setMatrix(osg::Matrix::rotate(-osg::PI_2f, osg::Y_AXIS));
		transZDragger->addChild(zMt);

		// create rotate dragger
		osg::Geometry* rotZ_rangeFan = createRangeFan(0.6, 1.0, 90, osg::Vec4(0.0, 0.0, 1.0, 1.0));
		osg::Geometry* rotX_rangeFan = createRangeFan(0.6, 1.0, 90, osg::Vec4(1.0, 0.0, 0.0, 1.0));
		osg::Geometry* rotY_rangeFan = createRangeFan(0.6, 1.0, 90, osg::Vec4(0.0, 1.0, 0.0, 1.0));

		osg::ref_ptr<osgManipulator::RotateCylinderDragger> rotateZDragger = new osgManipulator::RotateCylinderDragger;
		rotateZDragger->setHandleEvents(true);
		rotateZDragger->addChild(rotZ_rangeFan);

		osg::ref_ptr<osgManipulator::RotateCylinderDragger> rotateXDragger = new osgManipulator::RotateCylinderDragger;
		rotateXDragger->setHandleEvents(true);
		rotateXDragger->setMatrix(osg::Matrix::rotate(-osg::PI_2, osg::Y_AXIS));
		rotateXDragger->addChild(rotX_rangeFan);

		osg::ref_ptr<osgManipulator::RotateCylinderDragger> rotateYDragger = new osgManipulator::RotateCylinderDragger;
		rotateYDragger->setHandleEvents(true);
		rotateYDragger->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
		rotateYDragger->addChild(rotY_rangeFan);

		addDragger(transXDragger);
		addDragger(transYDragger);
		addDragger(transZDragger);
		addDragger(rotateZDragger);
		addDragger(rotateXDragger);
		addDragger(rotateYDragger);

		addChild(transXDragger);
		addChild(transYDragger);
		addChild(transZDragger);
		addChild(rotateZDragger);
		addChild(rotateXDragger);
		addChild(rotateYDragger);

		setHandleEvents(true);
		float scale = m_bb.radius();
		setMatrix(osg::Matrix::scale(scale, scale, scale) * osg::Matrix::translate(m_bb.center()));

		addTransformUpdating(m_mt);
		setParentDragger(this);
	}

protected:
	osg::ref_ptr<osg::MatrixTransform> m_mt;
	osg::BoundingBox m_bb;
};

void Interface::enableDrag(Object* obj)
{
	qDebug() << "enableDrag" << obj;

	static osg::ref_ptr<NodeDragger> s_nodeDragger;
	if (s_nodeDragger.valid()) {
		auto dragger = s_nodeDragger;
		auto view = m_renderInfo->m_mainView;
		m_renderInfo->addOperation(new LambdaOperation([dragger, view]() {
			while (dragger->getNumParents() > 0) {
				dragger->getParent(0)->removeChild(dragger);
			}
			}));
		s_nodeDragger = nullptr;
	}

	auto node = dynamic_cast<Node*>(obj);
	if (node) {
		osg::MatrixTransform* mt = node->getMatrixTransform();
		NodeDragger* nodeDragger = new NodeDragger(mt);

		auto view = m_renderInfo->m_mainView;
		m_renderInfo->addOperation(new LambdaOperation([nodeDragger, view]() {
			nodeDragger->setupDefaultGeometry();

			auto otherGroup = ViewInfo::getOtherGroup(view);
			otherGroup->addChild(nodeDragger);
			}));

		s_nodeDragger = nodeDragger;
		return;
	}

	auto light = dynamic_cast<Light*>(obj);
	if (light) {
		osg::MatrixTransform* mt = light->getMatrixTransform();

		osg::BoundingBox bb = light->getBoundingBox();
		osg::BoundingSphere bs;
		bs.expandBy(bb);
		osg::Vec3 transCenter = bs.center() * mt->getMatrix();
		bs.set(transCenter, bs.radius());
		bb.init();
		bb.expandBy(bs);
		NodeDragger* nodeDragger = new NodeDragger(mt, bb);

		LightNotifier* notifier = new LightNotifier;
		connect(notifier, &LightNotifier::lightDraggered, light, [light]() {
			light->updateInfoByNode();
			});
		class LightDraggerCallback : public osgManipulator::DraggerCallback
		{
		public:
			LightDraggerCallback(LightNotifier* notifier) : m_notifier(notifier) {}
			~LightDraggerCallback() {
				if (m_notifier) {
					m_notifier->deleteLater();
				}
			}
			virtual bool receive(const osgManipulator::MotionCommand& mc) {
				if (m_notifier) {
					emit m_notifier->lightDraggered();
				}
				return false;
			}
		protected:
			LightNotifier* m_notifier;
		};

		nodeDragger->addDraggerCallback(new LightDraggerCallback(notifier));

		auto view = m_renderInfo->m_mainView;
		m_renderInfo->addOperation(new LambdaOperation([nodeDragger, view]() {
			nodeDragger->setupDefaultGeometry();

			auto otherGroup = ViewInfo::getOtherGroup(view);
			otherGroup->addChild(nodeDragger);
			}));

		s_nodeDragger = nodeDragger;
		return;
	}
}

void Interface::setShaderMode(int mode)
{
	auto view = m_renderInfo->m_mainView;
	m_renderInfo->addOperation(new LambdaOperation([mode, view]() {
		view->getCamera()->getOrCreateStateSet()->getUniform("shaderMode")->set(mode);
		}));
}

void Interface::showFrustum()
{
	auto view = m_renderInfo->m_mainView;
	osg::Vec3d lightDir(1.0, 1.0, 1.0);
	osg::Vec3 pos;
	if (!m_lights.empty()) {
		auto directionalLight = dynamic_cast<DirectionalLight*>(m_lights.front().get());
		if (directionalLight) {
			auto lightMatrix = directionalLight->getMatrixTransform()->getMatrix();
			auto rot = lightMatrix.getRotate();
			lightDir = osg::Vec3d(0.0, 0.0, -1.0) * osg::Matrix::rotate(rot);
			pos = lightMatrix.getTrans();
		}
	}
	lightDir.normalize();

	m_renderInfo->addOperation(new LambdaOperation([lightDir, pos, view]() {
		auto camera = view->getCamera();

		osg::Vec3d eye, center, up;
		camera->getViewMatrixAsLookAt(eye, center, up);
		osg::Vec3d lookDir = center - eye;
		lookDir.normalize();
		osg::Vec3d left = up ^ lookDir;

		double fovy, aspect, zNear, zFar;
		if (camera->getProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar)) {
			double dis = zFar - zNear;
			osg::Vec3d nearCenter = eye + lookDir * zNear;
			osg::Vec3d farCenter = eye + lookDir * zFar;

			double nearHalfHeight = zNear * tan(osg::DegreesToRadians(fovy) * 0.5);
			double nearHalfWidth = nearHalfHeight * aspect;

			osg::Vec3d nearLeftBottom(nearCenter + left * nearHalfWidth + -up * nearHalfHeight);	// nearLeftBottom
			osg::Vec3d nearLeftUp(nearCenter + left * nearHalfWidth + up * nearHalfHeight);			// nearLeftUp
			osg::Vec3d nearRightBottom(nearCenter + -left * nearHalfWidth + -up * nearHalfHeight);	// nearRightBottom;
			osg::Vec3d nearRightUp(nearCenter + -left * nearHalfWidth + up * nearHalfHeight);		// nearRightUp

			double ratio = zFar / zNear;
			osg::Vec3d farLeftBottom = eye + (nearLeftBottom - eye) * ratio;
			osg::Vec3d farLeftUp = eye + (nearLeftUp - eye) * ratio;
			osg::Vec3d farRightBottom = eye + (nearRightBottom - eye) * ratio;
			osg::Vec3d farRightUp = eye + (nearRightUp - eye) * ratio;

			osg::Vec3Array* vArray = new osg::Vec3Array;
			vArray->push_back(nearLeftBottom);
			vArray->push_back(nearLeftUp);
			vArray->push_back(nearRightBottom);
			vArray->push_back(nearRightUp);
			vArray->push_back(farLeftBottom);
			vArray->push_back(farLeftUp);
			vArray->push_back(farRightBottom);
			vArray->push_back(farRightUp);

			osg::DrawElementsUShort* drawElements = new osg::DrawElementsUShort(GL_TRIANGLES);
			drawElements->push_back(0);
			drawElements->push_back(1);
			drawElements->push_back(2);
			drawElements->push_back(1);
			drawElements->push_back(2);
			drawElements->push_back(3);

			drawElements->push_back(4);
			drawElements->push_back(5);
			drawElements->push_back(6);
			drawElements->push_back(5);
			drawElements->push_back(6);
			drawElements->push_back(7);

			osg::Geometry* geometry = new osg::Geometry;
			geometry->setUseDisplayList(false);
			geometry->setUseVertexBufferObjects(true);
			geometry->setVertexArray(vArray);
			geometry->addPrimitiveSet(drawElements);
			geometry->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE));
			geometry->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(3.0));
			geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
			auto other = ViewInfo::getOtherGroup(view);
			other->addChild(geometry);

			// compute bounding box for frustum from lightDir direction
			osg::Vec3d lightUp = osg::X_AXIS;
			lightUp = lightUp ^ lightDir;
			lightUp.normalize();
			osg::Vec3d lightLeft = lightUp ^ lightDir;
			lightLeft.normalize();

			osg::Matrix rotMatrix(
				lightLeft.x(), lightLeft.y(), lightLeft.z(), 0.0,
				lightUp.x(), lightUp.y(), lightUp.z(), 0.0,
				lightDir.x(), lightDir.y(), lightDir.z(), 0.0,
				0, 0, 0, 1.0
			);

			osg::Vec3d tmpX = osg::X_AXIS * rotMatrix;
			osg::Vec3d tmpY = osg::Y_AXIS * rotMatrix;
			osg::Vec3d tmpZ = osg::Z_AXIS * rotMatrix;

			osg::Matrix rotMatrix_inv = osg::Matrix::inverse(rotMatrix);
			osg::BoundingBoxd wrapBB;
			for (auto& v : vArray->asVector()) {
				osg::Vec3d sv = v * rotMatrix_inv;
				wrapBB.expandBy(sv);
			}
			
			auto createGeometryForBoundingBox = [](osg::BoundingBoxd bb) {
				osg::Vec3d diff = bb._max - bb._min;
				osg::ShapeDrawable* shape = new osg::ShapeDrawable(new osg::Box(
					bb.center(), diff.x(), diff.y(), diff.z()
				));
				shape->setColor(osg::Vec4(0.0, 1.0, 0.0, 1.0));
				shape->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonMode(
					osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE
				));
				shape->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(2.0));
				shape->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
				return shape;
			};

			auto wrapShape = createGeometryForBoundingBox(wrapBB);
			auto mt = new osg::MatrixTransform;
			mt->setMatrix(rotMatrix);
			mt->addChild(wrapShape);
			other->addChild(mt);

			auto createAxis = [](const osg::Vec3& xAxis, const osg::Vec3& yAxis, const osg::Vec3& zAxis) {
				osg::Geometry* geometry = new osg::Geometry;
				geometry->setUseDisplayList(false);
				geometry->setUseVertexBufferObjects(true);

				osg::Vec3Array* vArray = new osg::Vec3Array;
				vArray->push_back(osg::Vec3());
				vArray->push_back(xAxis);
				vArray->push_back(osg::Vec3());
				vArray->push_back(yAxis);
				vArray->push_back(osg::Vec3());
				vArray->push_back(zAxis);
				osg::Vec4Array* cArray = new osg::Vec4Array;
				cArray->push_back(osg::Vec4(1.0, 0.0, 0.0, 1.0));
				cArray->push_back(osg::Vec4(1.0, 0.0, 0.0, 1.0));
				cArray->push_back(osg::Vec4(0.0, 1.0, 0.0, 1.0));
				cArray->push_back(osg::Vec4(0.0, 1.0, 0.0, 1.0));
				cArray->push_back(osg::Vec4(0.0, 0.0, 1.0, 1.0));
				cArray->push_back(osg::Vec4(0.0, 0.0, 1.0, 1.0));
				geometry->setVertexArray(vArray);
				geometry->setColorArray(cArray, osg::Array::BIND_PER_VERTEX);
				geometry->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, vArray->getNumElements()));
				geometry->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(10));
				geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
				return geometry;
			};
			auto axis = createAxis(lightLeft, lightUp, lightDir);
			auto axisMt = new osg::MatrixTransform;
			axisMt->setMatrix(osg::Matrix::scale(100, 100, 100) * osg::Matrix::translate(pos));
			axisMt->addChild(axis);
			other->addChild(axisMt);
		}
		else {
			qWarning() << "main camera's projection matrix isn't perspective";
		}

		}));
}

void Interface::useShadow(bool bUsed)
{
	auto view = m_renderInfo->m_mainView;
	m_renderInfo->addOperation(new LambdaOperation([bUsed, view]() {
		view->getCamera()->getOrCreateStateSet()->getUniform("useShadow")->set(bUsed);
		}));
}
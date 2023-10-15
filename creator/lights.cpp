#include "lights.h"
#include "deferred_rendering.h"
#include <operation.h>
#include <osg/ShapeDrawable>
#include <osg/BufferIndexBinding>
#include <osg/PolygonMode>

Light::Light(Type type) : m_type(type)
{
	m_mt = new osg::MatrixTransform;
	m_proxyGeode = new osg::Geode;
	m_mt->addChild(m_proxyGeode);
}

void Light::addToScene()
{
	auto renderInfo = getRenderInfo();
	auto view = renderInfo->m_mainView;
	auto node = m_mt;
	renderInfo->addOperation(new LambdaOperation([node, view]() {
		auto otherGroup = ViewInfo::getOtherGroup(view);
		otherGroup->addChild(node);
		view->home();
		}));
	m_bAddedToScene = true;

	updateShader();
}

void Light::setEmissionColor(const osg::Vec3& color)
{
	m_emissionColor = color;
	if (m_bAddedToScene) {
		updateShader();
	}
}

void Light::updateInfoByNode()
{
	if (m_bAddedToScene) {
		updateShader();
	}
}

osg::Vec3 DirectionalLight::s_defaultDir = osg::Vec3(0.0, 0.0, -1.0);
int DirectionalLight::s_count = 0;

//void DirectionalLight::setDir(const osg::Vec3& dir)
//{
//	m_dir = dir;
//	if (m_bAddedToScene) {
//		updateShader();
//	}
//}
class UpdateDirectionalLightCallback : public osg::Camera::DrawCallback
{
public:
	virtual void operator () (osg::RenderInfo& renderInfo) const override {
		auto rttCamera = renderInfo.getCurrentCamera();
		auto view = dynamic_cast<osgViewer::View*>(renderInfo.getView());
		auto modelGroup = ViewInfo::getModelGroup(view);
		auto mainCamera = view->getCamera();

		osg::Vec3d lightUp = osg::X_AXIS;
		lightUp = lightUp ^ m_lightDir;
		lightUp.normalize();
		osg::Matrix rotMatrix;
		auto bb = getLightSpaceBoundingBoxOfMainCameraViewFrustum(mainCamera, m_lightDir, lightUp, rotMatrix);

		osg::Vec3d bb_diff = bb._max - bb._min;
		double halfWidth = bb_diff.x() * 0.5;
		double halfHeight = bb_diff.y() * 0.5;
		double halfDepth = bb_diff.z() * 0.5;

		osg::Vec3d center = bb.center() * rotMatrix;
		osg::Vec3d eye = center - m_lightDir * halfDepth * 1.01;
		osg::Matrix viewMatrix = osg::Matrix::lookAt(eye, center, lightUp);
		osg::Matrix projMatrix = osg::Matrix::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0, halfDepth * (2 + 0.01));
		rttCamera->setViewMatrix(viewMatrix);
		rttCamera->setProjectionMatrix(projMatrix);

		osg::Matrixf VP = viewMatrix * projMatrix;
		modelGroup->getOrCreateStateSet()->getUniform("dLightShadowVP")->set(VP);
	}
	void setLightDir(const osg::Vec3d& dir) { m_lightDir = dir; }
protected:
	osg::BoundingBoxd getLightSpaceBoundingBoxOfMainCameraViewFrustum(osg::Camera* camera,
		const osg::Vec3d& lightDir, const osg::Vec3d& lightUp, osg::Matrix& rotMatrix) const
	{
		osg::BoundingBoxd bb;
		osg::Vec3d eye, center, up;
		camera->getViewMatrixAsLookAt(eye, center, up);
		osg::Vec3d lookDir = center - eye;
		lookDir.normalize();
		osg::Vec3d left = up ^ lookDir;
		left.normalize();

		double fovy, aspect, zNear, zFar;
		if (camera->getProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar)) {
			// compute frustum vertex
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

			std::vector<osg::Vec3d> verts;
			verts.push_back(nearLeftBottom);
			verts.push_back(nearLeftUp);
			verts.push_back(nearRightBottom);
			verts.push_back(nearRightUp);
			verts.push_back(farLeftBottom);
			verts.push_back(farLeftUp);
			verts.push_back(farRightBottom);
			verts.push_back(farRightUp);

			// compute bounding box for frustum from lightDir direction
			osg::Vec3d lightLeft = lightUp ^ lightDir;
			lightLeft.normalize();
			rotMatrix = osg::Matrix(lightLeft.x(), lightLeft.y(), lightLeft.z(), 0.0,
				lightUp.x(), lightUp.y(), lightUp.z(), 0.0,
				lightDir.x(), lightDir.y(), lightDir.z(), 0.0,
				0, 0, 0, 1.0
			);
			osg::Vec3d tmpX = osg::X_AXIS * rotMatrix;
			osg::Vec3d tmpY = osg::Y_AXIS * rotMatrix;
			osg::Vec3d tmpZ = osg::Z_AXIS * rotMatrix;

			osg::Matrix rotMatrix_inv = osg::Matrix::inverse(rotMatrix);
			for (const auto& v : verts) {
				osg::Vec3d sv = v * rotMatrix_inv;
				bb.expandBy(sv);
			}

		}
		else {
			qWarning() << "main camera's projection matrix isn't perspective";
		}

		return bb;
	}
protected:
	osg::Vec3d m_lightDir;
};

osg::Program* createShadowProgram()
{
	const char* vs = R"(
#version 330 core
layout(location = 0) in vec4 Position;
uniform mat4 osg_ModelViewProjectionMatrix;
void main()
{
	gl_Position = osg_ModelViewProjectionMatrix * Position;
}
)";
	const char* fs = R"(
#version 330 core
void main()
{
}
)";
	osg::Program* program = new osg::Program;
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
	return program;
}

void DirectionalLight::addToScene()
{
	osg::ShapeDrawable* plane = new osg::ShapeDrawable(new osg::Box(osg::Vec3(), 10, 10, 0.05));
	plane->setUseDisplayList(false);
	plane->setUseVertexBufferObjects(true);

	osg::ShapeDrawable* line = new osg::ShapeDrawable(new osg::Cylinder(osg::Vec3(0.0, 0.0, -5.0), 0.3, 10));
	line->setUseDisplayList(false);
	line->setUseVertexBufferObjects(true);

	// because osg::Cone has BaseOffset, so offset z value.
	osg::ShapeDrawable* cone = new osg::ShapeDrawable(new osg::Cone(osg::Vec3(0.0, 0.0, -10 + -0.25 * 5.0), 1.0, -5.0));
	cone->setUseDisplayList(false);
	cone->setUseVertexBufferObjects(true);

	m_proxyGeode->addDrawable(plane);
	m_proxyGeode->addDrawable(line);
	m_proxyGeode->addDrawable(cone);

	int texWidth = 1024, texHeight = 1024;
	osg::ref_ptr<osg::Texture2D> depthTexture = createDepthTexture(texWidth, texHeight);
	auto rttCamera = createRTTCamera(texWidth, texHeight);
	rttCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	rttCamera->setRenderOrder(osg::Camera::RenderOrder::PRE_RENDER, -1);
	rttCamera->setPreDrawCallback(new UpdateDirectionalLightCallback);
	rttCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	rttCamera->attach(osg::Camera::DEPTH_BUFFER, depthTexture);
	rttCamera->setDrawBuffer(GL_NONE);
	rttCamera->setReadBuffer(GL_NONE);
	auto shadowProgram = createShadowProgram();
	rttCamera->getOrCreateStateSet()->setAttributeAndModes(shadowProgram, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

	auto view = getRenderInfo()->m_mainView;
	getRenderInfo()->addOperation(new LambdaOperation([rttCamera, depthTexture, view]() {
		auto model = ViewInfo::getModelGroup(view);
		auto other = ViewInfo::getOtherGroup(view);
		rttCamera->addChild(model);
		other->addChild(rttCamera);

		model->getOrCreateStateSet()->setTextureAttributeAndModes(0, depthTexture, osg::StateAttribute::ON);
		osg::Uniform* shadowMapUniform = new osg::Uniform(osg::Uniform::SAMPLER_2D, "directionalLightShadowMap");
		shadowMapUniform->set(0);
		model->getOrCreateStateSet()->addUniform(shadowMapUniform);

		osg::Uniform* shadowVPUniform = new osg::Uniform("dLightShadowVP", osg::Matrixf());
		model->getOrCreateStateSet()->addUniform(shadowVPUniform);
		}));

	m_shadowRTTCamera = rttCamera;
	m_index = s_count++;
	Light::addToScene();
}

osg::BoundingBox DirectionalLight::getBoundingBox() const
{
	osg::BoundingSphere bs(osg::Vec3(), 5);
	osg::BoundingBox bb;
	bb.expandBy(bs);
	return bb;
}

void DirectionalLight::updateShader()
{
	auto renderInfo = getRenderInfo();
	auto view = renderInfo->m_mainView;
	int index = m_index;
	int count = s_count;
	DirectionalLightMaterial material;
	material.color = osg::Vec4(m_emissionColor, 1.0);
	auto matrix = getMatrixTransform()->getMatrix();
	osg::Vec3 dir = s_defaultDir * osg::Matrix::rotate(matrix.getRotate());
	dir.normalize();
	material.direction = osg::Vec4(dir, 1.0);
	auto rttCamera = m_shadowRTTCamera;
	renderInfo->addOperation(new LambdaOperation([material, index, count, rttCamera, view]() {
		osg::StateAttribute* sa = view->getCamera()->getOrCreateStateSet()->getAttribute(osg::StateAttribute::UNIFORMBUFFERBINDING);
		auto uBuffer = dynamic_cast<osg::UniformBufferBinding*>(sa);
		auto bd = dynamic_cast<osg::FloatArray*>(uBuffer->getBufferData());
		auto data = reinterpret_cast<DirectionalLightUBuffer*>(bd->asVector().data());
		data->count = osg::Vec4i(count, 0, 0, 0);
		data->mats[index] = material;
		bd->dirty();

		auto cb = dynamic_cast<UpdateDirectionalLightCallback*>(rttCamera->getPreDrawCallback());
		osg::Vec3 lightDir(
			material.direction.x(),
			material.direction.y(),
			material.direction.z()
		);
		cb->setLightDir(lightDir);
		}));
}

int PointLight::s_count = 0;

//void PointLight::setPosition(const QVector3D& pos)
//{
//	m_position = pos;
//}

osg::Program* createPointLightShadowProgram()
{
	const char* vs = R"(
#version 330 core
layout(location = 0) in vec4 Position;
uniform mat4 osg_ModelViewMatrix;
void main()
{
	gl_Position = osg_ModelViewMatrix * Position;
}
)";
	const char* gs = R"(
#version 330 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;
uniform mat4 pLightShadowVPs[6];
out vec3 worldPos;
void main()
{
	for(int face = 0; face < 6; ++face) {
		gl_Layer = face;
		for(int i = 0; i < 3; ++i) {
			worldPos = gl_in[i].gl_Position.xyz;
			gl_Position = pLightShadowVPs[face] * vec4(worldPos, 1.0);
			EmitVertex();
		}
		EndPrimitive();
	}
}
)";
	const char* fs = R"(
#version 330 core
uniform vec3 lightPosition;
uniform float pointLightRadius;
in vec3 worldPos;
void main()
{
	float lightDistance = length(worldPos - lightPosition);
	lightDistance = lightDistance / pointLightRadius;
	gl_FragDepth = lightDistance;
}
)";
	osg::Program* program = new osg::Program;
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
	program->addShader(new osg::Shader(osg::Shader::GEOMETRY, gs));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
	return program;
}

void PointLight::addToScene()
{
	osg::ShapeDrawable* sphere = new osg::ShapeDrawable(new osg::Sphere);
	sphere->setUseDisplayList(false);
	sphere->setUseVertexBufferObjects(true);

	m_proxyGeode->addDrawable(sphere);

	int texWidth = 1024, texHeight = 1024;
	osg::ref_ptr<osg::TextureCubeMap> depthTexture = createCubeMapDepthTexture(texWidth, texHeight);
	osg::ref_ptr<osg::TextureCubeMap> colorTexture = createCubeMapColorTexture(texWidth, texHeight);
	auto rttCamera = createRTTCamera(texWidth, texHeight);
	rttCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	rttCamera->setRenderOrder(osg::Camera::RenderOrder::PRE_RENDER, -1);
	rttCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	rttCamera->setCullingMode(osg::CullSettings::NO_CULLING);
	rttCamera->attach(osg::Camera::COLOR_BUFFER, colorTexture, 0, osg::Camera::FACE_CONTROLLED_BY_GEOMETRY_SHADER);
	rttCamera->attach(osg::Camera::DEPTH_BUFFER, depthTexture, 0, osg::Camera::FACE_CONTROLLED_BY_GEOMETRY_SHADER);
	rttCamera->setDrawBuffer(GL_NONE);
	rttCamera->setReadBuffer(GL_NONE);
	auto shadowProgram = createPointLightShadowProgram();
	rttCamera->getOrCreateStateSet()->setAttributeAndModes(shadowProgram, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

	auto view = getRenderInfo()->m_mainView;
	getRenderInfo()->addOperation(new LambdaOperation([rttCamera, depthTexture, view]() {
		auto model = ViewInfo::getModelGroup(view);
		auto other = ViewInfo::getOtherGroup(view);
		rttCamera->addChild(model);
		other->addChild(rttCamera);

		model->getOrCreateStateSet()->setTextureAttributeAndModes(0, depthTexture, osg::StateAttribute::ON);
		osg::Uniform* shadowMapUniform = new osg::Uniform(osg::Uniform::SAMPLER_2D, "pointLightShadowMap");
		shadowMapUniform->set(0);
		model->getOrCreateStateSet()->addUniform(shadowMapUniform);

		osg::Uniform* shadowVPArray = new osg::Uniform(osg::Uniform::Type::FLOAT_MAT4, "pLightShadowVPs", 6);
		model->getOrCreateStateSet()->addUniform(shadowVPArray);

		osg::Uniform* lightPosUniform = new osg::Uniform("lightPosition", osg::Vec3());
		model->getOrCreateStateSet()->addUniform(lightPosUniform);

		osg::Uniform* radiusUniform = new osg::Uniform("pointLightRadius", 0.0f);
		model->getOrCreateStateSet()->addUniform(radiusUniform);

		}));

	m_shadowRTTCamera = rttCamera;

	m_index = s_count++;
	Light::addToScene();
}

osg::BoundingBox PointLight::getBoundingBox() const
{
	osg::BoundingSphere bs(osg::Vec3(), 1);
	osg::BoundingBox bb;
	bb.expandBy(bs);
	return bb;
}

void PointLight::updateShader()
{
	auto renderInfo = getRenderInfo();
	auto view = renderInfo->m_mainView;
	int index = m_index;
	int count = s_count;
	PointLightMaterial material;
	material.color = osg::Vec4(m_emissionColor, 1.0);
	auto matrix = getMatrixTransform()->getMatrix();
	material.position = osg::Vec4(matrix.getTrans(), 1.0);
	material.param = m_param;
	auto rttCamera = m_shadowRTTCamera;
	renderInfo->addOperation(new LambdaOperation([material, index, count, rttCamera, view]() {
		osg::StateAttribute* sa = view->getCamera()->getOrCreateStateSet()->getAttribute(osg::StateAttribute::UNIFORMBUFFERBINDING, 1);
		auto uBuffer = dynamic_cast<osg::UniformBufferBinding*>(sa);
		auto bd = dynamic_cast<osg::FloatArray*>(uBuffer->getBufferData());
		auto data = reinterpret_cast<PointLightUBuffer*>(bd->asVector().data());
		data->count = osg::Vec4i(count, 0, 0, 0);
		data->mats[index] = material;
		bd->dirty();

		double constant = material.param.x();
		double linear = material.param.y();
		double exp = material.param.z();
		double distance = -linear + sqrt(linear * linear - 4 * exp * (constant - 256)) / (2 * exp);
		qDebug() << "point light valid distance:" << distance;


		double Fatt = 1.0 / (1 + 0.09 * 50 + 0.032 * 50 * 50);
		qDebug() << "Fatt" << Fatt;

		osg::Vec3 lightPosition(
			material.position.x(),
			material.position.y(),
			material.position.z()
		);

		osg::Matrix projMatrix = osg::Matrix::perspective(90, 1.0, 0.1, distance);
		rttCamera->setViewMatrix(osg::Matrix::identity());
		rttCamera->setProjectionMatrix(projMatrix);

		auto model = ViewInfo::getModelGroup(view);
		model->getOrCreateStateSet()->getUniform("lightPosition")->set(lightPosition);
		model->getOrCreateStateSet()->getUniform("pointLightRadius")->set(static_cast<float>(distance));
		auto vpUniform = model->getOrCreateStateSet()->getUniform("pLightShadowVPs");
		vpUniform->setElement(0,
			osg::Matrix::lookAt(lightPosition, lightPosition + osg::Vec3(1.0, 0.0, 0.0), osg::Vec3(0.0, -1.0, 0.0)) * projMatrix);
		vpUniform->setElement(1,
			osg::Matrix::lookAt(lightPosition, lightPosition + osg::Vec3(-1.0, 0.0, 0.0), osg::Vec3(0.0, -1.0, 0.0)) * projMatrix);
		vpUniform->setElement(2,
			osg::Matrix::lookAt(lightPosition, lightPosition + osg::Vec3(0.0, 1.0, 0.0), osg::Vec3(0.0, 0.0, 1.0)) * projMatrix);
		vpUniform->setElement(3,
			osg::Matrix::lookAt(lightPosition, lightPosition + osg::Vec3(0.0, -1.0, 0.0), osg::Vec3(0.0, 0.0, -1.0)) * projMatrix);
		vpUniform->setElement(4,
			osg::Matrix::lookAt(lightPosition, lightPosition + osg::Vec3(0.0, 0.0, 1.0), osg::Vec3(0.0, -1.0, 0.0)) * projMatrix);
		vpUniform->setElement(5,
			osg::Matrix::lookAt(lightPosition, lightPosition + osg::Vec3(0.0, 0.0, -1.0), osg::Vec3(0.0, -1.0, 0.0)) * projMatrix);
		}));
}

int SpotLight::s_count = 0;
osg::Vec3 SpotLight::s_defaultDir = osg::Vec3(0, 0, -1);

//void SpotLight::setPosition(const QVector3D& position)
//{
//	m_position = position;
//}
//
//void SpotLight::setDir(const QVector3D& dir)
//{
//	m_dir = dir;
//}
//
//void SpotLight::setRadius(const float radius)
//{
//	m_radius = radius;
//}

class UpdateSpotLightCallback : public osg::Camera::DrawCallback
{
public:
	virtual void operator () (osg::RenderInfo& renderInfo) const override {
		auto rttCamera = renderInfo.getCurrentCamera();
		auto view = dynamic_cast<osgViewer::View*>(renderInfo.getView());
		auto modelGroup = ViewInfo::getModelGroup(view);
		auto mainCamera = view->getCamera();

		osg::Vec3d lightUp = osg::X_AXIS;
		lightUp = lightUp ^ m_lightDir;
		lightUp.normalize();
		osg::Matrix rotMatrix;
		auto bb = getLightSpaceBoundingBoxOfMainCameraViewFrustum(mainCamera, m_lightDir, lightUp, rotMatrix);
		osg::Vec3d farestPos = bb._max * rotMatrix;
		double farLength = (farestPos - m_lightPosition) * m_lightDir;
		double farRadius = 1.0;
		if (farLength > 0) {
			farRadius = tan(osg::DegreesToRadians(m_outerCutOffAngle)) * farLength;
		}

		osg::Vec3d center = m_lightPosition + m_lightDir;
		osg::Matrix viewMatrix = osg::Matrix::lookAt(m_lightPosition, center, lightUp);
		//osg::Matrix projMatrix = osg::Matrix::ortho(-farRadius, farRadius, -farRadius, farRadius, 0, farLength);
		osg::Matrix projMatrix = osg::Matrix::perspective(m_outerCutOffAngle * 2, 1, 0.1, farLength);
		rttCamera->setViewMatrix(viewMatrix);
		rttCamera->setProjectionMatrix(projMatrix);

		osg::Matrixf VP = viewMatrix * projMatrix;
		modelGroup->getOrCreateStateSet()->getUniform("sLightShadowVP")->set(VP);
		modelGroup->getOrCreateStateSet()->getUniform("spot_near_plane")->set(0.1f);
		modelGroup->getOrCreateStateSet()->getUniform("spot_far_plane")->set(static_cast<float>(farLength));
	}
	void setLightDir(const osg::Vec3d& dir) { m_lightDir = dir; }
	void setLightPosition(const osg::Vec3d& pos) { m_lightPosition = pos; }
	void setOuterCutOffAngle(double angle) { m_outerCutOffAngle = angle; }
protected:
	osg::BoundingBoxd getLightSpaceBoundingBoxOfMainCameraViewFrustum(osg::Camera* camera,
		const osg::Vec3d& lightDir, const osg::Vec3d& lightUp, osg::Matrix& rotMatrix) const
	{
		osg::BoundingBoxd bb;
		osg::Vec3d eye, center, up;
		camera->getViewMatrixAsLookAt(eye, center, up);
		osg::Vec3d lookDir = center - eye;
		lookDir.normalize();
		osg::Vec3d left = up ^ lookDir;
		left.normalize();

		double fovy, aspect, zNear, zFar;
		if (camera->getProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar)) {
			// compute frustum vertex
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

			std::vector<osg::Vec3d> verts;
			verts.push_back(nearLeftBottom);
			verts.push_back(nearLeftUp);
			verts.push_back(nearRightBottom);
			verts.push_back(nearRightUp);
			verts.push_back(farLeftBottom);
			verts.push_back(farLeftUp);
			verts.push_back(farRightBottom);
			verts.push_back(farRightUp);

			// compute bounding box for frustum from lightDir direction
			osg::Vec3d lightLeft = lightUp ^ lightDir;
			lightLeft.normalize();
			rotMatrix = osg::Matrix(lightLeft.x(), lightLeft.y(), lightLeft.z(), 0.0,
				lightUp.x(), lightUp.y(), lightUp.z(), 0.0,
				lightDir.x(), lightDir.y(), lightDir.z(), 0.0,
				0, 0, 0, 1.0
			);
			osg::Vec3d tmpX = osg::X_AXIS * rotMatrix;
			osg::Vec3d tmpY = osg::Y_AXIS * rotMatrix;
			osg::Vec3d tmpZ = osg::Z_AXIS * rotMatrix;

			osg::Matrix rotMatrix_inv = osg::Matrix::inverse(rotMatrix);
			for (const auto& v : verts) {
				osg::Vec3d sv = v * rotMatrix_inv;
				bb.expandBy(sv);
			}

		}
		else {
			qWarning() << "main camera's projection matrix isn't perspective";
		}

		return bb;
	}
protected:
	osg::Vec3d m_lightDir;
	osg::Vec3d m_lightPosition;
	double m_outerCutOffAngle = 0.0;
};

void SpotLight::addToScene()
{
	float height = 10.0;
	float radius = tanf(osg::DegreesToRadians(m_cutOffAngle)) * height;
	osg::TessellationHints* hints = new osg::TessellationHints();
	hints->setCreateBottom(false);
	osg::ShapeDrawable* range = new osg::ShapeDrawable(new osg::Cone(osg::Vec3(0, 0, -height * 0.75), radius, height), hints);
	range->setUseDisplayList(false);
	range->setUseVertexBufferObjects(true);
	range->getOrCreateStateSet()->setAttribute(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE));

	osg::ShapeDrawable* line = new osg::ShapeDrawable(new osg::Cylinder(osg::Vec3(0.0, 0.0, -5.0), 0.3, 10));
	line->setUseDisplayList(false);
	line->setUseVertexBufferObjects(true);

	// because osg::Cone has BaseOffset, so offset z value.
	osg::ShapeDrawable* cone = new osg::ShapeDrawable(new osg::Cone(osg::Vec3(0.0, 0.0, -10 + -0.25 * 5.0), 1.0, -5.0));
	cone->setUseDisplayList(false);
	cone->setUseVertexBufferObjects(true);

	osg::ShapeDrawable* sphere = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), 0.5));
	sphere->setUseDisplayList(false);
	sphere->setUseVertexBufferObjects(true);

	m_proxyGeode->addDrawable(range);
	m_proxyGeode->addDrawable(line);
	m_proxyGeode->addDrawable(cone);
	m_proxyGeode->addDrawable(sphere);

	int texWidth = 1024, texHeight = 1024;
	osg::ref_ptr<osg::Texture2D> depthTexture = createDepthTexture(texWidth, texHeight);
	auto rttCamera = createRTTCamera(texWidth, texHeight);
	rttCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	rttCamera->setRenderOrder(osg::Camera::RenderOrder::PRE_RENDER, -1);
	rttCamera->setPreDrawCallback(new UpdateSpotLightCallback);
	rttCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	rttCamera->attach(osg::Camera::DEPTH_BUFFER, depthTexture);
	rttCamera->setDrawBuffer(GL_NONE);
	rttCamera->setReadBuffer(GL_NONE);
	auto shadowProgram = createShadowProgram();
	rttCamera->getOrCreateStateSet()->setAttributeAndModes(shadowProgram, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

	auto view = getRenderInfo()->m_mainView;
	getRenderInfo()->addOperation(new LambdaOperation([rttCamera, depthTexture, view]() {
		auto model = ViewInfo::getModelGroup(view);
		auto other = ViewInfo::getOtherGroup(view);
		rttCamera->addChild(model);
		other->addChild(rttCamera);

		model->getOrCreateStateSet()->setTextureAttributeAndModes(1, depthTexture, osg::StateAttribute::ON);
		osg::Uniform* shadowMapUniform = new osg::Uniform(osg::Uniform::SAMPLER_2D, "spotLightShadowMap");
		shadowMapUniform->set(1);
		model->getOrCreateStateSet()->addUniform(shadowMapUniform);
		model->getOrCreateStateSet()->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
		model->getOrCreateStateSet()->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON);

		osg::Uniform* shadowVPUniform = new osg::Uniform("sLightShadowVP", osg::Matrixf());
		model->getOrCreateStateSet()->addUniform(shadowVPUniform);
		model->getOrCreateStateSet()->addUniform(new osg::Uniform("spot_near_plane", 0.0f));
		model->getOrCreateStateSet()->addUniform(new osg::Uniform("spot_far_plane", 0.0f));
		}));

	m_shadowRTTCamera = rttCamera;

	m_index = s_count++;

	Light::addToScene();
}

osg::BoundingBox SpotLight::getBoundingBox() const
{
	osg::BoundingSphere bs(osg::Vec3(), 2);
	osg::BoundingBox bb;
	bb.expandBy(bs);
	return bb;
}

void SpotLight::updateShader()
{
	auto renderInfo = getRenderInfo();
	auto view = renderInfo->m_mainView;
	int index = m_index;
	int count = s_count;
	SpotLightMaterial material;
	material.color = osg::Vec4(m_emissionColor, 1.0);
	auto matrix = getMatrixTransform()->getMatrix();
	material.position = osg::Vec4(matrix.getTrans(), 1.0);
	auto dir = s_defaultDir * osg::Matrix::rotate(matrix.getRotate());
	material.direction = osg::Vec4(dir, 1.0);
	material.cutOff = osg::Vec4(cosf(osg::DegreesToRadians(m_cutOffAngle)), osg::DegreesToRadians(m_outerCutOffAngle), 0.0, 1.0);
	auto rttCamera = m_shadowRTTCamera;
	auto outerCutOffAngle = m_outerCutOffAngle;
	renderInfo->addOperation(new LambdaOperation([material, index, count, rttCamera, outerCutOffAngle, view]() {
		osg::StateAttribute* sa = view->getCamera()->getOrCreateStateSet()->getAttribute(osg::StateAttribute::UNIFORMBUFFERBINDING, 2);
		auto uBuffer = dynamic_cast<osg::UniformBufferBinding*>(sa);
		auto bd = dynamic_cast<osg::FloatArray*>(uBuffer->getBufferData());
		auto data = reinterpret_cast<SpotLightUBuffer*>(bd->asVector().data());
		data->count = osg::Vec4i(count, 0, 0, 0);
		data->mats[index] = material;
		bd->dirty();

		auto cb = dynamic_cast<UpdateSpotLightCallback*>(rttCamera->getPreDrawCallback());
		osg::Vec3 lightDir(
			material.direction.x(),
			material.direction.y(),
			material.direction.z()
		);
		osg::Vec3 lightPosition(
			material.position.x(),
			material.position.y(),
			material.position.z()
		);
		cb->setLightDir(lightDir);
		cb->setLightPosition(lightPosition);
		cb->setOuterCutOffAngle(outerCutOffAngle);
		}));
}
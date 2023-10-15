#include "deferred_rendering.h"
#include <render_info.h>

osg::Texture2D* createTexture(int width, int height)
{
	osg::Texture2D* texture = new osg::Texture2D;
	texture->setTextureSize(width, height);
	texture->setInternalFormat(GL_RGBA32F_ARB);
	texture->setFilter(osg::Texture::FilterParameter::MIN_FILTER, osg::Texture::FilterMode::LINEAR);
	texture->setFilter(osg::Texture::FilterParameter::MAG_FILTER, osg::Texture::FilterMode::NEAREST);
	return texture;
}

osg::Texture2D* createDepthTexture(int width, int height)
{
	osg::Texture2D* texture = new osg::Texture2D;
	texture->setTextureSize(width, height);
	texture->setInternalFormat(GL_DEPTH_COMPONENT32F);
	texture->setSourceFormat(GL_DEPTH_COMPONENT);
	texture->setSourceType(GL_FLOAT);
	texture->setFilter(osg::Texture::FilterParameter::MIN_FILTER, osg::Texture::FilterMode::LINEAR);
	texture->setFilter(osg::Texture::FilterParameter::MAG_FILTER, osg::Texture::FilterMode::NEAREST);
	return texture;
}

osg::TextureCubeMap* createCubeMapColorTexture(int width, int height)
{
	osg::TextureCubeMap* texture = new osg::TextureCubeMap;
	texture->setTextureSize(width, height);
	texture->setInternalFormat(GL_RGBA32F_ARB);
	texture->setSourceFormat(GL_RGBA);
	texture->setSourceType(GL_FLOAT);
	texture->setFilter(osg::Texture::FilterParameter::MIN_FILTER, osg::Texture::FilterMode::LINEAR);
	texture->setFilter(osg::Texture::FilterParameter::MAG_FILTER, osg::Texture::FilterMode::NEAREST);
	texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
	texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
	texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
	return texture;
}

osg::TextureCubeMap* createCubeMapDepthTexture(int width, int height)
{
	osg::TextureCubeMap* texture = new osg::TextureCubeMap;
	texture->setTextureSize(width, height);
	texture->setInternalFormat(GL_DEPTH_COMPONENT32F);
	texture->setSourceFormat(GL_DEPTH_COMPONENT);
	texture->setSourceType(GL_FLOAT);
	texture->setFilter(osg::Texture::FilterParameter::MIN_FILTER, osg::Texture::FilterMode::LINEAR);
	texture->setFilter(osg::Texture::FilterParameter::MAG_FILTER, osg::Texture::FilterMode::NEAREST);
	texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
	texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
	texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
	return texture;
}

osg::Camera* createRTTCamera(int width, int height)
{
	class RTTCamera : public osg::Camera
	{
	public:
		void traverse(osg::NodeVisitor& nv)
		{
			if (nv.getVisitorType() == osg::NodeVisitor::INTERSECTION_VISITOR) {
				return;
			}
			for (osg::NodeList::iterator itr = _children.begin();
				itr != _children.end();
				++itr)
			{
				(*itr)->accept(nv);
			}
		}
	};

	osg::Camera* camera = new RTTCamera;
	camera->setRenderOrder(osg::Camera::RenderOrder::PRE_RENDER);
	camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	camera->setClearDepth(1.0);
	camera->setAllowEventFocus(false);
	camera->setRenderTargetImplementation(osg::Camera::RenderTargetImplementation::FRAME_BUFFER_OBJECT);
	camera->setViewport(0, 0, width, height);

	class FBOPostDrawCallback : public osg::Camera::DrawCallback
	{
	public:
		virtual void operator () (osg::RenderInfo& renderInfo) const override {
			osg::View* view = renderInfo.getView();
			QOpenGLFramebufferObject* qtFBO = ViewInfo::getQtFBO(dynamic_cast<osgViewer::View*>(view));
			if (qtFBO) {
				qtFBO->bind();
			}
		}
	};
	camera->setPostDrawCallback(new FBOPostDrawCallback);
	return camera;
}

osg::Geometry* createRectForTexture(osg::Texture2D* baseColorTexture, osg::Texture2D* normalTexture, osg::Texture2D* depthTexture)
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

	geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, baseColorTexture, osg::StateAttribute::ON);
	osg::Uniform* baseColorTexUniform = new osg::Uniform(osg::Uniform::SAMPLER_2D, "baseColorTex");
	baseColorTexUniform->set(0);
	geometry->getOrCreateStateSet()->addUniform(baseColorTexUniform);

	geometry->getOrCreateStateSet()->setTextureAttributeAndModes(1, normalTexture, osg::StateAttribute::ON);
	osg::Uniform* normalTexUniform = new osg::Uniform(osg::Uniform::SAMPLER_2D, "normalTex");
	normalTexUniform->set(1);
	geometry->getOrCreateStateSet()->addUniform(normalTexUniform);

	geometry->getOrCreateStateSet()->setTextureAttributeAndModes(2, depthTexture, osg::StateAttribute::ON);
	osg::Uniform* depthTexUniform = new osg::Uniform(osg::Uniform::SAMPLER_2D, "depthTex");
	depthTexUniform->set(2);
	geometry->getOrCreateStateSet()->addUniform(depthTexUniform);

	geometry->setCullingActive(false);
	geometry->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
	geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vArray->getNumElements()));


	const char* vs = R"(
#version 330 core
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vUV;

out vec2 uv;

void main()
{
	gl_Position = vPosition;
	uv = vUV;
}
)";
	const char* fs = R"(
#version 330 core
uniform sampler2D baseColorTex;
uniform sampler2D normalTex;
uniform sampler2D depthTex;
uniform mat4 osg_ProjectionMatrix;
uniform vec4 viewport;

in vec2 uv;
out vec4 FragColor;
const vec3 lightDir = vec3(0.0, 0.0, 1.0);
const vec3 lightColor = vec3(1.0, 1.0, 1.0);
const float diffStrengh = 0.5;
const float ambientStrengh = 0.5;
const float specularStrength = 0.5;
void main()
{
	float depth = texture(depthTex, uv).r;
	if(depth == 1.0) discard;

	vec4 baseColor = texture(baseColorTex, uv).rgba;
	vec3 normal = texture(normalTex, uv).rgb;
	vec4 pos_ndc = vec4((gl_FragCoord.xy - viewport.xy) / viewport.zw * 2.0 - 1.0,
		depth * 2.0 - 1,
		1.0);
	vec4 pos_view_tmp = inverse(osg_ProjectionMatrix) * pos_ndc;
	vec3 pos_view = pos_view_tmp.xyz /= pos_view_tmp.w;

	// ambient color
	vec3 ambient = ambientStrengh * lightColor;
	
	// diffuse color
	float diff = max(0.0, dot(normal, lightDir));
	vec3 diffuse = diff * diffStrengh * lightColor;

	// specular color
	vec3 viewDir = normalize(-pos_view);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;

	// final color
	vec3 resultColor = baseColor.rgb * (ambient + diffuse + specular);

	FragColor = pos_view_tmp;
	FragColor = vec4(resultColor, baseColor.a);
	//FragColor = vec4(0.0, 1.0, 0.0, 1.0);
	gl_FragDepth = depth;
}
)";
	osg::Program* program = new osg::Program;
	program->setName("deferedLighting");
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
	geometry->getOrCreateStateSet()->setAttributeAndModes(program, osg::StateAttribute::ON);
	return geometry;
}

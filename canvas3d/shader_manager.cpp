#include "shader_manager.h"

ShaderMgr* ShaderMgr::s_instance = nullptr;
std::mutex ShaderMgr::s_mtx;
const std::string ShaderMgr::s_meshProgram = "mesh";
const std::string ShaderMgr::s_deferedMeshProgram = "defered_mesh";

ShaderMgr::ShaderMgr()
{

}

ShaderMgr::~ShaderMgr()
{

}


#include <osg/BufferIndexBinding>
ShaderMgr* ShaderMgr::instance()
{
	if (s_instance == nullptr) {
		std::lock_guard<std::mutex> locker(s_mtx);
		if (s_instance == nullptr) {
			s_instance = new ShaderMgr;
			s_instance->init();
		}
	}
	return s_instance;
}

osg::Program* createMeshProgram()
{
	const char* vs = R"(
#version 450 core
layout(location=0) in vec4 Position;
layout(location=1) in vec3 Normal;
uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat3 osg_NormalMatrix;

out vec3 normal;
out vec3 position;
void main() {
	gl_Position = osg_ModelViewProjectionMatrix * Position;
	normal = osg_NormalMatrix * Normal;
	position = vec4(osg_ModelViewMatrix * Position).xyz;
}
)";
	const char* fs = R"(
#version 450 core

// common data
uniform mat4 osg_ViewMatrix;
uniform mat4 osg_ViewMatrixInverse;
uniform int shaderMode; // 0: usePreviewMaterial; 1: phong; 2: pbr
uniform vec3 baseColor = vec3(0.5, 0.5, 0.5);
in vec3 normal;
in vec3 position;
out vec4 FragColor;

// for real meterial. note: light's direction points from light source to object
struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};
uniform Material material;

struct DirectionalLightMaterial {
	vec4 color;
	vec4 direction;
};
struct PointLightMaterial {
	vec4 color;
	vec4 position;
	vec4 param; // x: constant; y: linear; z: quadratic
};
struct SpotLightMaterial {
	vec4 color;
	vec4 position;
	vec4 direction;
	vec4 cutOff; // x: inner cutOff(cos(angle)); y: outer cutOff(cos(angle))
};
#define DIRECTIONAL_LIGHTS_MAX 10
#define POINT_LIGHTS_MAX 10
#define SPOT_LIGHTS_MAX 10
layout(std140, binding = 0) uniform DirectionalLightMat {
	ivec4 count;
	DirectionalLightMaterial mats[DIRECTIONAL_LIGHTS_MAX];
}DirectionalLights;
layout(std140, binding = 1) uniform PointLightMat {
	ivec4 count;
	PointLightMaterial mats[POINT_LIGHTS_MAX];
}PointLights;
layout(std140, binding = 2) uniform SpotLightMat {
	ivec4 count;
	SpotLightMaterial mats[SPOT_LIGHTS_MAX];
}SpotLights;
uniform bool useShadow;
uniform sampler2D directionalLightShadowMap;
uniform mat4 dLightShadowVP;
uniform sampler2D spotLightShadowMap;
uniform mat4 sLightShadowVP;
uniform float spot_near_plane;
uniform float spot_far_plane;
uniform samplerCube pointLightShadowMap;
uniform float pointLightRadius;

float DirectionalLightShadowCalculation(vec3 viewPos, float bias)
{
	vec4 worldPos = osg_ViewMatrixInverse * vec4(viewPos, 1.0);
	vec4 lightNDCPos = dLightShadowVP * worldPos;
	vec3 ndc = lightNDCPos.xyz / lightNDCPos.w;
	vec3 screen = (ndc.xyz + 1) * 0.5;

	//float lightDepth = texture(directionalLightShadowMap, screen.xy).r;
	//float shadow = (screen.z - bias) > lightDepth ? 1.0 : 0.0;
	//return shadow;

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(directionalLightShadowMap, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(directionalLightShadowMap, screen.xy + vec2(x, y) * texelSize).r; 
			shadow += (screen.z - bias) > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;
	return shadow;
}

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * spot_near_plane * spot_far_plane) / (spot_far_plane + spot_near_plane - z * (spot_far_plane - spot_near_plane));
}

float SpotLightShadowCalculation(vec3 viewPos, float bias)
{
	vec4 worldPos = osg_ViewMatrixInverse * vec4(viewPos, 1.0);
	vec4 lightNDCPos = sLightShadowVP * worldPos;
	vec3 ndc = lightNDCPos.xyz / lightNDCPos.w;
	if(ndc.x < -1 || ndc.x > 1 || ndc.y < -1 || ndc.y > 1) {
		return 0.0;
	}
	vec3 screen = (ndc.xyz + 1) * 0.5;
	float linear_screen_z = LinearizeDepth(screen.z);

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(spotLightShadowMap, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(spotLightShadowMap, screen.xy + vec2(x, y) * texelSize).r; 
			float linear_pcfDepth = LinearizeDepth(pcfDepth);
			shadow += (linear_screen_z - bias) > linear_pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;
	return shadow;
}

const vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float PointLightShadowCalculation(vec3 viewPos, vec3 lightWorldPos, float bias)
{
	vec3 worldPos = vec4(osg_ViewMatrixInverse * vec4(viewPos, 1.0)).xyz;
	vec3 posToLight = worldPos - lightWorldPos;
	float currentDepth = length(posToLight);

	float shadow = 0.0;
	int samples = 20;
	float diskRadius = 0.05;
	for (int i = 0; i < samples; ++i) {
		float closestDepth = texture(pointLightShadowMap, posToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= pointLightRadius;
		if ((currentDepth - bias) > closestDepth) {
			shadow += 1.0;
		}
	}
	shadow /= float(samples);
	return shadow;
}

vec3 computePhongLightAndShadow()
{
	vec3 resultColor = vec3(0, 0, 0);
	for(int i = 0; i < DirectionalLights.count.x; ++i) {
		DirectionalLightMaterial mat = DirectionalLights.mats[i];
		vec3 lightDir = normalize(mat3(osg_ViewMatrix) * mat.direction.xyz);
		vec3 lightColor = mat.color.rgb;

		vec3 ambientColor = material.ambient * lightColor;

		float diff = max(dot(normal, -lightDir), 0);
		vec3 diffuseColor = material.diffuse * diff * lightColor;

		vec3 viewDir = normalize(-position);
		vec3 reflectDir = reflect(lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
		vec3 specularColor = material.specular * spec * lightColor;

		float bias = max(0.05 * (1 - dot(normal, -lightDir)), 0.005);
		float shadow = DirectionalLightShadowCalculation(position, bias);
		if(!useShadow) {
			shadow = 0.0;
		}
		resultColor += baseColor * (ambientColor + (1 - shadow) * (diffuseColor + specularColor));
	}

	for(int i = 0; i < PointLights.count.x; ++i) {
		PointLightMaterial mat = PointLights.mats[i];
		vec3 lightPos = vec4(osg_ViewMatrix * mat.position).xyz;
		vec3 tmpDir = position - lightPos;
		float distance = length(tmpDir);

		vec3 lightDir = normalize(tmpDir);
		vec3 lightColor = mat.color.rgb;

		vec3 ambientColor = material.ambient * lightColor;

		float diff = max(dot(normal, -lightDir), 0);
		vec3 diffuseColor = material.diffuse * diff * lightColor;

		vec3 viewDir = normalize(-position);
		vec3 reflectDir = reflect(lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
		vec3 specularColor = material.specular * spec * lightColor;

		float bias = 0.1;
		float shadow = PointLightShadowCalculation(position, mat.position.xyz, bias);
		if(!useShadow) {
			shadow = 0.0;
		}

		float attenuation = 1.0 / (mat.param.x + mat.param.y * distance + mat.param.z * (distance * distance));
		resultColor += baseColor * (ambientColor + (1 - shadow) * (diffuseColor + specularColor)) * attenuation;
	}
	for(int i = 0; i < SpotLights.count.x; ++i) {
		SpotLightMaterial mat = SpotLights.mats[i];
		vec3 lightPos = vec4(osg_ViewMatrix * mat.position).xyz;
		vec3 spotDir = normalize(mat3(osg_ViewMatrix) * mat.direction.xyz);
		vec3 tmpDir = position - lightPos;
		vec3 lightDir = normalize(tmpDir);
		float theta = dot(lightDir, spotDir);
		float epsilon = mat.cutOff.x - mat.cutOff.y;
		float intensity = clamp((theta - mat.cutOff.y) / epsilon, 0.0, 1.0);

		vec3 lightColor = mat.color.rgb;

		vec3 ambientColor = material.ambient * lightColor;

		float diff = max(dot(normal, -lightDir), 0);
		vec3 diffuseColor = material.diffuse * diff * lightColor;

		vec3 viewDir = normalize(-position);
		vec3 reflectDir = reflect(lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
		vec3 specularColor = material.specular * spec * lightColor;

		float bias = max(1 * (1 - dot(normal, -lightDir)), 0.5);
		float shadow = SpotLightShadowCalculation(position, bias);
		if(!useShadow) {
			shadow = 0.0;
		}
		resultColor += baseColor * (ambientColor + (1 - shadow) * (diffuseColor + specularColor)) * intensity;
	}
	return resultColor;
}

// for preview material. note: preview_lightDir points from vertex to lightSource
const vec3 preview_lightDir = vec3(0.0, 0.0, 1.0);
const vec3 preview_lightColor = vec3(1.0, 1.0, 1.0);
const float diffStrengh = 0.5;
const float ambientStrengh = 0.5;
const float specularStrength = 0.5;

// for pbr
uniform float metallic;
uniform float roughness;
uniform float ao;
const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------

vec3 computePBRLightAndShadow()
{
	vec3 resultColor = vec3(0, 0, 0);

	vec3 N = normalize(normal);
	vec3 V = normalize(-position);
	
	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, baseColor, metallic);

	// reflectance equation
	vec3 Lo = vec3(0.0);
	for(int i = 0; i < PointLights.count.x; ++i) {
		PointLightMaterial mat = PointLights.mats[i];
		vec3 lightPos = vec4(osg_ViewMatrix * mat.position).xyz;
		vec3 tmpDir = position - lightPos;
		vec3 lightColor = mat.color.rgb * 300;

		// calculate per-light radiance
		vec3 L = normalize(-tmpDir);
		vec3 H = normalize(V + L);
		float distance = length(tmpDir);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lightColor * attenuation;

		// Cook-Torrance BRDF
		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);
		vec3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = numerator / denominator;


		// kS is equal to Fresnel
		vec3 kS = F;
		// for energy conservation, the diffuse and specular light can't
		// be above 1.0 (unless the surface emits light); to preserve this
		// relationship the diffuse component (kD) should equal 1.0 - kS.
		vec3 kD = vec3(1.0) - kS;
		// multiply kD by the inverse metalness such that only non-metals 
		// have diffuse lighting, or a linear blend if partly metal (pure metals
		// have no diffuse light).
		kD *= 1.0 - metallic;	

		// scale light by NdotL
		float NdotL = max(dot(N, L), 0.0);

		// add to outgoing radiance Lo
		Lo += (kD * baseColor / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	}

	vec3 ambient = vec3(0.03) * baseColor * ao;
	vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

	//float bias = 0.1;
	//float shadow = PointLightShadowCalculation(position, mat.position.xyz, bias);

	return color;
}

void main() {
	vec3 resultColor = vec3(0.0, 0.0, 0.0);
	if (shaderMode == 0) {
		vec3 viewDir = normalize(-position);
		vec3 reflectDir = reflect(-preview_lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
		resultColor = baseColor * (ambientStrengh + diffStrengh * max(dot(normal, preview_lightDir), 0) + specularStrength * spec);
	}
	else if (shaderMode == 1) {
		resultColor = computePhongLightAndShadow();
	}
	else if(shaderMode == 2) {
		resultColor = computePBRLightAndShadow();
	}

	FragColor = vec4(resultColor, 1.0);
}
)";

	osg::Program* program = new osg::Program;
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
	return program;
}

osg::Program* createMeshDeferedProgram()
{
	const char* vs = R"(
#version 450 core
layout(location=0) in vec4 Position;
layout(location=1) in vec3 Normal;
uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat4 osg_ModelViewMatrix;
uniform mat3 osg_NormalMatrix;

out vec3 normal;
out vec4 pos;
void main() {
	pos = osg_ModelViewMatrix * Position;
	gl_Position = osg_ModelViewProjectionMatrix * Position;;

	normal = osg_NormalMatrix * Normal;
}
)";
	const char* fs = R"(
#version 450 core
uniform vec3 baseColor = vec3(0.5, 0.5, 0.5);

uniform mat4 osg_ProjectionMatrix;
uniform vec4 viewport;

in vec3 normal;
in vec4 pos;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 FragNormal;
layout(location = 2) out vec4 FragPos;
layout(location = 3) out vec4 testPos;
void main() {
	//vec3 lightColor = baseColor * (0.5 + 0.5 * dot(normal, vec3(0.0, 0.0, 1.0)));
	FragColor = vec4(baseColor, 1.0);
	FragNormal = vec4(normal, 1.0);
	FragPos = pos;

	float depth = gl_FragCoord.z;
	vec4 ndcPos = vec4((gl_FragCoord.xy - viewport.xy) / viewport.zw * 2.0 - 1.0,
		depth * 2.0 - 1,
		1.0);
	vec4 viewPos = inverse(osg_ProjectionMatrix) * ndcPos;
	viewPos.xyz /= viewPos.w;
testPos = viewPos;
}
)";

	osg::Program* program = new osg::Program;
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
	return program;
}

void ShaderMgr::init()
{
	std::lock_guard<std::mutex> locker(m_mutex);
	// init shader
	auto program = createMeshProgram();
	program->setName(s_meshProgram);
	m_vecPrograms.push_back(program);

	auto deferedProgram = createMeshDeferedProgram();
	deferedProgram->setName(s_deferedMeshProgram);
	m_vecPrograms.push_back(deferedProgram);
}

osg::Program* ShaderMgr::getShader(const std::string& name)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	for (auto& program : m_vecPrograms) {
		if (program->getName() == name) {
			return program.get();
		}
	}
	return nullptr;
}

bool ShaderMgr::addShader(osg::Program* p)
{
	if (p == nullptr) {
		return false;
	}
	std::lock_guard<std::mutex> locker(m_mutex);
	for (auto& program : m_vecPrograms) {
		if (program->getName() == p->getName()) {
			printf("shader has existed\n");
			return false;
		}
	}
	m_vecPrograms.push_back(p);
	return true;
}

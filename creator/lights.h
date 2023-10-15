#ifndef LIGHTS_H
#define LIGHTS_H

#include "object.h"
#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Camera>

class Light : public Object
{
public:
	enum Type {
		Directional,
		Point,
		Spot
	};

	Light(Type type);

	virtual void addToScene() override;

	osg::ref_ptr<osg::MatrixTransform> getMatrixTransform() { return m_mt; }

	virtual osg::BoundingBox getBoundingBox() const = 0;

	void setEmissionColor(const osg::Vec3& color);

	virtual void updateInfoByNode();

protected:
	virtual void updateShader() = 0;

protected:
	Type m_type;

	osg::ref_ptr<osg::Geode> m_proxyGeode;
	osg::ref_ptr<osg::MatrixTransform> m_mt;
	osg::Vec3 m_emissionColor;
};

class LightNotifier : public QObject
{
	Q_OBJECT
public:
	LightNotifier() {}
	~LightNotifier() {}
signals:
	void lightDraggered();
};

class DirectionalLight : public Light
{
public:
	struct RenderParam {
		osg::Vec3 direction;
		osg::Vec3 ambient;
		osg::Vec3 diffuse;
		osg::Vec3 specular;
	};
	DirectionalLight() : Light(Type::Directional),/* m_dir(s_defaultDir),*/ m_index(-1) {}

	//void setDir(const osg::Vec3& dir);
	//const osg::Vec3& getDir() const { return m_dir; }

	virtual void addToScene() override;
	virtual osg::BoundingBox getBoundingBox() const override;

protected:
	virtual void updateShader() override;

protected:
	//osg::Vec3 m_dir;
	static osg::Vec3 s_defaultDir;
	osg::ref_ptr<osg::Camera> m_shadowRTTCamera;
private:
	static int s_count;
	int m_index;
};

class PointLight : public Light
{
public:
	PointLight() : Light(Type::Point), m_index(-1), m_param(1.0, 0.007, 0.0002, 0.0) {}

	//void setPosition(const QVector3D& pos);
	//const QVector3D& getPosition() const { return m_position; }

	virtual void addToScene() override;
	virtual osg::BoundingBox getBoundingBox() const override;

protected:
	virtual void updateShader() override;

protected:
	//QVector3D m_position;
	osg::Vec4 m_param;	//x: constant; y: linear; z: quadratic
	osg::ref_ptr<osg::Camera> m_shadowRTTCamera;

private:
	static int s_count;
	int m_index;
};

class SpotLight : public Light
{
public:
	SpotLight() : Light(Type::Spot), /*m_dir(s_defaultDir), */ m_cutOffAngle(30.0f), m_outerCutOffAngle(35.0f), m_index(-1) {}

	//void setPosition(const QVector3D& position);
	//const QVector3D& getPosition() const { return m_position; }
	//
	//void setDir(const QVector3D& dir);
	//const QVector3D& getDir() const { return m_dir; }
	//
	//void setRadius(const float radius);
	//float getRadius() const { return m_radius; }

	virtual void addToScene() override;
	virtual osg::BoundingBox getBoundingBox() const override;

protected:
	virtual void updateShader() override;

protected:
	//osg::Vec3 m_position;
	//osg::Vec3 m_dir;
	float m_cutOffAngle;
	float m_outerCutOffAngle;
	static osg::Vec3 s_defaultDir;
	osg::ref_ptr<osg::Camera> m_shadowRTTCamera;
private:
	int m_index;
	static int s_count;
};


#endif
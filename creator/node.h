#ifndef MY_RENDER_NODE_H
#define MY_RENDER_NODE_H

#include "object.h"
#include <osg/MatrixTransform>
#include <osg/Switch>
#include <osg/Geometry>
#include <osg/Geode>

namespace Physical {
	class Object;
}

typedef std::shared_ptr<osg::Vec3d> Force;


class Node : public Object
{
	Q_OBJECT
	Q_PROPERTY(bool gravityEnabled READ isGravityEnabled WRITE enableGravity NOTIFY gravityEnabledChanged)
	Q_PROPERTY(double gravity READ getGravity WRITE setGravity NOTIFY gravityChanged)
	Q_PROPERTY(double quality READ getQuality WRITE setQuality NOTIFY qualityChanged)
	Q_PROPERTY(double isShowLine READ isShowLine WRITE showLine NOTIFY showLineChanged)
	Q_PROPERTY(float metallic READ metallic WRITE setMetallic NOTIFY metallicChanged)
	Q_PROPERTY(float roughness READ roughness WRITE setRoughness NOTIFY roughnessChanged)
public:
	Node();
	~Node();

	void addGeometry(osg::Geometry* geometry);
	osg::ref_ptr<osg::Geometry> getGeometry();

	osg::ref_ptr<osg::MatrixTransform> getMatrixTransform();

	virtual void addToScene() override;

	bool isGravityEnabled() const { return m_bGravityEnabled; }
	void enableGravity(bool enable);
	void setGravity(double gravity);
	double getGravity() const { return m_gravity; }

	void setQuality(double quality);
	double getQuality() const { return m_quality; }

	void showLine(bool line);
	bool isShowLine() const { return m_bShowLine; }

	void setPhysicalObject(std::shared_ptr<Physical::Object> pObj);
	std::shared_ptr<Physical::Object>& getPhysicalObject();

	struct Material
	{
		osg::Vec3 m_ambient;
		osg::Vec3 m_diffuse;
		osg::Vec3 m_specular;
		float m_shininess = 0.0f;
	};
	void setMaterial(Material mat);

	struct PBRMaterial
	{
		float mentallic = 0.0f;
		float roughness = 0.0f;
		float ao = 1.0f;
	};
	void setPBRMaterial(PBRMaterial mat);
	float metallic() const { return m_pbrMaterial.mentallic; }
	void setMetallic(float metallic);
	float roughness() const { return m_pbrMaterial.roughness; }
	void setRoughness(float roughness);

signals:
	void gravityEnabledChanged();
	void gravityChanged();
	void qualityChanged();
	void showLineChanged();
	void metallicChanged();
	void roughnessChanged();

protected:
	osg::ref_ptr<osg::Switch> m_switch;
	osg::ref_ptr<osg::MatrixTransform> m_mt;
	osg::ref_ptr<osg::Geode> m_geode;
	osg::ref_ptr<osg::Geometry> m_geometry;

	bool m_bGravityEnabled;
	bool m_bShowLine;
	double m_gravity;
	Force m_gravityForce;

	double m_quality;
	std::shared_ptr<Physical::Object> m_physicalObj;

	Material m_material;
	PBRMaterial m_pbrMaterial;
};
//QML_DECLARE_TYPE(Node);


#endif
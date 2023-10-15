#include "node.h"
#include <operation.h>
#include <osg/PolygonMode>

class ForceCallback : public osg::NodeCallback
{
public:
	ForceCallback(double quality) : m_quality(quality) {

	}

	struct ForceCache
	{
		osg::Vec3d m_externalForcesDirection;
		osg::Vec3d m_acceleration;
	};

	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) override {
		const osg::FrameStamp* fs = nv->getFrameStamp();

		static osg::ref_ptr<osg::FrameStamp> s_fs;

		if (!m_fs.valid()) {
			m_fs = new osg::FrameStamp(*fs);
			s_fs = new osg::FrameStamp(*fs);
			return;
		}

		const double refTime = fs->getReferenceTime();
		const double oldRefTime = m_fs->getReferenceTime();
		const double offsetTime = refTime - oldRefTime;
		//qDebug() << "offsetTime:" << offsetTime;
		m_fs = new osg::FrameStamp(*fs);
		//qDebug() << "globalOffsetTime:" << refTime - s_fs->getReferenceTime();

		const auto& forceCache = getForceCache();
		osg::Vec3d offset = m_velocity * offsetTime + forceCache.m_acceleration * offsetTime * offsetTime * 0.5;
		//qDebug() << "offset" << offset;
		osg::MatrixTransform* mt = node->asTransform()->asMatrixTransform();
		osg::Matrix curMatrix = mt->getMatrix();
		osg::Vec3 curTrans = curMatrix.getTrans();
		curTrans += offset;
		//qDebug() << "curTrans" << curTrans;
		curMatrix.setTrans(curTrans);
		mt->setMatrix(curMatrix);
		//qDebug() << "acceleration:" << forceCache.m_acceleration;
		m_velocity += forceCache.m_acceleration * offsetTime;
		//qDebug() << "velocity" << m_velocity;
	}

	void addForce(const Force& force) {
		m_vecForces.push_back(force);
		m_bForcesDirty = true;
	}

	void updateForce(const Force& force) {
		for (auto itr = m_vecForces.begin(); itr != m_vecForces.end(); ++itr) {
			if (itr->get() == force.get()) {
				m_bForcesDirty = true;
				return;
			}
		}
	}

	void removeForce(const Force& force) {
		for (auto itr = m_vecForces.begin(); itr != m_vecForces.end(); ++itr) {
			if (itr->get() == force.get()) {
				m_vecForces.erase(itr);
				m_bForcesDirty = true;
				return;
			}
		}
	}

	const ForceCache& getForceCache() const {
		if (m_bForcesDirty) {
			osg::Vec3d allForces;
			for (auto& force : m_vecForces) {
				allForces += *force;
			}

			m_forceCache.m_externalForcesDirection = allForces;
			m_forceCache.m_acceleration = allForces / m_quality;

			m_bForcesDirty = false;
		}
		return m_forceCache;
	}

	void setQuality(double quality) {
		m_quality = quality;
		const auto& forceCache = getForceCache();
		ForceCache& fc = const_cast<ForceCache&>(forceCache);
		fc.m_acceleration = fc.m_externalForcesDirection / m_quality;
		m_bForcesDirty = true;
	}

protected:
	osg::Vec3d m_velocity;
	mutable ForceCache m_forceCache;
	std::vector<Force> m_vecForces;
	mutable bool m_bForcesDirty = false;
	osg::ref_ptr<osg::FrameStamp> m_fs;
	double m_quality;
};




Node::Node() :
	m_quality(1.0),
	m_bGravityEnabled(false),
	m_gravity(9.8),
	m_bShowLine(false)
{
	m_switch = new osg::Switch;
	m_mt = new osg::MatrixTransform;
	m_mt->setUpdateCallback(new ForceCallback(m_quality));
	m_geode = new osg::Geode;

	m_geode->getOrCreateStateSet()->addUniform(new osg::Uniform("material.ambient", m_material.m_ambient));
	m_geode->getOrCreateStateSet()->addUniform(new osg::Uniform("material.diffuse", m_material.m_diffuse));
	m_geode->getOrCreateStateSet()->addUniform(new osg::Uniform("material.specular", m_material.m_specular));
	m_geode->getOrCreateStateSet()->addUniform(new osg::Uniform("material.shininess", m_material.m_shininess));

	m_geode->getOrCreateStateSet()->addUniform(new osg::Uniform("metallic", m_pbrMaterial.mentallic));
	m_geode->getOrCreateStateSet()->addUniform(new osg::Uniform("roughness", m_pbrMaterial.roughness));
	m_geode->getOrCreateStateSet()->addUniform(new osg::Uniform("ao", m_pbrMaterial.ao));

	m_mt->addChild(m_geode);
	m_switch->addChild(m_mt);

	m_gravityForce.reset(new osg::Vec3d);
}

Node::~Node()
{

}

void Node::addGeometry(osg::Geometry* geometry)
{
	if (geometry) {
		auto geode = m_geode;
		m_geometry = geometry;
		osg::ref_ptr<osg::Geometry> geom = geometry;
		getRenderInfo()->addOperation(new LambdaOperation([geode, geom]() {
			geode->addDrawable(geom);
			}));
	}
}

osg::ref_ptr<osg::MatrixTransform> Node::getMatrixTransform()
{
	return m_mt;
}

osg::ref_ptr<osg::Geometry> Node::getGeometry()
{
	return m_geometry;
}

void Node::addToScene()
{
	auto renderInfo = getRenderInfo();
	auto view = renderInfo->m_mainView;
	auto sw = m_switch;
	renderInfo->addOperation(new LambdaOperation([sw, view]() {
		auto modelGroup = ViewInfo::getModelGroup(view);
		modelGroup->addChild(sw);
		view->home();
		}));
	m_bAddedToScene = true;
}

void Node::enableGravity(bool enable)
{
	if (m_bGravityEnabled == enable) {
		return;
	}
	m_bGravityEnabled = enable;
	auto mt = m_mt;

	if (enable) {
		auto gravityForce = m_gravityForce;
		auto gravity = m_gravity;
		auto quality = m_quality;
		getRenderInfo()->addOperation(new LambdaOperation([mt, gravityForce, gravity, quality]() {
			auto forceCallback = dynamic_cast<ForceCallback*>(mt->getUpdateCallback());
			*gravityForce = osg::Vec3d(0.0, 0.0, -quality) * gravity;
			forceCallback->addForce(gravityForce);
			}));
	}
	else {
		auto gravityForce = m_gravityForce;
		getRenderInfo()->addOperation(new LambdaOperation([mt, gravityForce]() {
			auto forceCallback = dynamic_cast<ForceCallback*>(mt->getUpdateCallback());
			forceCallback->removeForce(gravityForce);
			}));
	}

	emit gravityEnabledChanged();
}

void Node::setGravity(double gravity)
{
	if (m_gravity == gravity) {
		return;
	}
	auto mt = m_mt;
	if (m_bGravityEnabled) {
		auto gravityForce = m_gravityForce;
		auto quality = m_quality;
		getRenderInfo()->addOperation(new LambdaOperation([mt, gravityForce, gravity, quality]() {
			auto forceCallback = dynamic_cast<ForceCallback*>(mt->getUpdateCallback());
			*gravityForce = osg::Vec3d(0.0, 0.0, -quality) * gravity;
			forceCallback->updateForce(gravityForce);
			}));
	}
	m_gravity = gravity;
	emit gravityChanged();
}

void Node::setQuality(double quality)
{
	if (m_quality == quality) {
		return;
	}
	auto mt = m_mt;
	auto gravityForce = m_gravityForce;
	auto gravity = m_gravity;
	m_quality = quality;
	getRenderInfo()->addOperation(new LambdaOperation([mt, gravityForce, gravity, quality]() {
		auto forceCallback = dynamic_cast<ForceCallback*>(mt->getUpdateCallback());
		*gravityForce = osg::Vec3d(0.0, 0.0, -quality) * gravity;
		forceCallback->updateForce(gravityForce);
		forceCallback->setQuality(quality);
		}));
	emit qualityChanged();
}

void Node::showLine(bool line)
{
	if (m_bShowLine == line) {
		return;
	}
	m_bShowLine = line;
	auto mt = m_mt;
	getRenderInfo()->addOperation(new LambdaOperation([mt, line]() {
		auto ss = mt->getOrCreateStateSet();
		if (line) {
			ss->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::Face::FRONT, osg::PolygonMode::Mode::LINE));
		}
		else {
			ss->removeAttribute(osg::StateAttribute::POLYGONMODE);
		}

		}));

	emit showLineChanged();
}

void Node::setPhysicalObject(std::shared_ptr<Physical::Object> pObj)
{
	m_physicalObj = pObj;
}

std::shared_ptr<Physical::Object>& Node::getPhysicalObject()
{
	return m_physicalObj;
}

void Node::setMaterial(Material mat)
{
	m_material = mat;
	auto geode = m_geode;
	getRenderInfo()->addOperation(new LambdaOperation([mat, geode]() {
		geode->getOrCreateStateSet()->getUniform("material.ambient")->set(mat.m_ambient);
		geode->getOrCreateStateSet()->getUniform("material.diffuse")->set(mat.m_diffuse);
		geode->getOrCreateStateSet()->getUniform("material.specular")->set(mat.m_specular);
		geode->getOrCreateStateSet()->getUniform("material.shininess")->set(mat.m_shininess);
		}));
}

void Node::setPBRMaterial(PBRMaterial mat)
{
	m_pbrMaterial = mat;
	auto geode = m_geode;
	getRenderInfo()->addOperation(new LambdaOperation([mat, geode]() {
		geode->getOrCreateStateSet()->addUniform(new osg::Uniform("metallic", mat.mentallic));
		geode->getOrCreateStateSet()->addUniform(new osg::Uniform("roughness", mat.roughness));
		geode->getOrCreateStateSet()->addUniform(new osg::Uniform("ao", mat.ao));
		}));
	emit metallicChanged();
	emit roughnessChanged();
}

void Node::setMetallic(float metallic)
{
	m_pbrMaterial.mentallic = metallic;
	setPBRMaterial(m_pbrMaterial);
	emit metallicChanged();
}

void Node::setRoughness(float roughness)
{
	m_pbrMaterial.roughness = roughness;
	setPBRMaterial(m_pbrMaterial);
	emit roughnessChanged();
}
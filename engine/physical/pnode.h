#pragma once

#include <vector>
#include <memory>
#include <osg/Matrix>
#include <thread>
#include "physical_export.h"

namespace Physical {

	typedef float Physical_Real;
	typedef int Physical_Int;
	typedef osg::Vec3d Physical_Vec3d;
	typedef osg::Vec3f Physical_Vec3;
	typedef osg::Matrixd Physical_Matrix;
	typedef osg::Quat Physical_Quat;

	const Physical_Vec3 Physical_Vec3_Min = osg::Vec3d(-DBL_MAX, -DBL_MAX, -DBL_MAX);
	const Physical_Vec3 Physical_Vec3_Max = osg::Vec3d(DBL_MAX, DBL_MAX, DBL_MAX);

	class PHYSICAL_EXPORT Shape
	{
	public:
		enum Type {
			BoxShape,
			SphereShape,
			PointsShape,
			MeshShape,
			PlaneShape
		};

		Shape() {}
		Shape(const Shape& other) :
			m_type(other.m_type)
		{}
		virtual ~Shape() {}

		Shape& operator=(const Shape& other) {
			m_type = other.m_type;
			return *this;
		}

		Type m_type;
	};

	class PHYSICAL_EXPORT Sphere : public Shape
	{
	public:
		Sphere() {
			m_type = Type::SphereShape;
		}
		Physical_Vec3 m_center;
		Physical_Real m_radius;
	};

	class PHYSICAL_EXPORT Box : public Shape
	{
	public:
		Box() : m_min(Physical_Vec3_Max), m_max(Physical_Vec3_Min) {
			m_type = Type::BoxShape;
		}
		Box(const Physical_Vec3& min, const Physical_Vec3& max) :
			m_min(min), m_max(max) {
			m_type = Type::BoxShape;
		}

		Sphere getSphere() const {
			Sphere sphere;
			sphere.m_center = (m_min + m_max) / 2.0;
			sphere.m_radius = ((m_max - m_min) / 2.0).length();
			return sphere;
		}

		Box& operator=(const Box& other) {
			m_min = other.m_min;
			m_max = other.m_max;
			return *this;
		}

		Physical_Vec3 m_min;
		Physical_Vec3 m_max;
	};

	class PHYSICAL_EXPORT Points : public Shape
	{
	public:
		Points() {
			m_type = Type::PointsShape;
		}
		Box m_box;
		Physical_Vec3* m_pVec3 = nullptr;
		Physical_Int m_numPoints = 0;
	};

	class PHYSICAL_EXPORT Mesh : public Points
	{
	public:
		Mesh() {
			m_type = Type::MeshShape;
		}
		Physical_Int* m_pTopo = nullptr;
		Physical_Int m_numTopo = 0;
	};

	class PHYSICAL_EXPORT Plane : public Shape
	{
	public:
		Plane() {
			m_type = Type::PlaneShape;
		}
		Physical_Vec3 m_normal;
		Physical_Vec3 m_point;
	};

	class PHYSICAL_EXPORT PhysicalNode
	{
	public:
		enum PhysicalForm
		{
			Solid,
			Liquid,
			Gas
		};
		PhysicalNode() {}
		virtual ~PhysicalNode() {}
	};

	enum CollideDetectLevel
	{
		Bound,
		Primitive
	};

	class PHYSICAL_EXPORT Object : public PhysicalNode
	{
	public:
		Object() :
			m_quality(1.0),
			m_collideDetectLevel(CollideDetectLevel::Bound)
		{

		}
		Physical_Real m_quality;
		Physical_Matrix m_matrix;
		CollideDetectLevel m_collideDetectLevel;

		std::shared_ptr<Shape> m_shape;
	};

	class PHYSICAL_EXPORT Field : public PhysicalNode
	{
	public:

	};

	struct PHYSICAL_EXPORT CollideInfo
	{
		std::shared_ptr<Object> m_collideObject;
		Physical_Vec3 m_collideDir;
		Physical_Real m_collideForce;
	};

	class PHYSICAL_EXPORT CollideFunctor
	{
	public:
		bool detect(const Box& box, const Box& otherbox)
		{
			if (box.m_max.x() < otherbox.m_min.x() || box.m_min.x() > otherbox.m_max.x()) {
				return false;
			}
			if (box.m_max.y() < otherbox.m_min.y() || box.m_min.y() > otherbox.m_max.y()) {
				return false;
			}
			if (box.m_max.z() < otherbox.m_min.z() || box.m_min.z() > otherbox.m_max.z()) {
				return false;
			}
			return true;
		}
		bool detect(const Box& box, const Sphere& sphere)
		{
			Sphere sphereOfBox = box.getSphere();
			if (detect(sphereOfBox, sphere)) {
				return true;
			}
			else {
				//todo
				return false;
			}
		}
		bool detect(const Sphere& sphere, const Sphere& othersphere)
		{
			Physical_Real distance2 = (sphere.m_center - othersphere.m_center).length2();
			if (distance2 > (sphere.m_radius + othersphere.m_radius)) {
				return false;
			}
			else {
				return true;
			}
		}
		bool detect(const Mesh& mesh, const Mesh& otherMesh)
		{
			const auto& meshBox = mesh.m_box;
			const auto& otherMeshBox = otherMesh.m_box;
			return detect(meshBox, otherMeshBox);
		}

		bool detect(const Mesh& mesh, const Box& box)
		{
			if (mesh.m_pTopo) {
				//todo 
				return false;
			}
			else {
				Physical_Int numFace = mesh.m_numPoints / 3;
				for (int i = 0; i < numFace; ++i) {
					int startIndex = i * 3;
					Box triangleBox;
					for (int v = 0; v < 3; ++v) {
						const auto& vertex = mesh.m_pVec3[startIndex++];
						if (vertex.x() < triangleBox.m_min.x()) triangleBox.m_min.x() = vertex.x();
						if (vertex.y() < triangleBox.m_min.y()) triangleBox.m_min.y() = vertex.y();
						if (vertex.z() < triangleBox.m_min.z()) triangleBox.m_min.z() = vertex.z();
						if (vertex.x() > triangleBox.m_max.x()) triangleBox.m_max.x() = vertex.x();
						if (vertex.y() > triangleBox.m_max.y()) triangleBox.m_max.y() = vertex.y();
						if (vertex.z() > triangleBox.m_max.z()) triangleBox.m_max.z() = vertex.z();
					}
					if (detect(triangleBox, box)) {
						return true;
					}
				}
			}
			return false;
		}
	};

	struct PHYSICAL_EXPORT TestResult
	{
		std::vector<CollideInfo> m_vecCollideInfo;
	};

	class PHYSICAL_EXPORT CollideActor
	{
	public:
		void collide(std::shared_ptr<Object> obj, std::shared_ptr<Object> otherObj, Physical_Vec3& trans, TestResult& res) const
		{
			// 构建包含trans路径的三维体，并判断是否碰撞，并给出碰撞后的反馈trans
			auto objShape = obj->m_shape;
			auto otherShape = otherObj->m_shape;

			switch (objShape->m_type)
			{
			case Shape::Type::BoxShape: {
				auto objBox = static_cast<Box*>(objShape.get());
				switch (otherShape->m_type)
				{
				case Shape::Type::BoxShape: {
					auto otherBox = static_cast<Box*>(otherShape.get());
					CollideFunctor cf;
					if (cf.detect(*objBox, *otherBox)) {
						CollideInfo info;
						info.m_collideObject = otherObj;
						res.m_vecCollideInfo.emplace_back(std::move(info));
					}
					break;
				}
				default:
					break;
				}

				return;
			}
			case Shape::Type::MeshShape: {
				auto objMesh = static_cast<Mesh*>(objShape.get());
				switch (otherShape->m_type)
				{
				case Shape::Type::MeshShape: {
					auto otherMesh = static_cast<Mesh*>(otherShape.get());
					CollideFunctor cf;

					auto meshBox = objMesh->m_box;
					meshBox.m_min = meshBox.m_min * obj->m_matrix;
					meshBox.m_max = meshBox.m_max * obj->m_matrix;

					auto otherMeshBox = otherMesh->m_box;
					otherMeshBox.m_min = otherMeshBox.m_min * otherObj->m_matrix;
					otherMeshBox.m_max = otherMeshBox.m_max * otherObj->m_matrix;

					if (cf.detect(meshBox, otherMeshBox)) {
						if (obj->m_collideDetectLevel == CollideDetectLevel::Primitive) {
							if (otherObj->m_collideDetectLevel == CollideDetectLevel::Bound) {
								if (cf.detect(*objMesh, otherMeshBox)) {
									CollideInfo info;
									info.m_collideObject = otherObj;
									res.m_vecCollideInfo.emplace_back(std::move(info));
								}
							}
							else {
								if (cf.detect(*objMesh, *otherMesh)) {
									CollideInfo info;
									info.m_collideObject = otherObj;
									res.m_vecCollideInfo.emplace_back(std::move(info));
								}
							}
						}
						else {
							if (otherObj->m_collideDetectLevel == CollideDetectLevel::Bound) {
								CollideInfo info;
								info.m_collideObject = otherObj;
								res.m_vecCollideInfo.emplace_back(std::move(info));
							}
							else {
								if (cf.detect(*otherMesh, meshBox)) {
									CollideInfo info;
									info.m_collideObject = otherObj;
									res.m_vecCollideInfo.emplace_back(std::move(info));
								}
							}

						}
					}
					break;
				}
				default:
					break;
				}
				return;
			}
			default:
				break;
			}
		}

		void collide(std::shared_ptr<Object> obj, std::shared_ptr<Object> otherObj, Physical_Quat& rotation, TestResult& res) const
		{
			// 构建包含rotation路径的三维体，并判断是否碰撞，并给出碰撞后的反馈rotation
		}

	protected:
		void apply()
		{

		}
	};

	class PHYSICAL_EXPORT Time
	{
	public:

	};

	class PHYSICAL_EXPORT PhysicalEngine
	{
	public:
		PhysicalEngine() {
			m_time.reset(new Time);
		}

		virtual ~PhysicalEngine();

		void addObject(std::shared_ptr<Object> obj) {
			m_objects.push_back(obj);
		}

		void addField(std::shared_ptr<Field> field) {
			m_fields.push_back(field);
		}

		TestResult testObject(std::shared_ptr<Object> obj, Physical_Vec3& newTransform) {
			TestResult res;
			for (auto& object : m_objects) {
				if (obj == object) {
					continue;
				}
				m_collideActor->collide(obj, object, newTransform, res);
			}
			if (!res.m_vecCollideInfo.empty()) {
				newTransform.set(0, 0, 0);
			}
			return res;
		}

		TestResult testObject(std::shared_ptr<Object> obj, Physical_Quat& newQuat) {
			TestResult res;
			for (auto& object : m_objects) {
				if (obj == object) {
					continue;
				}
				m_collideActor->collide(obj, object, newQuat, res);
			}
			if (!res.m_vecCollideInfo.empty()) {
				newQuat = Physical_Quat();
			}
			return res;
		}

		void exec()
		{
			//物理引擎内循环
		}

		void forward() {
			// 对物理空间中所有的力作下一步推演、计算、处理
		}

	protected:
		void forwardImplementation(Time&) {

		}

	protected:
		std::vector<std::shared_ptr<Object>> m_objects;
		std::vector<std::shared_ptr<Field>> m_fields;

		std::shared_ptr<CollideActor> m_collideActor;

		std::shared_ptr<Time> m_time;
		//std::thread m_workThread;
	};

}
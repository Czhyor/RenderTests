#include "read_stp.h"
#include <gp_Pln.hxx>
#include <GC_MakeSegment.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Solid.hxx>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include <BRepPrimAPI_MakePrism.hxx>

#include <IMeshData_Model.hxx>
#include <IMeshTools_Parameters.hxx>
#include <BRepMesh_IncrementalMesh.hxx>

#include <TopExp_Explorer.hxx>

#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <Poly_Triangulation.hxx>

#include <STEPControl_Reader.hxx>

ModelData testCAD()
{
	ModelData modelData;
	modelData.m_vertexArray = new osg::Vec3Array;
	auto& vecV = modelData.m_vertexArray->asVector();
	modelData.m_normalArray = new osg::Vec3Array;
	auto& vecN = modelData.m_normalArray->asVector();

	float myWidth = 100;
	float myHeight = 150;
	float myThickness = 40;

	// build bottle's profile
	gp_Pnt aPnt1(-myWidth / 2., 0, 0);
	gp_Pnt aPnt2(-myWidth / 2., -myThickness / 4., 0);
	gp_Pnt aPnt3(0, -myThickness / 2., 0);
	gp_Pnt aPnt4(myWidth / 2., -myThickness / 4., 0);
	gp_Pnt aPnt5(myWidth / 2., 0, 0);

	Handle(Geom_TrimmedCurve) aArcOfCircle = GC_MakeArcOfCircle(aPnt2, aPnt3, aPnt4);
	Handle(Geom_TrimmedCurve) aSegment1 = GC_MakeSegment(aPnt1, aPnt2);
	Handle(Geom_TrimmedCurve) aSegment2 = GC_MakeSegment(aPnt4, aPnt5);

	TopoDS_Edge anEdge1 = BRepBuilderAPI_MakeEdge(aSegment1);
	TopoDS_Edge anEdge2 = BRepBuilderAPI_MakeEdge(aArcOfCircle);
	TopoDS_Edge anEdge3 = BRepBuilderAPI_MakeEdge(aSegment2);

	TopoDS_Wire aWire = BRepBuilderAPI_MakeWire(anEdge1, anEdge2, anEdge3);

	gp_Pnt aOrigin(0, 0, 0);
	gp_Dir xDir(1, 0, 0);
	gp_Ax1 xAxis(aOrigin, xDir);
	gp_Trsf aTrsf;
	aTrsf.SetMirror(xAxis);

	BRepBuilderAPI_Transform aBRepTrsf(aWire, aTrsf);
	TopoDS_Shape aMirroredShape = aBRepTrsf.Shape();
	TopoDS_Wire aMirroredWire = TopoDS::Wire(aMirroredShape);

	BRepBuilderAPI_MakeWire mkWire;
	mkWire.Add(aWire);
	mkWire.Add(aMirroredWire);
	TopoDS_Wire myWireProfile = mkWire.Wire();

	// build bottle's body
	TopoDS_Face myFaceProfile = BRepBuilderAPI_MakeFace(myWireProfile);

	gp_Vec aPrismVec(0, 0, myHeight);
	TopoDS_Shape myBody = BRepPrimAPI_MakePrism(myFaceProfile, aPrismVec);

#if 1
	const Standard_Real aLinearDeflection = 0.1;
	const Standard_Real anAngularDeflection = 0.5;
	BRepMesh_IncrementalMesh aMesher(myBody, aLinearDeflection, Standard_False, anAngularDeflection, Standard_True);
#else
	IMeshTools_Parameters aMeshParams;
	aMeshParams.Deflection = 0.01;
	aMeshParams.Angle = 0.5;
	aMeshParams.Relative = Standard_False;
	aMeshParams.InParallel = Standard_True;
	aMeshParams.MinSize = Precision::Confusion();
	aMeshParams.InternalVerticesMode = Standard_True;
	aMeshParams.ControlSurfaceDeflection = Standard_True;
	BRepMesh_IncrementalMesh aMesher(myBody, aMeshParams);
#endif

	const Standard_Integer aStatus = aMesher.GetStatusFlags();

	const TopoDS_Shape& shape = aMesher.Shape();
	opencascade::handle<TopoDS_TShape> tshape = shape.TShape();

	TopExp_Explorer expFace(shape, TopAbs_FACE);
	while (expFace.More()) {
		const TopoDS_Face& f = TopoDS::Face(expFace.Current());
		TopAbs_Orientation faceOrientation = f.Orientation();

		TopLoc_Location location;
		Handle(Poly_Triangulation) aTr = BRep_Tool::Triangulation(f, location);

		if (!aTr.IsNull()) {
			Standard_Integer nbNodes, nbTriangles;
			nbNodes = aTr->NbNodes();
			nbTriangles = aTr->NbTriangles();

			//const TColgp_Array1OfPnt& aNodes = aTr->NbNodes();
			const Poly_Array1OfTriangle& triangles = aTr->Triangles();
			//const TColgp_Array1OfPnt2d& uvNodes = aTr->UVNode();

			TColgp_Array1OfPnt points(1, nbNodes);
			for (int i = 1; i <= nbNodes; ++i) {
				gp_Pnt node = aTr->Node(i);
				points(i) = node.Transformed(location);
			}

			Standard_Integer n1, n2, n3;
			for (int i = 1; i <= nbTriangles; ++i) {
				const Poly_Triangle& triangle = aTr->Triangle(i);
				triangle.Get(n1, n2, n3);
				gp_Pnt pt1 = points(n1);
				gp_Pnt pt2 = points(n2);
				gp_Pnt pt3 = points(n3);

				osg::Vec3 v0, v1, v2;
				if (faceOrientation == TopAbs_Orientation::TopAbs_FORWARD) {
					v0.set(pt1.X(), pt1.Y(), pt1.Z());
					v1.set(pt2.X(), pt2.Y(), pt2.Z());
					v2.set(pt3.X(), pt3.Y(), pt3.Z());
				}
				else {
					v0.set(pt3.X(), pt3.Y(), pt3.Z());
					v1.set(pt2.X(), pt2.Y(), pt2.Z());
					v2.set(pt1.X(), pt1.Y(), pt1.Z());
				}

				vecV.push_back(v0);
				vecV.push_back(v1);
				vecV.push_back(v2);

				osg::Vec3 normal = (v1 - v0) ^ (v2 - v0);
				normal.normalize();
				vecN.push_back(normal);
				vecN.push_back(normal);
				vecN.push_back(normal);

			}

		}

		expFace.Next();
	}

	qDebug() << "shape:" << &shape << shape.ShapeType();

	qDebug() << "aStatus:" << aStatus;

	return modelData;
}

void processFace(TopoDS_Shape& shape, std::vector<osg::Vec3>& vecV, std::vector<osg::Vec3>& vecN)
{
	const Standard_Real aLinearDeflection = 0.1;
	const Standard_Real anAngularDeflection = 0.5;
	BRepMesh_IncrementalMesh aMesher(shape, aLinearDeflection, Standard_False, anAngularDeflection, Standard_True);

	const TopoDS_Shape& aShape = aMesher.Shape();
	TopExp_Explorer aExpFace(aShape, TopAbs_FACE);
	while (aExpFace.More()) {
		const TopoDS_Face& f = TopoDS::Face(aExpFace.Current());
		TopAbs_Orientation faceOrientation = f.Orientation();

		TopLoc_Location location;
		Handle(Poly_Triangulation) aTr = BRep_Tool::Triangulation(f, location);
		if (!aTr.IsNull()) {
			Standard_Integer nbNodes = aTr->NbNodes();
			Standard_Integer nbTriangles = aTr->NbTriangles();

			TColgp_Array1OfPnt points(1, nbNodes);
			for (int i = 1; i <= nbNodes; ++i) {
				gp_Pnt node = aTr->Node(i);
				points(i) = node.Transformed(location);
			}

			Standard_Integer n1, n2, n3;
			for (int i = 1; i <= nbTriangles; ++i) {
				const Poly_Triangle& triangle = aTr->Triangle(i);
				triangle.Get(n1, n2, n3);
				gp_Pnt& pt1 = points(n1);
				gp_Pnt& pt2 = points(n2);
				gp_Pnt& pt3 = points(n3);

				osg::Vec3 v0, v1, v2;
				if (faceOrientation == TopAbs_Orientation::TopAbs_FORWARD) {
					v0.set(pt1.X(), pt1.Y(), pt1.Z());
					v1.set(pt2.X(), pt2.Y(), pt2.Z());
					v2.set(pt3.X(), pt3.Y(), pt3.Z());
				}
				else {
					v0.set(pt3.X(), pt3.Y(), pt3.Z());
					v1.set(pt2.X(), pt2.Y(), pt2.Z());
					v2.set(pt1.X(), pt1.Y(), pt1.Z());
				}

				vecV.push_back(v0);
				vecV.push_back(v1);
				vecV.push_back(v2);

				osg::Vec3 normal = (v1 - v0) ^ (v2 - v0);
				normal.normalize();
				vecN.push_back(normal);
				vecN.push_back(normal);
				vecN.push_back(normal);
			}
		}

		aExpFace.Next();
	}
}

ModelData readSTP(const std::string& fileName)
{
	//return testCAD();

	ModelData data;

	STEPControl_Reader reader;
	IFSelect_ReturnStatus retStat = reader.ReadFile(fileName.c_str());
	if (retStat != IFSelect_ReturnStatus::IFSelect_RetDone) {
		qDebug() << "read stp failed:" << retStat;
		return data;
	}

	reader.PrintCheckLoad(false, IFSelect_PrintCount::IFSelect_ItemsByEntity);

	Standard_Integer nbRoots = reader.NbRootsForTransfer();
	qDebug() << "NbRootsForTransfer:" << nbRoots;
	Standard_Integer num = reader.TransferRoots();
	qDebug() << "TransferRoots num:" << num;

	reader.PrintCheckTransfer(false, IFSelect_PrintCount::IFSelect_ItemsByEntity);

	Standard_Integer nbShapes = reader.NbShapes();
	qDebug() << "NbShapes:" << nbShapes;

	if (nbShapes == 0) {
		return data;
	}

	data.m_vertexArray = new osg::Vec3Array;
	data.m_normalArray = new osg::Vec3Array;
	auto& vecV = data.m_vertexArray->asVector();
	auto& vecN = data.m_normalArray->asVector();

	for (int i = 1; i <= nbShapes; ++i) {
		TopoDS_Shape shape = reader.Shape(i);

		TopAbs_ShapeEnum shapeType = shape.ShapeType();
		switch (shapeType)
		{
		case TopAbs_COMPOUND:
		case TopAbs_COMPSOLID:
		case TopAbs_SOLID:
		case TopAbs_SHELL:
		case TopAbs_FACE:
			processFace(shape, vecV, vecN);
			break;
		case TopAbs_WIRE:
			break;
		case TopAbs_EDGE:
			break;
		case TopAbs_VERTEX:
			break;
		case TopAbs_SHAPE:
			break;
		default:
			break;
		}

		
	}

	return data;
}
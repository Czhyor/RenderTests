#include "read_stl.h"

ModelData readSTL(const std::string& fileName) {
	std::ifstream in;
	in.open(fileName.data(), in.binary | in.in);
	if (!in.is_open()) {
		printf("open file failed: %s\n", fileName.data());
	}

	in.seekg(in.beg + 80);
	int numTriangles = 0;
	in.read((char*)(&numTriangles), sizeof(int));
	printf("num vertex: %d\n", numTriangles);

	struct STLUnit {
		osg::Vec3 faceNormal;
		osg::Vec3 v0;
		osg::Vec3 v1;
		osg::Vec3 v2;
		char _reserved[2];
	};

	size_t dataSize = numTriangles * 50;
	char* tmpMem = (char*)malloc(dataSize);
	memset(tmpMem, 0, dataSize);
	in.read((char*)tmpMem, dataSize);
	in.close();

	ModelData data;
	data.m_vertexArray = new osg::Vec3Array;
	data.m_normalArray = new osg::Vec3Array;
	data.m_vertexArray->reserve(numTriangles * 3);
	data.m_normalArray->reserve(numTriangles * 3);
	auto& vecV = data.m_vertexArray->asVector();
	auto& vecN = data.m_normalArray->asVector();
	char* cur = tmpMem;
	STLUnit unit;
	for (int i = 0; i < numTriangles; ++i) {
		memcpy(&unit, cur, 50);
		vecV.push_back(unit.v0);
		vecV.push_back(unit.v1);
		vecV.push_back(unit.v2);
		vecN.push_back(unit.faceNormal);
		vecN.push_back(unit.faceNormal);
		vecN.push_back(unit.faceNormal);
		cur += 50;
	}
	free(tmpMem);
	tmpMem = nullptr;

	return data;
}

void convertToIndexed(ModelData& data) {
	if (data.m_drawElement.valid()) {
		return;
	}

	//std::bsearch

	data.m_drawElement->resize(data.m_vertexArray->getNumElements() / 3 * 3);
	auto& vecE = data.m_drawElement->asVector();
	const auto& vecV = data.m_vertexArray->asVector();

	std::map<osg::Vec3, int> vertices;
	int vertexID = 0;
	for (int i = 0; i < vecV.size(); ++i) {
		auto finder = vertices.find(vecV[i]);
		if (finder == vertices.end()) {
			vertices.insert(std::make_pair(vecV[i], vertexID++));
		}
	}

	int numFace = vecV.size() / 3;
	for (int i = 0; i < numFace; ++i) {
		for (int j = 0; j < 3; ++j) {
			auto finder = vertices.find(vecV[i * 3 + j]);
			if (finder != vertices.end()) {
				vecE[i * 3 + j] = finder->second;
			}
		}
	}

	std::vector<std::pair<osg::Vec3, int>> orderedVertices;
	for (auto& itr : vertices) {
		orderedVertices.push_back(itr);
	}

	std::sort(orderedVertices.begin(), orderedVertices.end(), [](const std::pair<osg::Vec3, int>& first, const std::pair<osg::Vec3, int>& second) {
		return first.second < second.second;
		});

	osg::Vec3Array* newVArray = new osg::Vec3Array;
	//osg::Vec3Array* newNArray = new osg::Vec3Array;
	newVArray->reserve(data.m_vertexArray->getNumElements());
	//newNArray->reserve(data.m_vertexArray->getNumElements());
	auto& newVecV = newVArray->asVector();
	//auto& newVecN = newNArray->asVector();
	for (auto& itr : orderedVertices) {
		newVecV.push_back(itr.first);
	}
}
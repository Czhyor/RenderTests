#include "drawable.h"

Drawable::Drawable()
{

}

Drawable::~Drawable()
{

}

osg::ref_ptr<osg::Geometry> Drawable::createGeometry()
{
	osg::Geometry* geometry = new osg::Geometry;
	geometry->setUseDisplayList(false);
	geometry->setUseVertexBufferObjects(true);
	geometry->setVertexAttribArray(Vertex, m_data.m_vertexArray, osg::Array::BIND_PER_VERTEX);
	geometry->setVertexAttribArray(Normal, m_data.m_normalArray, osg::Array::BIND_PER_VERTEX);
	geometry->setVertexAttribArray(Color, m_data.m_colorArray, osg::Array::BIND_PER_VERTEX);
	geometry->setVertexAttribArray(State, m_data.m_stateArray, osg::Array::BIND_PER_VERTEX);
	geometry->setVertexAttribArray(UV, m_data.m_uvArray, osg::Array::BIND_PER_VERTEX);
	geometry->setVertexAttribArray(Weight, m_data.m_weightArray, osg::Array::BIND_PER_VERTEX);
	return geometry;
}

void Drawable::setModelData(const ModelData& data)
{
	m_data = data;
}

const ModelData& Drawable::getModelData() const
{
	return m_data;
}

osg::ref_ptr<osg::Geometry> Mesh::createGeometry()
{
	auto geometry = Drawable::createGeometry();
	if (m_data.m_drawElement.valid()) {
		geometry->addPrimitiveSet(m_data.m_drawElement);
	}
	else {
		geometry->addPrimitiveSet(new osg::DrawArrays(
			osg::PrimitiveSet::Mode::TRIANGLES, 0, m_data.m_vertexArray->getNumElements()));
	}
	return geometry;
}

osg::ref_ptr<osg::Geometry> Line::createGeometry()
{
	auto geometry = Drawable::createGeometry();
	if (m_data.m_drawElement.valid()) {
		geometry->addPrimitiveSet(m_data.m_drawElement);
	}
	else {
		geometry->addPrimitiveSet(new osg::DrawArrays(
			osg::PrimitiveSet::Mode::LINES, 0, m_data.m_vertexArray->getNumElements()));
	}
	return geometry;
}

osg::ref_ptr<osg::Geometry> Point::createGeometry()
{
	auto geometry = Drawable::createGeometry();
	if (m_data.m_drawElement.valid()) {
		geometry->addPrimitiveSet(m_data.m_drawElement);
	}
	else {
		geometry->addPrimitiveSet(new osg::DrawArrays(
			osg::PrimitiveSet::Mode::POINTS, 0, m_data.m_vertexArray->getNumElements()));
	}
	return geometry;
}
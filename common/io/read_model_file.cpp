#include "read_model_file.h"
#include "read_stl.h"
#include "read_stp.h"

ReadModelFile::ReadModelFile(const QString& filePath) :
	m_filePath(filePath)
{

}

ReadModelFile::~ReadModelFile()
{

}

bool ReadModelFile::read()
{
	qDebug() << "filePath:" << m_filePath;
	if (m_filePath.isEmpty()) {
		return false;
	}

	QString filePath = m_filePath;
	auto index = m_filePath.indexOf("file:///");
	if (index >= 0) {
		filePath = m_filePath.mid(index + 8);
	}

	index = filePath.lastIndexOf(".");
	if (index < 0) {
		qWarning() << "can't find suffix";
		return false;
	}

	QString suffix = filePath.mid(index);
	qDebug() << "suffix:" << suffix;

	index = filePath.lastIndexOf("/");
	if (index >= 0) {
		m_modelFileName = filePath.mid(index + 1);
	}

	if (suffix == ".stl") {
		m_modelData = readSTL(filePath.toStdString());
		return true;
	}
	else if (suffix == ".stp" || suffix == ".step") {
		m_modelData = readSTP(filePath.toStdString());
	}
	else {
		qWarning() << "unsupport suffix:" << suffix;
	}

	return false;
}

const ModelData& ReadModelFile::getModelData() const
{
	return m_modelData;
}

const QString& ReadModelFile::getFilePath() const
{
	return m_filePath;
}

const QString& ReadModelFile::getModelFileName() const
{
	return m_modelFileName;
}
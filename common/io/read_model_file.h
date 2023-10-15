#pragma once

#include "model_data.h"
#include <QString>

class COMMON_EXPORT ReadModelFile
{
public:
	ReadModelFile(const QString& filePath);
	~ReadModelFile();

	bool read();
	const ModelData& getModelData() const;
	const QString& getFilePath() const;
	const QString& getModelFileName() const;
protected:
	ModelData m_modelData;
	QString m_filePath;
	QString m_modelFileName;
};
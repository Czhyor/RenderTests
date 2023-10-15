#pragma once

#include "canvas3d_export.h"
#include <osg/Program>
#include <mutex>

class CANVAS_EXPORT ShaderMgr
{
public:
	static const std::string s_meshProgram;
	static const std::string s_deferedMeshProgram;

	ShaderMgr();
	~ShaderMgr();

	static ShaderMgr* instance();

	void init();
	osg::Program* getShader(const std::string& name);
	bool addShader(osg::Program* program);

protected:
	ShaderMgr(ShaderMgr&) = delete;
	ShaderMgr(ShaderMgr&&) = delete;
	ShaderMgr& operator=(const ShaderMgr&) = delete;

	std::vector<osg::ref_ptr<osg::Program>> m_vecPrograms;
	std::mutex m_mutex;
private:
	static ShaderMgr* s_instance;
	static std::mutex s_mtx;
};
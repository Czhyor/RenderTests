#ifndef DEFERRED_RENDERING_H
#define DEFERRED_RENDERING_H

#include <osg/Texture2D>
#include <osg/TextureCubeMap>
#include <osg/Camera>
#include <osg/Geometry>

extern osg::Texture2D* createTexture(int width, int height);

extern osg::Texture2D* createDepthTexture(int width, int height);

extern osg::TextureCubeMap* createCubeMapColorTexture(int width, int height);

extern osg::TextureCubeMap* createCubeMapDepthTexture(int width, int height);

extern osg::Camera* createRTTCamera(int width, int height);

extern osg::Geometry* createRectForTexture(osg::Texture2D* baseColorTexture,
	osg::Texture2D* normalTexture, osg::Texture2D* depthTexture);

#endif
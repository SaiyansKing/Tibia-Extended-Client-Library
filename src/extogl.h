#ifndef __EXTENDEDOGL_H
#define __EXTENDEDOGL_H

#include <map>
#include <GL/gl.h>
#include "extEngines.h"

class ExtendedEngineOGL : public ExtendedEngine
{
	public:
		ExtendedEngineOGL() {;}
		~ExtendedEngineOGL() {;}

		virtual int getRenderId() {return OGL_RENDER;}
		virtual void LoadSprite(int surface, int x, int y, int w, int h, void* data);

		virtual void enableAlpha();
		virtual void disableAlpha();
};

#endif

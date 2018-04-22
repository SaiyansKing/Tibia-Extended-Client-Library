#ifndef __EXTENDEDDX9_H
#define __EXTENDEDDX9_H

#include <map>
#include <d3d9.h>
#include "extEngines.h"

class ExtendedEngineDX9 : public ExtendedEngine
{
	public:
		ExtendedEngineDX9() {;}
		~ExtendedEngineDX9() {;}

		virtual int getRenderId() {return DX9_RENDER;}
		virtual void LoadSprite(int surface, int x, int y, int w, int h, void* data);

		virtual void enableAlpha() {;}//don't need these beacuse alpha channel is enabled by default
		virtual void disableAlpha() {;}
};

#endif

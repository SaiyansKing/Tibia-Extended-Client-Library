#ifndef __SPRITES_H
#define __SPRITES_H

#include "main.h"

class Sprites
{
	public:
		Sprites(const char *filename, const char* readType);
		virtual ~Sprites();

		void sprSeek(unsigned long position);
		void sprRead(void* buf, unsigned long size);
		unsigned char sprGetC();
		bool sprLoad() {return FilesLoad;}

	protected:
		char* FilesData;
		bool FilesLoad;
		unsigned long spriteOffset;
};

#endif

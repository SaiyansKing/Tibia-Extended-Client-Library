#include "sprites.h"

#ifdef __CONFIG__
extern bool should_use_cached_sprites;
#endif
//todo let user chose if he want to use ram memory to cache

Sprites::Sprites(const char *filename, const char* readType)
{
	FilesLoad = false;

	FILE* sprFile = fopen(filename, readType);
	if(!sprFile)
		return;

	unsigned long dSize = 0;
	fseek(sprFile, 0, SEEK_END);
	dSize = ftell(sprFile);
	rewind(sprFile);

	FilesData = (char*)malloc(dSize);
	if(!FilesData)
	{
		fclose(sprFile);
		MessageBox(NULL, "Cannot cache client .spr file.", PROJECT_NAME, MB_OK|MB_ICONERROR);
		exit(-1);
		return;
	}

	if(fread(FilesData, 1, dSize, sprFile) != dSize)
	{
		fclose(sprFile);
		return;
	}

	fclose(sprFile);

	spriteOffset = 0;
	FilesLoad = true;
}

Sprites::~Sprites()
{
	free(FilesData);
}

void Sprites::sprSeek(unsigned long position)
{
	spriteOffset = position;
}

void Sprites::sprRead(void* buf, unsigned long size)
{
	memcpy(buf, FilesData+spriteOffset, size);
	spriteOffset += size;
}

unsigned char Sprites::sprGetC()
{
	unsigned char v;
	sprRead(&v, 1);
	return v;
}

#include "main.h"
#include "extogl.h"

void ExtendedEngineOGL::LoadSprite(int surface, int x, int y, int w, int h, void* data)
{
	unsigned char* rgbaPixels = LoadSpriteAlpha((uint32_t)data);//data = spriteid instead of pixelbuffer because of some patch
	if(!rgbaPixels)
		return;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, *(GLuint*)(GetEngineAddr()+0x1D4+surface*32));
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels);
	free(rgbaPixels);
}

void ExtendedEngineOGL::enableAlpha()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
}

void ExtendedEngineOGL::disableAlpha()
{
	glDisable(GL_BLEND);
}

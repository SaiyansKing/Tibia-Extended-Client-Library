#include "main.h"
#include "extdx9.h"

void ExtendedEngineDX9::LoadSprite(int surface, int x, int y, int w, int h, void* data)
{
	unsigned char* rgbaPixels = LoadSpriteAlpha((uint32_t)data);//data = spriteid instead of pixelbuffer because of some patch
	if(!rgbaPixels)
		return;

	RECT d3drect;
	d3drect.left = x;
	d3drect.right = x + w;
	d3drect.top = y;
	d3drect.bottom = y + h;
	IDirect3DTexture9* texture = (IDirect3DTexture9*)(*(DWORD*)(GetEngineAddr()+0x200+surface*4));

	D3DLOCKED_RECT rect;
	HRESULT hr = IDirect3DTexture9_LockRect(texture, 0, &rect, &d3drect, D3DLOCK_NOSYSLOCK);
	if(FAILED(hr))
	{
		free(rgbaPixels);
		return;
	}

	unsigned char* tBits = (unsigned char*)rect.pBits;

	uint32_t readData = 0, pixel = 0;
	for(int j = 0; j < h; j++)
	{
		for(int k = 0; k < w; k++)
		{
			pixel = j*rect.Pitch+k*4;
			tBits[pixel] = rgbaPixels[readData+2];//use bgra format instead of rgba
			tBits[pixel+1] = rgbaPixels[readData+1];
			tBits[pixel+2] = rgbaPixels[readData];
			tBits[pixel+3] = rgbaPixels[readData+3];
			readData += 4;
		}
	}

	IDirect3DTexture9_UnlockRect(texture, 0);
	free(rgbaPixels);
}

#include "config.h"
#include "extdx9.h"
#include "extogl.h"
#include "sprites.h"
#include "timer.h"
#include "hook.h"

//There aren't hardware acceleration in DirectDraw so even if you use transparency it won't work on DirectDraw(correct me if I wrong and send me some examples)(maybe later I rewrite usage of DirectDraw to some other api)
//Cipsoft Client draws sprites different than OTClient so there can be some glitches using transparency on DX9, OGL

struct Render_NEW
{
	DWORD padds1[5];
	void (__stdcall *DrawRectangle) (DWORD nSurface, DWORD X, DWORD Y, DWORD W, DWORD H, DWORD nRed, DWORD nGreen, DWORD nBlue);
	DWORD padds2[4];
	void  __stdcall (*LoadSprite) (int surface, int x, int y, int w, int h, void* data);
};

Render_NEW *newRenderer;

HMODULE orig_ddraw;
const char* prjInfo = PROJECT_INFO;

DWORD client_Version;
DWORD client_BaseAddr;
DWORD client_pointerTransPixels;
bool client_extended;
bool client_transparent;

#ifdef __CONFIG__
bool should_use_hirestimer;
bool should_use_extended;
bool should_use_alpha;
bool should_use_cached_sprites;
bool should_draw_manabar;
#endif

ExtendedEngine* transEngine;

Sprites* spritesFile;

typedef const char* (*_GetFileName) (int fileId, bool backup);
static _GetFileName GetFileName;

DWORD PlayerHealthAddr;
DWORD PlayerHealthMaxAddr;
DWORD PlayerManaAddr;
DWORD PlayerManaMaxAddr;
DWORD PlayerIDAddr;

DWORD CreateGlContext;
DWORD DeleteGlContext;
DWORD CreateDX9Context;
DWORD DeleteDX9Context;
DWORD CreateDX7Context;
DWORD DeleteDX7Context;
DWORD SpriteContext;
BYTE SpriteContextPos;
BYTE SpriteContextNeg;

_GetEngineAddr GetEngineAddr;
_DrawSkin DrawSkin;
_PrintText PrintText;

unsigned char* LoadSpriteAlpha(uint32_t sprite)
{
	unsigned char* pixels = (unsigned char*)malloc(4096);
	if(!pixels)
		return NULL;

	for(int i = 0; i < 1024; i++)
		pixels[i*4+3] = 0x00;

	uint32_t pointer = *(DWORD*)((*(DWORD*)client_pointerTransPixels)-0x10+sprite*0x10);
	if(pointer == 0)
		return pixels;

	spritesFile->sprSeek(pointer);

	//Ignore color key
	spritesFile->sprGetC();
	spritesFile->sprGetC();
	spritesFile->sprGetC();

	uint16_t sprSize;
	spritesFile->sprRead(&sprSize, 2);
	if(sprSize == 0)
		return pixels;

	uint32_t writeData = 0, readData = 0;
	uint16_t numPix;
	bool state = false;
	while(readData < sprSize)
	{
		spritesFile->sprRead(&numPix, 2);
		readData += 2;
		if(state)
		{
			for(int i = 0; i < numPix; i++)
			{
				pixels[writeData++] = spritesFile->sprGetC();
				pixels[writeData++] = spritesFile->sprGetC();
				pixels[writeData++] = spritesFile->sprGetC();
				readData += 3;
				if(client_transparent)
				{
					pixels[writeData++] = spritesFile->sprGetC();
					readData++;
				}
			}

			state = false;
		}
		else
		{
			writeData += numPix*4;
			state = true;
		}
	}

	return pixels;
}

uint32_t HookPointers()
{
	uint32_t v;
	spritesFile->sprRead(&v, 4);
	return v;
}

uint32_t HookSygnature()
{
	return 0x44545845; //'EXTD'
}

uint32_t HookNumSprites()
{
	uint32_t numSprites = 0;

	spritesFile = new Sprites(GetFileName(4, false), "rb");
	if(spritesFile && spritesFile->sprLoad())
	{
		spritesFile->sprSeek(4);
		if(client_extended)
			spritesFile->sprRead(&numSprites, 4);
		else
		{
			uint16_t u16Read;
			spritesFile->sprRead(&u16Read, 2);
			numSprites = u16Read;
		}
	}
	else
	{
		MessageBox(NULL, "Cannot read client .spr file.", PROJECT_NAME, MB_OK|MB_ICONERROR);
		exit(-1);
	}

	return numSprites;
}

void HookLoadSprite(uint32_t sprite, unsigned char* pixels)
{
	DWORD pointerTransPixels = *(DWORD*)client_pointerTransPixels;
	DWORD *transPixels = (DWORD *)(pointerTransPixels-0x10+sprite*0x10);
	if(transPixels[0] == 0)
	{
		//Write standard color key to make empty sprite
		transPixels[1] = 0xFF;
		transPixels[2] = 0x00;
		transPixels[3] = 0xFF;
		for(int i = 0; i < 1024; i++)
		{
			pixels[i*3] = 0xFF;
			pixels[i*3+1] = 0x00;
			pixels[i*3+2] = 0xFF;
		}
		return;
	}

	spritesFile->sprSeek(transPixels[0]);

	//Write color key
	unsigned char R = spritesFile->sprGetC();
	unsigned char G = spritesFile->sprGetC();
	unsigned char B = spritesFile->sprGetC();
	transPixels[1] = static_cast<DWORD>(R);
	transPixels[2] = static_cast<DWORD>(G);
	transPixels[3] = static_cast<DWORD>(B);
	for(int i = 0; i < 1024; i++)
	{
		pixels[i*3] = R;
		pixels[i*3+1] = G;
		pixels[i*3+2] = B;
	}

	uint16_t sprSize;
	spritesFile->sprRead(&sprSize, 2);
	if(sprSize == 0)
		return;

	uint32_t writeData = 0, readData = 0;
	uint16_t numPix;
	bool state = false;
	while(readData < sprSize)
	{
		spritesFile->sprRead(&numPix, 2);
		readData += 2;
		if(state)
		{
			for(int i=0;i<numPix;i++)
			{
				pixels[writeData++] = spritesFile->sprGetC();
				pixels[writeData++] = spritesFile->sprGetC();
				pixels[writeData++] = spritesFile->sprGetC();
				readData += 3;
				if(client_transparent)
				{
					spritesFile->sprGetC();
					readData++;
				}
			}

			state = false;
		}
		else
		{
			writeData += numPix*3;
			state = true;
		}
	}
}

void __stdcall MyLoadSprite(int surface, int x, int y, int w, int h, void* data)
{
	if(transEngine)
		transEngine->LoadSprite(surface, x, y, w, h, data);
}

DWORD HookExtendedEngine()
{
	if(transEngine)
		return (DWORD)&newRenderer;
	else
		return GetEngineAddr();
}

bool __stdcall HookCreateGLContext()
{
	bool created = ((bool (__stdcall *)())CreateGlContext)();
	if(created)
	{
		OverWriteByte(SpriteContext, SpriteContextPos);
		if(!transEngine)
			transEngine = new ExtendedEngineOGL();
	}

	return created;
}

void __stdcall HookDeleteGLContext()
{
	OverWriteByte(SpriteContext, SpriteContextNeg);
	if(transEngine && transEngine->getRenderId() == OGL_RENDER)
	{
		delete transEngine;
		transEngine = NULL;
	}

	((void (__stdcall *)())DeleteGlContext)();
}

void __stdcall HookGLEnableAlpha(GLenum cap)
{
	if(transEngine)
		transEngine->enableAlpha();

	glEnable(cap);
}

void __stdcall HookGLDisableAlpha(void)
{
	glEnd();
	if(transEngine)
		transEngine->disableAlpha();
}

void __fastcall HookCreateDX9Context(DWORD dx9engine, int)
{
	((void (__fastcall *)(DWORD, int))CreateDX9Context)(dx9engine, 0);
	OverWriteByte(SpriteContext, SpriteContextPos);
	if(!transEngine)
		transEngine = new ExtendedEngineDX9();
}

void __fastcall HookDeleteDX9Context(DWORD dx9engine, int)
{
	OverWriteByte(SpriteContext, SpriteContextNeg);
	if(transEngine && transEngine->getRenderId() == DX9_RENDER)
	{
		delete transEngine;
		transEngine = NULL;
	}

	((void (__fastcall *)(DWORD, int))DeleteDX9Context)(dx9engine, 0);
}

void __fastcall HookCreateDX7Context(DWORD dx7engine, int)
{
	((void (__fastcall *)(DWORD, int))CreateDX7Context)(dx7engine, 0);
	OverWriteByte(SpriteContext, SpriteContextNeg);
}

void __fastcall HookDeleteDX7Context(DWORD dx7engine, int)
{
	OverWriteByte(SpriteContext, SpriteContextNeg);
	((void (__fastcall *)(DWORD, int))DeleteDX7Context)(dx7engine, 0);
}

void HookDrawHealthBar(int nSurface, int X, int Y, int W, int H, int SkinId, int dX, int dY)
{
	double tmp_float = 0.f;
	if(*(DWORD*)PlayerHealthMaxAddr != 0)
		tmp_float = ((double)(*(DWORD*)PlayerHealthAddr))/(*(DWORD*)PlayerHealthMaxAddr);

	char tmpBuff[8];
	sprintf(tmpBuff, "%d%%", static_cast<int32_t>(100*tmp_float));
	DrawSkin(nSurface, X, Y, W, H, SkinId, dX, dY);
	PrintText(nSurface, X+45, Y, 2, 180, 180, 180, tmpBuff, 1);
}

void HookDrawManaBar(int nSurface, int X, int Y, int W, int H, int SkinId, int dX, int dY)
{
	double tmp_float = 0.f;
	if(*(DWORD*)PlayerManaMaxAddr != 0)
		tmp_float = ((double)(*(DWORD*)PlayerManaAddr))/(*(DWORD*)PlayerManaMaxAddr);

	char tmpBuff[8];
	sprintf(tmpBuff, "%d%%", static_cast<int32_t>(100*tmp_float));
	DrawSkin(nSurface, X, Y, W, H, SkinId, dX, dY);
	PrintText(nSurface, X+45, Y, 2, 180, 180, 180, tmpBuff, 1);
}

#if (!defined(__CONFIG__) && defined(__MANABAR__)) || defined(__CONFIG__)
void __stdcall MyDrawHPBar(DWORD nSurface, DWORD X, DWORD Y, DWORD W, DWORD creaturePointer/*this should be height but we don't need it so we hack our way'*/, DWORD nRed, DWORD nGreen, DWORD nBlue)
{
	if(nRed == 0 && nGreen == 0 && nBlue == 0)//skip drawing black bar
		return;

	//Players should be >= 0x10000000 and < 0x40000000
	//Monsters should be >= 0x40000000 and < 0x80000000
	//NPCs should be >= 0x80000000
	//FIXME if I wrong
	DWORD creatureId = *(DWORD*)creaturePointer;
	if(creatureId >= 0x80000000)//npc won't have any bar
		return;

	uint32_t engineAddr = GetEngineAddr();
	uint32_t drawRectAddr = *(DWORD*)(*(DWORD*)(engineAddr)+0x14);
	((void (__fastcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))drawRectAddr)(engineAddr, 0, nSurface, X-1, Y-1, 27, 4, 0, 0, 0);
	((void (__fastcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))drawRectAddr)(engineAddr, 0, nSurface, X, Y, W, 2, nRed, nGreen, nBlue);
	if(creatureId == *(DWORD*)PlayerIDAddr
	#ifdef __CONFIG__
		&& should_draw_manabar
	#endif
	)
	{
		double tmp_float = 1.f;
		if(*(DWORD*)PlayerManaMaxAddr != 0)
			tmp_float = ((double)(*(DWORD*)PlayerManaAddr))/(*(DWORD*)PlayerManaMaxAddr);

		((void (__fastcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))drawRectAddr)(engineAddr, 0, nSurface, X-1, Y+4, 27, 4, 0, 0, 0);
		((void (__fastcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))drawRectAddr)(engineAddr, 0, nSurface, X, Y+5, static_cast<int32_t>(25*tmp_float), 2, 0, 108, 255);
	}
}
#endif

DWORD HPBarRenderHandle()
{
	return (DWORD)&newRenderer;
}

static HRESULT WINAPI Init( bool extended, bool transparent)
{
	DWORD dwOldProtect, dwNewProtect;
	HRESULT result = S_OK;
	HANDLE baseHandle = GetModuleHandle(NULL);
	client_BaseAddr = (DWORD)baseHandle;
	if(!baseHandle)
	{
		result = E_HANDLE;
		return result;
	}

	DWORD entryPoint = *(DWORD*)(client_BaseAddr+0x148); //entrypoint should be unique for every version
	if(entryPoint == 0x1625EB)
		client_Version = 860;
	else if(entryPoint == 0x15D02B)
		client_Version = 854;
	else
		client_Version = 0;

	switch(client_Version)
	{
		#ifdef __INCLUDE_854_VERSION__
		case 854:
		{
			VirtualProtect((LPVOID)(client_BaseAddr+0x1000), 0x22E000, PAGE_EXECUTE_READWRITE, &dwOldProtect); //Allow us to write in .text and .rdata sections

			transEngine = NULL;
			client_extended = extended;
			client_transparent = transparent;
			client_pointerTransPixels = (client_BaseAddr+0x240A20);
			GetFileName = (_GetFileName)(client_BaseAddr+0x103C60);
			GetEngineAddr = (_GetEngineAddr)(client_BaseAddr+0x11D6B0);
			DrawSkin = (_DrawSkin)(client_BaseAddr+0xB4DA0);
			PrintText = (_PrintText)(client_BaseAddr+0xB0550);
			HookCall((client_BaseAddr+0xA71B6), (DWORD)&HookPointers);
			HookCall((client_BaseAddr+0xA7149), (DWORD)&HookSygnature);
			HookCall((client_BaseAddr+0xA7159), (DWORD)&HookNumSprites);
			HookCall((client_BaseAddr+0xA9449), (DWORD)&HookLoadSprite);
			HookCall((client_BaseAddr+0x33AE4), (DWORD)&HookDrawHealthBar);
			HookCall((client_BaseAddr+0x33C96), (DWORD)&HookDrawManaBar);
			OverWrite((client_BaseAddr+0x1B2598), (DWORD)&getTimerTick);
			Nop((client_BaseAddr+0xA715E), 3);

			//Use DirectDraw7 interface instead of DirectDraw4
			//DEFINE_GUID( IID_IDirectDraw4,          0x9c59509a,0x39bd,0x11d1,0x8c,0x4a,0x00,0xc0,0x4f,0xd9,0x30,0xc5 );
			//DEFINE_GUID( IID_IDirectDraw7,          0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b );
			OverWrite((client_BaseAddr+0x1CFE00), 0x15E65EC0);
			OverWrite((client_BaseAddr+0x1CFE00+4), 0x11D23B9C);
			OverWrite((client_BaseAddr+0x1CFE00+8), 0x60002FB9);
			OverWrite((client_BaseAddr+0x1CFE00+12), 0x5BEA9797);
			OverWriteByte((client_BaseAddr+0x1BBA07), 0x37);

			#ifdef __MAGIC_EFFECTS_U16__
			HookCall((client_BaseAddr+0x10134), (client_BaseAddr+0xF4FF0));
			OverWriteByte((client_BaseAddr+0x1013A), 0xB7);
			#endif

			//if you want to use this remember it's limited to 0x7FFFFFFF because its handled as signed int32_t
			#ifdef __PLAYER_HEALTH_U32__
			HookCall((client_BaseAddr+0x119AB), (client_BaseAddr+0xF5190));
			OverWrite((client_BaseAddr+0x119B0), 0x8990F08B);
			HookCall((client_BaseAddr+0x119B6), (client_BaseAddr+0xF5190));
			OverWrite((client_BaseAddr+0x119BB), 0x8990F88B);
			#endif

			#ifdef __PLAYER_MANA_U32__
			HookCall((client_BaseAddr+0x119E9), (client_BaseAddr+0xF5190));
			OverWrite((client_BaseAddr+0x119EE), 0x8990D08B);
			HookCall((client_BaseAddr+0x119F4), (client_BaseAddr+0xF5190));
			OverWrite((client_BaseAddr+0x119F9), 0x89909090);
			#endif

			//Might be usefull if you want to use custom skills system
			#ifdef __PLAYER_SKILLS_U16__
			HookCall((client_BaseAddr+0x11C24), (client_BaseAddr+0xF4FF0));
			OverWriteByte((client_BaseAddr+0x11C2A), 0xB7);
			#endif

			PlayerHealthAddr = (client_BaseAddr+0x235F0C);
			PlayerHealthMaxAddr = (client_BaseAddr+0x235F08);
			PlayerManaAddr = (client_BaseAddr+0x235EF0);
			PlayerManaMaxAddr = (client_BaseAddr+0x235EEC);
			PlayerIDAddr = (client_BaseAddr+0x235F10);

			newRenderer = (Render_NEW *) calloc(1, sizeof(*newRenderer));
			if(!newRenderer)
			{
				result = E_OUTOFMEMORY;
				return result;
			}

			#if (!defined(__CONFIG__) && defined(__MANABAR__)) || defined(__CONFIG__)
			newRenderer->DrawRectangle = MyDrawHPBar;
			HookCall((client_BaseAddr+0xF07E5), (DWORD)&HPBarRenderHandle);
			HookCall((client_BaseAddr+0xF09E5), (DWORD)&HPBarRenderHandle);
			HookCall((client_BaseAddr+0xF08AE), (DWORD)&HPBarRenderHandle);
			HookCall((client_BaseAddr+0xF0A92), (DWORD)&HPBarRenderHandle);
			OverWriteWord((client_BaseAddr+0xF0948), 0xFFD0);//hack our way to creaturepointer
			OverWriteWord((client_BaseAddr+0xF0B20), 0xFFD0);//hack our way to creaturepointer
			#endif
			if(transparent)
			{
				newRenderer->LoadSprite = MyLoadSprite;

				CreateGlContext = (client_BaseAddr+0x134B60);
				DeleteGlContext = (client_BaseAddr+0x134900);
				CreateDX9Context = (client_BaseAddr+0x12CBC0);
				DeleteDX9Context = (client_BaseAddr+0x12C920);
				CreateDX7Context = (client_BaseAddr+0x124630);
				DeleteDX7Context = (client_BaseAddr+0x1243F0);
				SpriteContext = (client_BaseAddr+0xAAE06);
				SpriteContextPos = 0x57;
				SpriteContextNeg = 0x51;

				HookCall((client_BaseAddr+0xAADFA), (DWORD)&HookExtendedEngine);
				HookCall((client_BaseAddr+0x13AFBB), (DWORD)&HookCreateGLContext);
				HookCall((client_BaseAddr+0x13ACB6), (DWORD)&HookDeleteGLContext);
				HookCallN((client_BaseAddr+0x139558), (DWORD)&HookGLEnableAlpha);
				HookCallN((client_BaseAddr+0x139654), (DWORD)&HookGLDisableAlpha);
				OverWrite((client_BaseAddr+0x1C97D4), (DWORD)&HookCreateDX9Context);
				OverWrite((client_BaseAddr+0x1C97D0), (DWORD)&HookDeleteDX9Context);
				OverWrite((client_BaseAddr+0x1C93B4), (DWORD)&HookCreateDX7Context);
				OverWrite((client_BaseAddr+0x1C93B0), (DWORD)&HookDeleteDX7Context);
			}

			if(extended)
			{
				HookCall((client_BaseAddr+0xFBD57), (client_BaseAddr+0x1129E0));
				HookJMP((client_BaseAddr+0xFBD34), (client_BaseAddr+0x1B1F00));
				OverWrite((client_BaseAddr+0xFBD62), 0x908A0489);

				OverWrite((client_BaseAddr+0x1B1F00), 0xBD048D3E);
				OverWriteByte((client_BaseAddr+0x1B1F08), 0x50);
				HookCall((client_BaseAddr+0x1B1F09), (client_BaseAddr+0x16C3B7));
				HookJMP((client_BaseAddr+0x1B1F0E), (client_BaseAddr+0xFBD3D));

				OverWrite((client_BaseAddr+0xFBBFC), 0xFC8A448B);
				Nop((client_BaseAddr+0xFBBFC+4), 1);
			}

			VirtualProtect((LPVOID)(client_BaseAddr+0x1000), 0x22E000, dwOldProtect, &dwNewProtect); //Restore old sections protection
		}
		break;
		#endif

		#ifdef __INCLUDE_860_VERSION__
		case 860:
		{
			VirtualProtect((LPVOID)(client_BaseAddr+0x1000), 0x238000, PAGE_EXECUTE_READWRITE, &dwOldProtect); //Allow us to write in .text and .rdata sections

			transEngine = NULL;
			client_extended = extended;
			client_transparent = transparent;
			client_pointerTransPixels = (client_BaseAddr+0x24A9A8);
			GetFileName = (_GetFileName)(client_BaseAddr+0x108870);
			GetEngineAddr = (_GetEngineAddr)(client_BaseAddr+0x122A90);
			DrawSkin = (_DrawSkin)(client_BaseAddr+0xB96E0);
			PrintText = (_PrintText)(client_BaseAddr+0xB4DD0);
			HookCall((client_BaseAddr+0xABA36), (DWORD)&HookPointers);
			HookCall((client_BaseAddr+0xAB9C9), (DWORD)&HookSygnature);
			HookCall((client_BaseAddr+0xAB9D9), (DWORD)&HookNumSprites);
			HookCall((client_BaseAddr+0xADCC9), (DWORD)&HookLoadSprite);
			HookCall((client_BaseAddr+0x340C4), (DWORD)&HookDrawHealthBar);
			HookCall((client_BaseAddr+0x34276), (DWORD)&HookDrawManaBar);
			OverWrite((client_BaseAddr+0x1B85A0), (DWORD)&getTimerTick);
			Nop((client_BaseAddr+0xAB9DE), 3);

			OverWrite((client_BaseAddr+0xC7BD5), (DWORD)&prjInfo[0]);
			OverWrite((client_BaseAddr+0xBD9FB), 0xD2);//Info Window Width - 0xBE+20 to make more space for text

			//Use DirectDraw7 interface instead of DirectDraw4
			//DEFINE_GUID( IID_IDirectDraw4,          0x9c59509a,0x39bd,0x11d1,0x8c,0x4a,0x00,0xc0,0x4f,0xd9,0x30,0xc5 );
			//DEFINE_GUID( IID_IDirectDraw7,          0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b );
			OverWrite((client_BaseAddr+0x1D8840), 0x15E65EC0);
			OverWrite((client_BaseAddr+0x1D8840+4), 0x11D23B9C);
			OverWrite((client_BaseAddr+0x1D8840+8), 0x60002FB9);
			OverWrite((client_BaseAddr+0x1D8840+12), 0x5BEA9797);
			OverWriteByte((client_BaseAddr+0x1C1DB7), 0x37);

			#ifdef __MAGIC_EFFECTS_U16__
			HookCall((client_BaseAddr+0x104B4), (client_BaseAddr+0xF9C00));
			OverWriteByte((client_BaseAddr+0x104BA), 0xB7);
			#endif

			//if you want to use this remember it's limited to 0x7FFFFFFF because its handled as signed int32_t
			#ifdef __PLAYER_HEALTH_U32__
			HookCall((client_BaseAddr+0x11D2B), (client_BaseAddr+0xF9DA0));
			OverWrite((client_BaseAddr+0x11D30), 0x8990F08B);
			HookCall((client_BaseAddr+0x11D36), (client_BaseAddr+0xF9DA0));
			OverWrite((client_BaseAddr+0x11D3B), 0x8990F88B);
			#endif

			#ifdef __PLAYER_MANA_U32__
			HookCall((client_BaseAddr+0x11D69), (client_BaseAddr+0xF9DA0));
			OverWrite((client_BaseAddr+0x11D6E), 0x8990D08B);
			HookCall((client_BaseAddr+0x11D74), (client_BaseAddr+0xF9DA0));
			OverWrite((client_BaseAddr+0x11D79), 0x89909090);
			#endif

			//Might be usefull if you want to use custom skills system
			#ifdef __PLAYER_SKILLS_U16__
			HookCall((client_BaseAddr+0x11FA4), (client_BaseAddr+0xF9C00));
			OverWriteByte((client_BaseAddr+0x11FAA), 0xB7);
			#endif

			PlayerHealthAddr = (client_BaseAddr+0x23FE94);
			PlayerHealthMaxAddr = (client_BaseAddr+0x23FE90);
			PlayerManaAddr = (client_BaseAddr+0x23FE78);
			PlayerManaMaxAddr = (client_BaseAddr+0x23FE74);
			PlayerIDAddr = (client_BaseAddr+0x23FE98);

			newRenderer = (Render_NEW *) calloc(1, sizeof(*newRenderer));
			if(!newRenderer)
			{
				result = E_OUTOFMEMORY;
				return result;
			}

			#if (!defined(__CONFIG__) && defined(__MANABAR__)) || defined(__CONFIG__)
			newRenderer->DrawRectangle = MyDrawHPBar;
			HookCall((client_BaseAddr+0xF5675), (DWORD)&HPBarRenderHandle);
			HookCall((client_BaseAddr+0xF573E), (DWORD)&HPBarRenderHandle);
			HookCall((client_BaseAddr+0xF5875), (DWORD)&HPBarRenderHandle);
			HookCall((client_BaseAddr+0xF5922), (DWORD)&HPBarRenderHandle);
			OverWriteWord((client_BaseAddr+0xF57D8), 0xFFD0);//hack our way to creaturepointer
			OverWriteWord((client_BaseAddr+0xF5912), 0xFFD0);//hack our way to creaturepointer
			#endif
			if(transparent)
			{
				newRenderer->LoadSprite = MyLoadSprite;

				CreateGlContext = (client_BaseAddr+0x139F50);
				DeleteGlContext = (client_BaseAddr+0x139CF0);
				CreateDX9Context = (client_BaseAddr+0x131FD0);
				DeleteDX9Context = (client_BaseAddr+0x131D30);
				CreateDX7Context = (client_BaseAddr+0x129A10);
				DeleteDX7Context = (client_BaseAddr+0x1297D0);
				SpriteContext = (client_BaseAddr+0xAF686);
				SpriteContextPos = 0x57;
				SpriteContextNeg = 0x51;

				HookCall((client_BaseAddr+0xAF67A), (DWORD)&HookExtendedEngine);
				HookCall((client_BaseAddr+0x1403AB), (DWORD)&HookCreateGLContext);
				HookCall((client_BaseAddr+0x1400A6), (DWORD)&HookDeleteGLContext);
				HookCallN((client_BaseAddr+0x13E948), (DWORD)&HookGLEnableAlpha);
				HookCallN((client_BaseAddr+0x13EA44), (DWORD)&HookGLDisableAlpha);
				OverWrite((client_BaseAddr+0x1D2214), (DWORD)&HookCreateDX9Context);
				OverWrite((client_BaseAddr+0x1D2210), (DWORD)&HookDeleteDX9Context);
				OverWrite((client_BaseAddr+0x1D1DF4), (DWORD)&HookCreateDX7Context);
				OverWrite((client_BaseAddr+0x1D1DF0), (DWORD)&HookDeleteDX7Context);
			}

			if(extended)
			{
				HookCall((client_BaseAddr+0x100967), (client_BaseAddr+0x1175D0));
				HookJMP((client_BaseAddr+0x100944), (client_BaseAddr+0x1B7AA0));
				OverWrite((client_BaseAddr+0x100972), 0x908A0489);

				OverWrite((client_BaseAddr+0x1B7AA0), 0xBD048D3E);
				OverWriteByte((client_BaseAddr+0x1B7AA8), 0x50);
				HookCall((client_BaseAddr+0x1B7AA9), (client_BaseAddr+0x171A12));
				HookJMP((client_BaseAddr+0x1B7AAE), (client_BaseAddr+0x10094D));

				OverWrite((client_BaseAddr+0x10080C), 0xFC8A448B);
				Nop((client_BaseAddr+0x10080C+4), 1);
			}

			VirtualProtect((LPVOID)(client_BaseAddr+0x1000), 0x238000, dwOldProtect, &dwNewProtect); //Restore old sections protection
		}
		break;
		#endif

		default:
			result = E_INVALIDARG;
			break;
	}

	return result;
}

static int InitMain()
{
	char systemDirectory[MAX_PATH] = {};
	char systemDDrawDllPath[MAX_PATH] = {};
	GetSystemDirectory(systemDirectory, MAX_PATH);
	sprintf(systemDDrawDllPath, "%s\\ddraw.dll", systemDirectory);
	orig_ddraw = LoadLibrary(systemDDrawDllPath);
	if(!orig_ddraw)
	{
		MessageBox(NULL, "Cannot load system 'ddraw.dll'.", PROJECT_NAME, MB_OK|MB_ICONERROR);
		exit(-1);
	}

	#ifdef __CONFIG__
	loadConfig();

	HRESULT result = Init(should_use_extended, should_use_alpha);
	#else
	HRESULT result = Init(
	#ifdef __EXTENDED_FILE__
	true,
	#else
	false,
	#endif
	#ifdef __ALPHA_SPRITES__
	true
	#else
	false
	#endif
	);
	#endif
	if(result != S_OK)
	{
		if(result == E_OUTOFMEMORY)
		{
			MessageBox(NULL, "Failed to allocate renderer memory.", PROJECT_NAME, MB_OK|MB_ICONERROR);
			exit(-1);
		}
		else
		{
			MessageBox(NULL, "This version of client is unsupported.", PROJECT_NAME, MB_OK|MB_ICONERROR);
			exit(-1);
		}
	}

	return 1;
}

extern "C"
{
	__declspec(dllexport) HRESULT WINAPI DirectDrawCreate(void* lpGUID, void* lplpDD, void* pUnkOuter)
	{
		FARPROC ddcreate = GetProcAddress(orig_ddraw, "DirectDrawCreate");
		if(!ddcreate)
			return E_INVALIDARG;

		return ((HRESULT (WINAPI *)(void*, void*, void*))(DWORD)(ddcreate))(lpGUID, lplpDD, pUnkOuter);
	}
}

extern "C"
{
	BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
	{
		switch(dwReason)
		{
			case DLL_PROCESS_ATTACH:
				return InitMain();

			case DLL_THREAD_ATTACH:
			case DLL_THREAD_DETACH:
			case DLL_PROCESS_DETACH:
				break;
		}

		return 1;
	}
}

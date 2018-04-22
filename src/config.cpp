#include "config.h"

#ifdef __CONFIG__
extern bool should_use_hirestimer;
extern bool should_use_extended;
extern bool should_use_alpha;
extern bool should_use_cached_sprites;
extern bool should_draw_manabar;

bool checkBool(char *buffer)
{
	if(stricmp(buffer, "1") == 0 || stricmp(buffer, "yes") == 0 || stricmp(buffer, "true") == 0)
		return true;

	return false;
}

void loadConfig()
{
	should_use_hirestimer = false;
	should_use_extended = true;
	should_use_alpha = false;
	should_use_cached_sprites = true;
	should_draw_manabar = true;

	FILE* f = fopen("config.ini", "rb");
	if(!f)
		return;

	while(!feof(f))
	{
		char read_buffer[2048] = {0};
		char *buffer = fgets(read_buffer, sizeof read_buffer, f);
		if(!buffer)
			break;

		if(buffer[0] != '#' && buffer[0] != '-')
		{
			while(buffer[strlen(buffer)-1] == '\n' || buffer[strlen(buffer)-1] == '\r')
				buffer[strlen(buffer)-1] = '\0';

			char* pch = NULL;
			if(pch = strstr(buffer, "="))
			{
				buffer[pch-buffer-1] = '\0';
				if(stricmp(buffer, "hirestimer") == 0)
					should_use_hirestimer = checkBool(pch+2);
				else if(stricmp(buffer, "extended") == 0)
					should_use_extended = checkBool(pch+2);
				else if(stricmp(buffer, "alpha") == 0)
					should_use_alpha = checkBool(pch+2);
				else if(stricmp(buffer, "cachesprites") == 0)
					should_use_cached_sprites = checkBool(pch+2);
				else if(stricmp(buffer, "drawmanabar") == 0)
					should_draw_manabar = checkBool(pch+2);
			}
		}
	}

	if(f)
		fclose(f);
}
#endif

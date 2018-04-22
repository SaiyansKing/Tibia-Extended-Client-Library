This code is distributed "as-is" without any license in the hope that it will be useful.\
Code was writen for studying purpose and I don't take any responsibility for how it is used.

# Current Features
* Extended client files(exceeds the 65535 sprite limit)
* Alpha channel .spr file(allows to use transparency(currently only work in DX9 and OGL)
* Show health/mana percentage in client
* Fix some weird problem with timeGetTime on windows 10+ryzen cpu
* Manabar drawing below player
* Exceeds the limit of 255 magic effects
* Exceeds the limit of 65535 max health display
* Exceeds the limit of 65535 max mana display
* Exceeds the limit of 255 skills display

# Building
Create a dynamic link library project, name it and save.\
Make sure your target filename is ddraw.dll\
Link opengl32 to your project.\
Always compile as release 32bit.

### Preprocesor Defines
**-D__INCLUDE_854_VERSION__**\
inludes 8.54 client version target\
**-D__INCLUDE_860_VERSION__**\
includes 8.60 client version target\
**-D__CONFIG__**\
allows to customize extended options via config.ini\
**-D__MAGIC_EFFECTS_U16__**\
changes the magic effects game protocol usage of uint8_t to uint16_t\
**-D__PLAYER_HEALTH_U32__**\
changes the player health game protocol usage of uint16_t to int32_t - 0x7FFFFFFF limit\
**-D__PLAYER_MANA_U32__**\
changes the player mana game protocol usage of uint16_t to int32_t - 0x7FFFFFFF limit\
**-D__PLAYER_SKILLS_U16__**\
changes the player skills value game protocol usage of uint8_t to uint16_t - can be used custom skills system\
**-D__EXTENDED_FILE__**\
use the extended .spr and .dat files(only if not defined **-D__CONFIG__**)\
**-D__ALPHA_SPRITES__**\
use the alpha channel in .spr file(only if not defined **-D__CONFIG__**)\
**-D__MANABAR__**\
force the manabar to be visible(only if not defined **-D__CONFIG__**)
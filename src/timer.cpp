#include "timer.h"

#ifdef __CONFIG__
extern bool should_use_hirestimer;
#endif

int64_t started_ticks = 0;
bool ticks_started = false;

#ifdef __CONFIG__
bool hires_timer_available = false;
LARGE_INTEGER hires_start_ticks;
LARGE_INTEGER hires_ticks_per_second;
#endif

void setupTimer()
{
	#ifdef __CONFIG__
	if(should_use_hirestimer && QueryPerformanceFrequency(&hires_ticks_per_second) == TRUE)
	{
		hires_timer_available = true;
		QueryPerformanceCounter(&hires_start_ticks);
	}
	else
	{
		hires_timer_available = false;
		started_ticks = GetTickCount();//there's some problem with timeGetTime on windows10 + ryzen CPU(and maybe on other platforms too) so let's fix it
	}
	#else
	started_ticks = GetTickCount();//there's some problem with timeGetTime on windows10 + ryzen CPU(and maybe on other platforms too) so let's fix it
	#endif

	ticks_started = true;
}

DWORD getTimerTick()
{
	int64_t now = 0;
	LARGE_INTEGER hires_now;
	if(!ticks_started)
		setupTimer();

	#ifdef __CONFIG__
	if(hires_timer_available)
	{
		QueryPerformanceCounter(&hires_now);
		hires_now.QuadPart -= hires_start_ticks.QuadPart;
		hires_now.QuadPart *= 1000;
		hires_now.QuadPart /= hires_ticks_per_second.QuadPart;
		return static_cast<DWORD>(hires_now.QuadPart);
	}
	#endif

	return static_cast<DWORD>(GetTickCount() - started_ticks);
}

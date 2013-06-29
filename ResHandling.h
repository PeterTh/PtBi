#pragma once

#include "stdafx.h"
#include "Windows.h"

static void setClosestMode(int w, int h, int hz) {
	int mode = -1;
	DEVMODE current, found;
	current.dmSize = sizeof(DEVMODE);
	for(int i = 0; ; ++i) {
		if(!EnumDisplaySettings(NULL, i, &current)) break;
		if(current.dmBitsPerPel != 32) continue;
		//printf("%5d x%5d @%4d, %2dbpp\n", current.dmPelsWidth, current.dmPelsHeight, current.dmDisplayFrequency, current.dmBitsPerPel);
		//printf("%d - %d - %d\n", current.dmPelsWidth == w , current.dmPelsHeight == h , current.dmDisplayFrequency%hz);
		if(current.dmPelsWidth == w && current.dmPelsHeight == h && current.dmDisplayFrequency%hz == 0) {
			printf("Found display mode:\n%5d x%5d @%4dHz, %2dbpp, %x\n",
				current.dmPelsWidth,
				current.dmPelsHeight,
				current.dmDisplayFrequency,
				current.dmBitsPerPel,
				current.dmDisplayFlags);
			mode = i;
			found = current;
			//break;
		}
	}
	if(mode != -1) {
		ChangeDisplaySettings(&found, CDS_RESET | CDS_FULLSCREEN);
	} else {
		WARN("No suitable display mode found.");
	}
}

static void setClosestHz(int hz) {
	DEVMODE current;
	RT_ASSERT(
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &current) == TRUE, 
		"Could not determine current display mode.");
	setClosestMode(current.dmPelsWidth, current.dmPelsHeight, hz);
}


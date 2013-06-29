// PtBi.cpp : Defines the entry point for the console application.
//

#include <cstring>

#include "stdafx.h"
#include "PTOpenGL.h"
#include "Audio.h"
#include "Capture.h"

class KeepDisplayOn {
    UINT prevScreenSaver;
    EXECUTION_STATE prevExecState;
public:
    KeepDisplayOn() {
        prevExecState = SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
        SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &prevScreenSaver, 0);
        SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, FALSE, NULL, 0);
    }

    ~KeepDisplayOn() {
        SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, prevScreenSaver, NULL, 0);
        SetThreadExecutionState(prevExecState);
    }
};

#ifdef WIN32
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

int main(int argc, char* argv[])
{
	// redirect stdout
	freopen("ptbi.log", "w+", stdout);

	RT_ASSERT(SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS) == TRUE, "Could not set high process priority.");

	RT_ASSERT(CoInitializeEx(NULL, COINIT_MULTITHREADED) == S_OK, "Failed to initialize COM in MT mode.");

	// Disable screensaver & sleep (uses RAII)
	KeepDisplayOn on;

	// parse arguments
	_BMDDisplayMode mode = bmdModeHD720p5994;
	bool disableAudio = false;
	int w = 1280, h = 720, hz = 59;
	for(int i=1; i<argc; ++i) {
		if(strstr(argv[i], "-mode=") == argv[1]) {
			if(strstr(argv[i], "720p60")) {
				mode = bmdModeHD720p60;
				w = 1280; h = 720; hz = 60;
			}
			else if(strstr(argv[i], "720p5994")) {
				mode = bmdModeHD720p5994;
				w = 1280; h = 720; hz = 59;
			}
			else if(strstr(argv[i], "720p50")) {
				mode = bmdModeHD720p50;
				w = 1280; h = 720; hz = 50;
			}
			else if(strstr(argv[i], "1080p60")) {
				mode = bmdModeHD1080p6000;
				w = 1920; h = 1080; hz = 60;
			}
			else if(strstr(argv[i], "1080p5994")) {
				mode = bmdModeHD1080p5994;
				w = 1920; h = 1080; hz = 59;
			}
			else if(strstr(argv[i], "1080p50")) {
				mode = bmdModeHD1080p50;
				w = 1920; h = 1080; hz = 50;
			}
			else if(strstr(argv[i], "1080p30")) {
				mode = bmdModeHD1080p30;
				w = 1920; h = 1080; hz = 30;
			}
			else if(strstr(argv[i], "1080p25")) {
				mode = bmdModeHD1080p25;
				w = 1920; h = 1080; hz = 25;
			}
			else if(strstr(argv[i], "PAL")) {
				mode = bmdModePALp;
				w = 720; h = 576; hz = 50;
			}
			else if(strstr(argv[i], "NTSC")) {
				mode = bmdModeNTSCp;
				w = 720; h = 480; hz = 59;
			}
			else RT_ASSERT(false, "Unknown capture mode.");
		} else if(strstr(argv[i], "-disable-audio") == argv[1]) {
			disableAudio = true;
		} else {
			RT_ASSERT(false, "Unknown argument: " << argv[i]);
		}
	}

	DeckLinkCapture capturer(mode, disableAudio);
	{ // block to make sure presenter is destroyed before capturer
		AudioRenderer renderer(capturer, 2);
		GLPresenter presenter(capturer, w, h, hz);

		capturer.start();
		while(presenter.run());
	}
	
	wcout << "Final Shutdown.\n";
	CoUninitialize();
	return 0;
}


#pragma once

#include "stdafx.h"

#include "DeckLinkAPI_h.h"

#include "Timer.h"

struct FrameListener {
	virtual void frameRecieved(void* data) = 0;
};
struct AudioListener {
	virtual void packetRecieved(long samples, void* data) = 0;
};

enum class CapturePixelFormat {
	YUV, ARGB8, BGRA8
};

class DeckLinkCapture : public IDeckLinkInputCallback {
	static const unsigned maxBuffers = 3;

	IDeckLinkIterator* pDeckLinkIterator;
	IDeckLink* pDeckLink;
	BSTR modelName;
	CapturePixelFormat pf;
	IDeckLinkInput* pDeckLinkInput;
	bool streaming, disableAudio;
	FrameListener *displayListener, *consumeListener;
	AudioListener *audioListener;
	Timer timer;

	void safeRelease() {
		if(streaming == true) stop();
		if(pDeckLinkInput) pDeckLinkInput->Release(); 
		if(pDeckLink) pDeckLink->Release();
		if(pDeckLinkIterator) pDeckLinkIterator->Release();
	}

	const char* getModeString(_BMDDisplayMode mode) {
		switch(mode) {
			case bmdModeHD720p50: return "720p 50Hz";
			case bmdModeHD720p5994: return "720p 59.94Hz";
			case bmdModeHD720p60: return "720p 60Hz";
			case bmdModeHD1080p6000: return "1080p 60Hz";
			case bmdModeHD1080p5994: return "1080p 59.94Hz";
			case bmdModeHD1080p50: return "1080p 50Hz";
			case bmdModeHD1080p30: return "1080p 30Hz";
			case bmdModeHD1080p25: return "1080p 25Hz";
			case bmdModePALp: return "PAL progressive (720x576@50Hz)";
			case bmdModeNTSCp: return "NTSC progressive (720x480@59.94Hz)";
			default: return "UNKNOWN MODE";
		}
	}

	const char* getPixelFormatString(_BMDPixelFormat pf) {
		switch(pf) {
		case bmdFormat8BitYUV: return "8 bit YUV";
		case bmdFormat10BitYUV: return "10 bit YUV";
		case bmdFormat8BitARGB: return "8 bit ARGB";
		case bmdFormat8BitBGRA: return "8 bit BGRA";
		case bmdFormat10BitRGB: return "10 bit RGB";
		default: return "UNKNOWN PIXEL FORMAT";
		}
	}

public:
	DeckLinkCapture(_BMDDisplayMode mode, _BMDPixelFormat format, bool disableAudio) : pDeckLinkIterator(NULL), pDeckLink(NULL), pDeckLinkInput(NULL),
		displayListener(NULL), consumeListener(NULL), audioListener(NULL), streaming(false), disableAudio(disableAudio) {
		try {
			CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&pDeckLinkIterator);
			RT_ASSERT(pDeckLinkIterator != NULL, "Failed to create DeckLinkIterator instance.\nTry updating your Blackmagic software.");
			RT_ASSERT((pDeckLinkIterator->Next(&pDeckLink) == S_OK) && (pDeckLink != NULL), "Failed to get DeckLink instance.");
			RT_ASSERT(pDeckLink->GetModelName(&modelName) == S_OK, "Failed to get model name.");
			wcout << "Found " << (wchar_t*)modelName << " capture hardware.\n";
			RT_ASSERT((pDeckLink->QueryInterface(IID_IDeckLinkInput, (void**)&pDeckLinkInput) == S_OK) && (pDeckLinkInput != NULL),
				"Failed to get DeckLink video input interface.");

			BMDDisplayModeSupport support = bmdDisplayModeNotSupported;
			RT_ASSERT(pDeckLinkInput->DoesSupportVideoMode(mode, format, bmdVideoInputFlagDefault, &support, NULL)
				== S_OK, "Failed querying display mode support.");
			RT_ASSERT(support != bmdDisplayModeNotSupported, "Requested display mode not supported.");
			wcout << "Display mode " << getModeString(mode) << ", " << getPixelFormatString(format) << " supported."
			      << ((support == bmdDisplayModeSupportedWithConversion) ? " (with conversion)" : "") << ".\n";
			RT_ASSERT(pDeckLinkInput->EnableVideoInput(mode, format, bmdVideoInputFlagDefault) == S_OK,
				"Failed to enable video input.");

			switch(format) {
			case bmdFormat8BitARGB: pf = CapturePixelFormat::ARGB8; break;
			case bmdFormat8BitBGRA: pf = CapturePixelFormat::BGRA8; break;
			case bmdFormat8BitYUV: pf = CapturePixelFormat::YUV; break;
			default: RT_ASSERT(false, "Unsupported pixel format");
			}

			if(!disableAudio) {
				RT_ASSERT(pDeckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2) == S_OK,
					"Failed to enable audio input.");
				wcout << "Enabled Audio, 16 bit @ 48KHz\n";
			}

			RT_ASSERT(pDeckLinkInput->SetCallback(this) == S_OK, "Failed to register capturing callback function.");
		}
		catch(RTAssertException rtae) {
			safeRelease();
			throw new runtime_error("Error during DeckLinkCapture initialization.");
		}
	}

	virtual HRESULT STDMETHODCALLTYPE IDeckLinkInputCallback::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode *, BMDDetectedVideoInputFormatFlags) {
		WARN("Input Format changed! Not implemented.");
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame *videoFrame, IDeckLinkAudioInputPacket *audioPacket) {
		NONRELEASE(
			static unsigned long numframes = 0;
			if(++numframes%600 == 0) {
				printf("VideoInputFrameArrived #% 10d,        time: % 10.2lf\n", numframes, timer.elapsed()/1000.0);
			} )
		
		// increase capturing/display thread priority
		//if(GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_TIME_CRITICAL) {
		//	RT_ASSERT(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) == TRUE, 
		//		"Could not set capturing thread priority.");
		//}

		// skip frames if newer ones available
		unsigned long framecount = 0;
		pDeckLinkInput->GetAvailableVideoFrameCount(&framecount);
		if(framecount>0) {
			NONRELEASE(WARN("Skipping frame, queued frames: " << framecount));
			return S_OK;
		}
		
		if(videoFrame) {
			//printf("%ld x %ld, bwidth: %ld\n", videoFrame->GetWidth(), videoFrame->GetHeight(), videoFrame->GetRowBytes());
			void *buffer;
			RT_ASSERT(videoFrame->GetBytes(&buffer) == S_OK, "Error getting frame buffer pointer.");
			if(displayListener) displayListener->frameRecieved(buffer);
			if(consumeListener) consumeListener->frameRecieved(buffer);
		}

		if(!disableAudio) {
			// audio
			long frameCount = audioPacket->GetSampleFrameCount();
			void* audioBuffer;
			RT_ASSERT(audioPacket->GetBytes(&audioBuffer) == S_OK, "Error getting audio data buffer.");
			if(audioListener) {
				audioListener->packetRecieved(frameCount, audioBuffer);
			}
		}

		NONRELEASE(
			if(numframes%600 == 0) printf("VideoInputFrameArr end #% 10d,        time: % 10.2lf\n", numframes, timer.elapsed()/1000.0); )
		return S_OK;
	}

	void start() {
		streaming = true;
		RT_ASSERT(pDeckLinkInput->StartStreams() == S_OK, "Error starting capture stream.");
	}
	
	void stop() {
		if(streaming) {
			streaming = false;
			RT_ASSERT(pDeckLinkInput->StopStreams() == S_OK, "Error stopping capture stream.");
			RT_ASSERT(pDeckLinkInput->DisableVideoInput() == S_OK, "Error disabling video input.");
		}
	}

	void registerDisplayListener(FrameListener *listen) {
		displayListener = listen;
	}
	void removeDisplayListener() { displayListener = NULL; }
	void registerConsumeListener(FrameListener *listen) {
		consumeListener = listen;
	}
	void removeConsumeListener() { displayListener = NULL; }
	void registerAudioListener(AudioListener *listen) {
		audioListener = listen;
	}
	void removeAudioListener() { audioListener = NULL; }

	// No, this is not how it should be done.
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { RT_ASSERT(0, "Fatal."); return S_OK; };
	virtual ULONG STDMETHODCALLTYPE AddRef(void) { return 1L; };
	virtual ULONG STDMETHODCALLTYPE Release(void) { return 0L; };

	~DeckLinkCapture() {
		stop();
		Sleep(100);
		safeRelease();
	}

	int getBytesPerPixel() {
		return pf == CapturePixelFormat::YUV ? 2 : 4;
	}

	bool isYUV() {
		return pf == CapturePixelFormat::YUV;
	}

	CapturePixelFormat getPixelFormat() {
		return pf;
	}

};

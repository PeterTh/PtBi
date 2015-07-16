#pragma once

#include <bass.h>
#include <cstdint>

extern "C" {
#include <dca.h>
#include <a52.h>
};

#include "stdafx.h"
#include "Capture.h"
#include "Console.h"


class AudioRenderer : public AudioListener {
	HSTREAM bStream, dtsStream, exStream;
	bool playing, extendStereo, muted, enableDTS, enableDD;
	unsigned channels;
	float volume;
	uint16_t boostAudio;

	dca_state_t *dcaState;
	a52_state_t *a52State;

	StaticTextPtr audioReportText;

	static AudioRenderer *singleton;

public:
	AudioRenderer(DeckLinkCapture &capture, unsigned channels);
	~AudioRenderer() {
		BASS_Free();

		dca_free(dcaState);
		a52_free(a52State);
	}

	static AudioRenderer& get() {
		RT_ASSERT(singleton != NULL, "Requested AudioRenderer before initialization");
		return *singleton;
	}

	void bufferUnderrunProtection(HSTREAM stream, unsigned streamChannels);

	void packetRecieved(long samples, void* data);

	void decodeDTSFrame();
	bool tryDTS(long samples, void* data);
	
	void decodeDDFrame();
	bool tryDD(long samples, void* data);

	void toggleExpandStereo() {
		extendStereo = !extendStereo;
		Console::get().add(format("Stereo -> Surround sound expansion: %s", extendStereo ? "enabled" : "disabled"));
	}
	void toggleMuted() {
		muted = !muted;
		Console::get().add(format("Sound %s", muted ? "disabled" : "enabled"));
		BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, muted ? 0 : (DWORD)(volume*10000));
	}
	void toggleDTS() {
		enableDTS = !enableDTS;
		Console::get().add(format("DTS decoding %s", enableDTS ? "enabled" : "disabled"));
	}
	void toggleDD() {
		enableDD = !enableDD;
		Console::get().add(format("DD decoding %s", enableDD ? "enabled" : "disabled"));
	}
	void adjustVolume(double factor) {
		volume += (float)factor;
		if(volume < 0.0f) volume = 0.0f;
		if(volume > 1.0f) volume = 1.0f;
		Console::get().add(format("Volume: %d%", (int)(volume*100)));
		BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, (DWORD)(volume*10000));
		muted = false;
	}
	void adjustBoost(uint16_t factor) {
		boostAudio += factor;
		if(boostAudio < 1) boostAudio = 1;
		Console::get().add(format("Boosting audio levels by factor: %d -- may cause clipping", boostAudio));
	}
};

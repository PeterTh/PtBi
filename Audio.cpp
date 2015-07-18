#include "Audio.h"

#define AC3_MAX_CODED_FRAME_SIZE 3840 /* in bytes */
#define PRE_DECODE_BUFFER_SIZE (AC3_MAX_CODED_FRAME_SIZE*4) /* in bytes */

#define AUDIO_HZ 48000

#define AUDIO_BUF_UNDERRUN_LIMIT 6000
#define AUDIO_BUF_UNDERRUN_LIMIT_F 250
#define AUDIO_BUF_OVERRUN_LIMIT 7000


AudioRenderer* AudioRenderer::singleton = NULL;

AudioRenderer::AudioRenderer(DeckLinkCapture &capture, unsigned channels) : 
	playing(false), extendStereo(false), muted(false), enableDTS(false),
	enableDD(false), channels(channels), volume(0.75f), boostAudio(1),
	dcaState(NULL), a52State(NULL)
{
	// BASS init
	BASS_SetConfig(BASS_CONFIG_BUFFER, 1000);
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
	BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);
	RT_ASSERT(BASS_Init(-1, AUDIO_HZ, /*flags*/0, /*window*/0, /*device*/NULL) == TRUE, "Failed to initialize BASS audio library");
	bStream = BASS_StreamCreate(AUDIO_HZ, channels, /*flags*/0, STREAMPROC_PUSH, NULL);
	RT_ASSERT(bStream != 0, "Failed to initialize basic audio stream.");
	BASS_ChannelSetAttribute(bStream, BASS_ATTRIB_NOBUFFER, 1);
	exStream = BASS_StreamCreate(AUDIO_HZ, 4, /*flags*/0, STREAMPROC_PUSH, NULL);
	RT_ASSERT(exStream != 0, "Failed to initialize expanded audio stream.");
	dtsStream = BASS_StreamCreate(AUDIO_HZ, 6, BASS_SAMPLE_FLOAT, STREAMPROC_PUSH, NULL);
	RT_ASSERT(dtsStream != 0, "Failed to initialize 5.1 stream.");
	adjustVolume(0.0);
	// start channels
	RT_ASSERT(BASS_ChannelPlay(bStream, TRUE) == TRUE, "Failed to start BASS bStream");
	RT_ASSERT(BASS_ChannelPlay(exStream, TRUE) == TRUE, "Failed to start BASS exStream");
	RT_ASSERT(BASS_ChannelPlay(dtsStream, TRUE) == TRUE, "Failed to start BASS dtsStream");
	// DCA init
	dcaState = dca_init(0);
	// DD init
	a52State = a52_init(0);

	audioReportText = std::make_shared<StaticText>("", 25.0f,300.0f);
	Console::get().add(audioReportText);

	singleton = this;
	capture.registerAudioListener(this);
}

AudioRenderer::~AudioRenderer()
{
	BASS_Free();

	dca_free(dcaState);
	a52_free(a52State);
}

void AudioRenderer::bufferUnderrunProtection(HSTREAM stream, unsigned streamChannels)
{
	unsigned avail = BASS_ChannelGetData(stream, NULL, BASS_DATA_AVAILABLE);
	audioReportText->text = format("Audio data buffered: %8u Byte \nAudio buffer time:   %8u ms", 
									avail, (avail*1000)/(streamChannels*2*AUDIO_HZ)).c_str();
	while(avail < AUDIO_BUF_UNDERRUN_LIMIT * streamChannels) { // add silence if running out of buffer
		cout << "---------- " << timeString() << endl;
		cout << "AUDIO BUFFER UNDERRUN: " << avail << " < " << "(" << AUDIO_BUF_UNDERRUN_LIMIT << " * " << streamChannels << ")\n";
		static void *fillup = NULL;
		if(fillup == NULL) {
			fillup = malloc(AUDIO_BUF_UNDERRUN_LIMIT_F*streamChannels);
			memset(fillup, 0, AUDIO_BUF_UNDERRUN_LIMIT_F*streamChannels);
		}
		BASS_StreamPutData(stream, fillup, AUDIO_BUF_UNDERRUN_LIMIT_F*streamChannels);
		avail = BASS_ChannelGetData(stream, NULL, BASS_DATA_AVAILABLE);
		cout << "         NOW BUFFERED: " << avail << endl;
	}
}

void AudioRenderer::packetRecieved(long samples, void* data)
{
	if(enableDD && tryDD(samples, data)) {
		bufferUnderrunProtection(dtsStream, 6 * 2 /* float */);
		return;
	}
	if(enableDTS && tryDTS(samples, data)) {
		bufferUnderrunProtection(dtsStream, 6 * 2 /* float */);
		return;
	}
	if(boostAudio > 1) {
		uint16_t *dbuffer = (uint16_t*)data;
		for(long i=0; i<samples*(long)channels; i++) {
			dbuffer[i] = dbuffer[i]*boostAudio;
		}
	}
	if(extendStereo) {
		static const size_t channels = 4;
		static uint16_t exBuffer[800*2*channels*2];
		uint16_t *dbuffer = (uint16_t*)data;
		for(long i=0; i<samples; ++i) {
			exBuffer[4*i+0] = dbuffer[2*i+0];
			exBuffer[4*i+1] = dbuffer[2*i+1];
			exBuffer[4*i+2] = dbuffer[2*i+0];
			exBuffer[4*i+3] = dbuffer[2*i+1];
		}
		bufferUnderrunProtection(exStream, channels);
		unsigned avail = BASS_ChannelGetData(exStream, NULL, BASS_DATA_AVAILABLE);
		if(avail > AUDIO_BUF_OVERRUN_LIMIT * channels) { // skip if buffering too much
			samples = (long)(samples*0.75);
			samples += samples%(2*channels);
		}
		RT_ASSERT(BASS_StreamPutData(exStream, exBuffer, samples * 2 * channels) != -1, "Failed to forward expanded audio to BASS");
	} 
	else 
	{
		bufferUnderrunProtection(bStream, channels);
		unsigned avail = BASS_ChannelGetData(bStream, NULL, BASS_DATA_AVAILABLE);
		if(avail > AUDIO_BUF_OVERRUN_LIMIT * channels) { // skip if buffering too much
			cout << "---------- " << timeString() << endl;
			cout << "AUDIO BUFFER OVERRUN: " << avail << " > " << "(AUDIO_BUF_OVERRUN_LIMIT * " << channels << ")\n";
			samples = (long)(samples*0.75);
			samples -= samples%(2*channels);
		}
		if(BASS_StreamPutData(bStream, data, samples * /*16 bit*/ 2 * channels) == -1) {
			int errcode = BASS_ErrorGetCode();
			switch(errcode) {
			case BASS_ERROR_HANDLE: cout << "Failed to push audio data to stream: BASS_ERROR_HANDLE\n";
			case BASS_ERROR_NOTAVAIL: cout << "Failed to push audio data to stream: BASS_ERROR_NOTAVAIL\n"; 
			case BASS_ERROR_ILLPARAM: cout << "Failed to push audio data to stream: BASS_ERROR_ILLPARAM\n";
			case BASS_ERROR_ENDED: cout << "Failed to push audio data to stream: BASS_ERROR_ENDED\n";
			case BASS_ERROR_MEM: cout << "Failed to push audio data to stream: BASS_ERROR_MEM.\n";
			}
		}
	}
	BASS_Update(100);
}

void AudioRenderer::decodeDTSFrame()
{
	const static size_t BBUFFER_LENGTH = 256 * /*channels*/ 6;
	static float bassBuffer[BBUFFER_LENGTH];
	// decode all blocks in this DTS frame
	for(int b=0; b < dca_blocks_num(dcaState); ++b) {
		dca_block(dcaState);
		// returns a pointer to an internal buffer which will contain 256 samples for the first channel, 
		// followed by 256 samples for the second channel, etc...
		// the channel order is center, left, right, left surround, right surround, LFE
		float* samples = dca_samples(dcaState);
		// BASS stream order: left-front, right-front, center, LFE, left-rear/side, right-rear/side
		for(int k=0; k<256; ++k) {
			bassBuffer[k*6+0] = samples[k+1*256]*boostAudio; // left front
			bassBuffer[k*6+1] = samples[k+2*256]*boostAudio; // right front
			bassBuffer[k*6+2] = samples[k+0*256]*boostAudio; // center
			bassBuffer[k*6+3] = samples[k+5*256]*boostAudio; // LFE
			bassBuffer[k*6+4] = samples[k+3*256]*boostAudio; // left rear
			bassBuffer[k*6+5] = samples[k+4*256]*boostAudio; // right rear
		}

		unsigned avail = BASS_ChannelGetData(dtsStream, NULL, BASS_DATA_AVAILABLE);
		if(avail > AUDIO_BUF_OVERRUN_LIMIT * /*channels*/ 6) { // skip if buffering too much
			cout << "---------- " << timeString() << endl;
			cout << "AUDIO BUFFER OVERRUN: " << avail << " > " << "(AUDIO_BUF_OVERRUN_LIMIT * " << channels << ")\n";
		} else {
			RT_ASSERT(BASS_StreamPutData(dtsStream, bassBuffer, BBUFFER_LENGTH * sizeof(float)) != -1, "Failed to forward DTS audio to BASS");
		}
	}
}

bool AudioRenderer::tryDTS(long samples, void* data)
{
	static uint8_t remData[32000];
	static int remBytes = 0, getBytes = 0;
	// determined experimentally, this is the difference between the reported getBytes value 
	// and the actual in-memory distance between DTS frames. No idea why
	const int MAGIC_DTS_OFFSET = 17; 

	uint8_t* udata = (uint8_t*)data;
	float level = 0.8f;
	int myFlags = DCA_3F2R | DCA_LFE;
	int pos = 0;
	int flags, sampleRate, bitRate, frameRate;

	// search packet start if remBytes not set
	if(remBytes == 0) {
		bool found = false;
		for(pos=0; (pos<3000) && !found; pos+=2) {
			// look ahead 2 bytes
			getBytes = dca_syncinfo(dcaState, udata+pos+2, &flags, &sampleRate, &bitRate, &frameRate);
			if(getBytes != 0) {
				found = true;
				printf("# New DTS data stream found at pos %d.\n", pos+2);
			}
		}
		if(!found) { // not a DTS stream
			return false;
		}
	} 
	else // remBytes was set, missing bytes from last invocation, handle those
	{ 
		pos = getBytes;
		NONRELEASE(printf("Handling %d remaining bytes, have %d.\n", getBytes, remBytes));
		RT_ASSERT(remBytes + getBytes < 32000, "Error in DTS decoding, buffer overflow.");
		memcpy(remData + remBytes, data, getBytes);
		getBytes = dca_syncinfo(dcaState, remData, &flags, &sampleRate, &bitRate, &frameRate);
		if(getBytes == 0) { // stream error/end, retry later
			NONRELEASE(printf("DTS broken at remainder.\n"));
			remBytes = 0;
			return false;
		}
		int ret = dca_frame(dcaState, remData, &myFlags, &level, 0.0f);
		RT_ASSERT((DCA_3F2R | DCA_LFE) == myFlags, "DTS audio decode failed. Not 5.1 format?");
		decodeDTSFrame();
	}

	// handle main data chunk
	while(true) { 
		NONRELEASE(printf("Main chunk handling at %d.\n", pos));
		// check if remainder enough to contain header, if not then wait until next frame
		if(samples*4 - pos < MAGIC_DTS_OFFSET) {
			// copy remainder to remData
			remBytes = samples*4 - pos;
			getBytes -= remBytes - MAGIC_DTS_OFFSET;
			memcpy(remData, udata+pos, remBytes);
			return true;
		}
		getBytes = dca_syncinfo(dcaState, udata+pos, &flags, &sampleRate, &bitRate, &frameRate);
		if(getBytes == 0) { // stream error/end, retry later
			NONRELEASE(printf("DTS broken at main chunk, pos %d.\n", pos));
			remBytes = 0;
			return false;
		}
		if(samples*4 - pos > getBytes + MAGIC_DTS_OFFSET) {
			// enough buffered data to work with
			int ret = dca_frame(dcaState, udata+pos, &myFlags, &level, 0.0f);
			RT_ASSERT((DCA_3F2R | DCA_LFE) == myFlags, "DTS audio decode failed. Not 5.1 format?");
			decodeDTSFrame();
			pos += getBytes + MAGIC_DTS_OFFSET;
		} else {
			NONRELEASE(printf("DTS out of buffer at main chunk.\n"));
			// copy remainder to remData
			remBytes = samples*4 - pos;
			getBytes -= remBytes - MAGIC_DTS_OFFSET;
			memcpy(remData, udata+pos, remBytes);
			return true;
		}
	}
	return false;
}

void AudioRenderer::decodeDDFrame()
{
	const static size_t BBUFFER_LENGTH = 256 * /*channels*/ 6;
	static float bassBuffer[BBUFFER_LENGTH];
	// decode all blocks in this DD frame (always 6)
	for(int b=0; b < 6; ++b) {
		a52_block(a52State);
		// returns a pointer to an internal buffer which will contain 256 samples
		// the channel order is LFE, left, center, right, left surround, right surround
		float* samples = a52_samples(a52State);
		// BASS stream order: left-front, right-front, center, LFE, left-rear/side, right-rear/side
		for(int k=0; k<256; ++k) {
			bassBuffer[k*6+0] = samples[k+1*256]*boostAudio; // left front
			bassBuffer[k*6+1] = samples[k+3*256]*boostAudio; // right front
			bassBuffer[k*6+2] = samples[k+2*256]*boostAudio; // center
			bassBuffer[k*6+3] = samples[k+0*256]*boostAudio; // LFE
			bassBuffer[k*6+4] = samples[k+4*256]*boostAudio; // left rear
			bassBuffer[k*6+5] = samples[k+5*256]*boostAudio; // right rear
		}

		unsigned avail = BASS_ChannelGetData(dtsStream, NULL, BASS_DATA_AVAILABLE);
		if(avail > AUDIO_BUF_OVERRUN_LIMIT * /*channels*/ 6) { // skip if buffering too much
			cout << "---------- " << timeString() << endl;
			cout << "AUDIO BUFFER OVERRUN: " << avail << " > " << "(AUDIO_BUF_OVERRUN_LIMIT * " << 6 << ")\n";
		} else {
			RT_ASSERT(BASS_StreamPutData(dtsStream, bassBuffer, BBUFFER_LENGTH * sizeof(float)) != -1, "Failed to forward DD audio to BASS");
		}
	}
}

bool AudioRenderer::tryDD(long samples, void* data)
{
	static uint8_t buffer[PRE_DECODE_BUFFER_SIZE];
	static long pos = 0;
	static long maxPos = 0;

	long sampleLength = samples * 2 * channels;
	memcpy(buffer+maxPos, data, sampleLength);
	long pMaxPos = maxPos;
	maxPos += sampleLength;

	// Convert Endianness
	for(int i=pMaxPos; i<maxPos; i+=2) {
		uint8_t t = buffer[i];
		buffer[i] = buffer[i+1];
		buffer[i+1] = t;
	}

	bool foundFrame = false;
	while(maxPos-pos > AC3_MAX_CODED_FRAME_SIZE) {
		int flags, sampleRate, bitRate;
		int ret = a52_syncinfo(buffer+pos, &flags, &sampleRate, &bitRate);
		if(ret > 0) {
			foundFrame = true;
			NONRELEASE(printf("DD sync pos: %d, ret: %d, flags: %d, srate: %d, brate: %d\n", pos, ret, flags, sampleRate, bitRate));
			flags = A52_3F2R | A52_LFE | A52_ADJUST_LEVEL;
			sample_t level = 1.0f;
			a52_frame(a52State, buffer+pos, &flags, &level, 0.0f);
			a52_dynrng(a52State, NULL, buffer+pos);
			decodeDDFrame();
			pos += ret;
		} else {
			pos += 2;
		}

		if(pos > PRE_DECODE_BUFFER_SIZE/2) {
			memcpy(buffer, buffer+PRE_DECODE_BUFFER_SIZE/2, PRE_DECODE_BUFFER_SIZE/2);
			pos -= PRE_DECODE_BUFFER_SIZE/2;
			maxPos -= PRE_DECODE_BUFFER_SIZE/2;
		}
	}
	return foundFrame;
}


	// this was the libavcodec DD decoding version
	// keeping it here since I don't want to ever have to figure out how to use that API again.
	//needs:
	//	#include <libavcodec/avcodec.h>
	//	#include <libavutil/avstring.h>
	//	#include <libavutil/channel_layout.h>
	//linking:
	// avutil.lib;avcodec.lib
	//vars:
	//	AVCodec* ac3Decoder;
	//	AVCodecContext* ac3Context;
	//	AVFrame *ac3Frame;
	//init:
	//	avcodec_register_all();
	//	ac3Decoder = avcodec_find_decoder(AVCodecID::AV_CODEC_ID_AC3);
	//	ac3Context = avcodec_alloc_context3(ac3Decoder);
	//	avcodec_open2(ac3Context, ac3Decoder, NULL);
	//shutdown:
	//	avcodec_close(ac3Context);
	//	avcodec_free_frame(&ac3Frame);
	//
	//void pushDDFrame(AVFrame* frame) {
	//	bufferUnderrunProtection(dtsStream, 6);
	//	char buffer[128];
	//	av_get_channel_layout_string(buffer, 128, ac3Context->channels, ac3Context->channel_layout);
	//	printf("frame:\n - channels: %d\n - samples: %d\n - layout: %s\n - format: %s\n - rate: %d\n - buffer size: %d\n", 
	//		ac3Context->channels, frame->nb_samples, buffer, av_get_sample_fmt_name(ac3Context->sample_fmt), ac3Context->sample_rate,
	//		av_samples_get_buffer_size(NULL, ac3Context->channels, frame->nb_samples, ac3Context->sample_fmt, 1));
	//	//RT_ASSERT(ac3Context->channels == 6, "DD audio not 5.1");
	//	RT_ASSERT(ac3Context->sample_fmt == AV_SAMPLE_FMT_FLTP, "DD audio not in planar float format");
	//	static float bassBuffer[AC3_MAX_CODED_FRAME_SIZE * 6];
	//	// convert samples to BASS interleaved layout
	//	// BASS stream order: left-front, right-front, center, LFE, left-rear/side, right-rear/side
	//	for(int s=0; s<frame->nb_samples; ++s) {
	//		bassBuffer[s*6+0] = 0.0f; // left front
	//		bassBuffer[s*6+1] = ((float*)frame->data[0])[s]; // right front
	//		bassBuffer[s*6+2] = 0.0f; // center
	//		bassBuffer[s*6+3] = 0.0f; // LFE
	//		bassBuffer[s*6+4] = 0.0f; // left rear
	//		bassBuffer[s*6+5] = 0.0f; // right rear
	//	}
	//	// push out converted samples
	//	RT_ASSERT(BASS_StreamPutData(dtsStream, bassBuffer, frame->nb_samples * 4 * 6) != -1, "Failed to forward DD audio to BASS");
	//}
	//
	//bool tryDD(long samples, void* data) {
	//	static uint8_t buffer[32000];
	//	static long pos = 0;
	//	static long maxPos = 0;
	//
	//	long sampleLength = samples * 2 * channels;
	//	memcpy(buffer+pos, data, sampleLength);
	//	maxPos += sampleLength;
	//
	//	while(maxPos-pos > AC3_MAX_CODED_FRAME_SIZE) {
	//		if(!ac3Frame) ac3Frame = avcodec_alloc_frame();
	//		else avcodec_get_frame_defaults(ac3Frame);
	//		int gotFrame;
	//		AVPacket packet;
	//		packet.size = AC3_MAX_CODED_FRAME_SIZE;
	//		packet.data = buffer+pos;
	//		packet.dts = packet.pts = AV_NOPTS_VALUE;
	//
	//		__declspec(align(64)) static uint8_t aligned_buffer[32000];
	//		memcpy(aligned_buffer, packet.data, packet.size);
	//		packet.data = aligned_buffer;
	//
	//		int ret = avcodec_decode_audio4(ac3Context, ac3Frame, &gotFrame, &packet);
	//		if(ret >= 0) {
	//			if(gotFrame) pushDDFrame(ac3Frame);
	//			pos += ret;
	//		} else {
	//			pos += 1;
	//		}
	//
	//		if(pos > 16000) {
	//			memcpy(buffer, buffer+16000, 16000);
	//			pos -= 16000;
	//			maxPos -= 16000;
	//		}
	//	}
	//	return true;
	//}
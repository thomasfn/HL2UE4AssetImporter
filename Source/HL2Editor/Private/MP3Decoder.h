#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "mpg123.h"

// Based on (but heavily modified) from https://github.com/kat0r/UE4_MP3Importer

DECLARE_LOG_CATEGORY_EXTERN(LogMP3Decoder, Log, All);

// https://gist.github.com/Jon-Schneider/8b7c53d27a7a13346a643dac9c19d34f
struct FWavHeader {
public:
	// RIFF Header
	char riff_header[4]; // Contains "RIFF"
	int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
	char wave_header[4]; // Contains "WAVE"

	// Format Header
	char fmt_header[4]; // Contains "fmt " (includes trailing space)
	int fmt_chunk_size; // Should be 16 for PCM
	short audio_format; // Should be 1 for PCM. 3 for IEEE Float
	short num_channels;
	int sample_rate;
	int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
	short sample_alignment; // num_channels * Bytes Per Sample
	short bit_depth; // Number of bits per sample

	// Data
	char data_header[4]; // Contains "data"
	int data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
	// uint8_t bytes[]; // Remainder of wave file is bytes
};

constexpr int WavHeaderSize = sizeof(FWavHeader);
static_assert(WavHeaderSize == 44);

class FMP3Decoder
{
private:

	mpg123_handle* handle = nullptr;
	int errorHandle;
	FFeedbackContext* warn;

	size_t blockBufferSize;

	uint32 sizeInBytes;
	uint32 sampleRate;
	uint32 channels;
	uint32 bitsPerSample;

public:

	static const int BitsPerSample;

public:

	FMP3Decoder(FFeedbackContext* warnContext);
	~FMP3Decoder();

public:

	void Init(const uint8*& Buffer, const uint8* BufferEnd);
	bool Decode(TArray<uint8>& OutBuffer);


public:

	inline uint32 GetSizeInBytes() const;
	inline uint32 GetSampleRate() const;
	inline uint32 GetChannels() const;
	inline uint32 GetBitsPerSample() const;

private:

	static FWavHeader GetWaveHeader(uint32 dataLength, uint32 sampleRate, uint32 channels);
	
};

inline uint32 FMP3Decoder::GetSizeInBytes() const { return sizeInBytes; }
inline uint32 FMP3Decoder::GetSampleRate() const { return sampleRate; }
inline uint32 FMP3Decoder::GetChannels() const { return channels; }
inline uint32 FMP3Decoder::GetBitsPerSample() const { return bitsPerSample; }

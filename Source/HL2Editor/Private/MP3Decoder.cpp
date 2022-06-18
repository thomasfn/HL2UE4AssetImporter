#include "MP3Decoder.h"

// Based on (but heavily modified) from https://github.com/kat0r/UE4_MP3Importer

DEFINE_LOG_CATEGORY(LogMP3Decoder);

const int FMP3Decoder::BitsPerSample = 16;

FMP3Decoder::FMP3Decoder(FFeedbackContext* warnContext)
	: warn(warnContext)
{ }

FMP3Decoder::~FMP3Decoder()
{
	if (handle)
	{
		mpg123_close(handle);
		mpg123_delete(handle);
		mpg123_exit();
	}
}

void FMP3Decoder::Init(const uint8*& Buffer, const uint8* BufferEnd)
{
	mpg123_init();
	handle = mpg123_new(NULL, &errorHandle);
	blockBufferSize = mpg123_outblock(handle);

	long _sampleRate;
	int _channels;
	int _encoding;
	mpg123_open_feed(handle);
	mpg123_feed(handle, Buffer, BufferEnd - Buffer);
	mpg123_getformat(handle, &_sampleRate, &_channels, &_encoding);
	const uint32 bytesPerSample = mpg123_encsize(_encoding);

	bitsPerSample = bytesPerSample * 8;
	sampleRate = (uint32)_sampleRate;
	channels = (uint32)_channels;

	UE_LOG(LogMP3Decoder, Display, TEXT("Initialized: Samplerate: %u, Channels: %u"), sampleRate, channels);
}

bool FMP3Decoder::Decode(TArray<uint8>& OutBuffer)
{
	check(OutBuffer.Num() == 0);
	OutBuffer.AddZeroed(WavHeaderSize);

	const FDateTime tStart = FDateTime::Now();

	TArray<uint8> blockBuffer;
	blockBuffer.AddUninitialized(blockBufferSize);
	size_t bytesRead = 0;
	size_t done = 0;
	int result;

	do
	{
		result = mpg123_read(handle, blockBuffer.GetData(), blockBufferSize, &done);
		bytesRead += done;
		OutBuffer.Append(blockBuffer.GetData(), done);
	} while (result == MPG123_OK);

	const FWavHeader header = GetWaveHeader(bytesRead, sampleRate, channels);
	FMemory::Memcpy(OutBuffer.GetData(), &header, WavHeaderSize);

	sizeInBytes = bytesRead;
	const bool bSuccess = result == MPG123_OK || result == MPG123_DONE;

	UE_LOG(LogMP3Decoder, Display, TEXT("Decoding finished. %s bytes in %d ms. Success: %s"),
        *FString::FormatAsNumber((int32)bytesRead), (int32)(FDateTime::Now() - tStart).GetTotalMilliseconds(), bSuccess ? TEXT("True") : TEXT("False"));

	return bSuccess;
}

FWavHeader FMP3Decoder::GetWaveHeader(uint32 dataLength, uint32 sampleRate, uint32 channels)
{
	FWavHeader outHeader;

	// RIFF Header
	FMemory::Memcpy(outHeader.riff_header, "RIFF", 4);
	outHeader.wav_size = dataLength + WavHeaderSize - 8;
	FMemory::Memcpy(outHeader.wave_header, "WAVE", 4);

	// Format Header
	FMemory::Memcpy(outHeader.fmt_header, "fmt ", 4);
	outHeader.fmt_chunk_size = 16;
	outHeader.audio_format = 1;
	outHeader.num_channels = channels;
	outHeader.sample_rate = sampleRate;
	outHeader.byte_rate = sampleRate * 16 * 2;
	outHeader.sample_alignment = channels * (16 + 7) / 8;
	outHeader.bit_depth = 16;

	// Data Header
	FMemory::Memcpy(outHeader.data_header, "data", 4);
	outHeader.data_bytes = dataLength;

	return outHeader;
}

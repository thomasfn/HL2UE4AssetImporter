#include "MP3SoundFactory.h"
#include "UnrealEd.h"
#include "Factories.h"
#include "Editor.h"
#include "AudioDeviceManager.h"
#include "MP3Decoder.h"

// Based on (but somewhat modified) from https://github.com/kat0r/UE4_MP3Importer

static bool bMP3SoundFactoryIsReimport = false;

UMP3SoundFactory::UMP3SoundFactory()
{
	SupportedClass = USoundWave::StaticClass();
	Formats.Add(TEXT("mp3;Sound"));

	// without higher priority, the MediaImporter tries to steal our file
	ImportPriority++;

	bCreateNew = false;
	bEditorImport = true;
}

UMP3SoundFactory::~UMP3SoundFactory()
{

}

UObject* UMP3SoundFactory::FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
	FString fileExt = FPaths::GetExtension(CurrentFilename);
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, Class, InParent, Name, *fileExt);

	// if the sound already exists, remember the user settings
	USoundWave* ExistingSound = FindObject<USoundWave>(InParent, *Name.ToString());

	// stop playing the file, if it already exists (e.g. reimport)
	TArray<UAudioComponent*> ComponentsToRestart;
	FAudioDeviceManager* AudioDeviceManager = GEngine->GetAudioDeviceManager();
	if (AudioDeviceManager && ExistingSound)
	{
		AudioDeviceManager->StopSoundsUsingResource(ExistingSound, &ComponentsToRestart);
	}

	// Read the mp3 header and make sure we have valid data
	FMP3Decoder Decoder(Warn);
	Decoder.Init(Buffer, BufferEnd);

	if (Decoder.GetBitsPerSample() != 16)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Unreal only supports 16bit WAVE data (%s)."), *Name.ToString());
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}

	if (Decoder.GetChannels() != 1 && Decoder.GetChannels() != 2)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Unreal only supports 1-2 channel WAVE data (Mono/Stereo). (%s)."), *Name.ToString());
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}

	//on reimport, reuse settings, wipe data. otherwise create new. (UE4 WAVE import has some more checks, maybe implement, too?)

	USoundWave* Sound;

	if (ExistingSound && bMP3SoundFactoryIsReimport)
	{
		Sound = ExistingSound;
		Sound->FreeResources();
		Sound->InvalidateCompressedData();
	}
	else
	{
		Sound = NewObject<USoundWave>(InParent, Name, Flags);
	}

	Sound->AssetImportData->Update(GetCurrentFilename());

	TArray<uint8> RawWavBuffer;
	RawWavBuffer.Reserve((BufferEnd - Buffer) * 16);

	//actual decoding
	Decoder.Decode(RawWavBuffer);

	Sound->RawData.Lock(LOCK_READ_WRITE);
	void* LockedData = Sound->RawData.Realloc(RawWavBuffer.Num() * RawWavBuffer.GetTypeSize());
	FMemory::Memcpy(LockedData, RawWavBuffer.GetData(), RawWavBuffer.Num() * RawWavBuffer.GetTypeSize());
	Sound->RawData.Unlock();
	RawWavBuffer.Empty();

	// Calculate duration.
	Sound->Duration = (float)Decoder.GetSizeInBytes() / Decoder.GetSampleRate() / Decoder.GetChannels() / (FMP3Decoder::BitsPerSample / 8);
	Sound->SetSampleRate(Decoder.GetSampleRate());
	Sound->NumChannels = Decoder.GetChannels();
	Sound->RawPCMDataSize = Decoder.GetSizeInBytes();

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Sound);

	if (ExistingSound)
	{
		Sound->PostEditChange();
	}

	for (int32 ComponentIndex = 0; ComponentIndex < ComponentsToRestart.Num(); ++ComponentIndex)
	{
		ComponentsToRestart[ComponentIndex]->Play();
	}

	return Sound;
}

// FReimportHandler

bool UMP3SoundFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	USoundWave* SoundWave = Cast<USoundWave>(Obj);
	if (SoundWave)
	{
		SoundWave->AssetImportData->ExtractFilenames(OutFilenames);
		return true;
	}
	return false;
}

void UMP3SoundFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	USoundWave* SoundWave = Cast<USoundWave>(Obj);
	if (SoundWave)
	{
		SoundWave->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UMP3SoundFactory::Reimport(UObject* Obj)
{
	if (Obj == nullptr || Cast<USoundWave>(Obj) == nullptr)
	{
		return EReimportResult::Failed;
	}

	USoundWave* SoundWave = Cast<USoundWave>(Obj);

	const FString Filename = SoundWave->AssetImportData->GetFirstFilename();
	const FString FileExtension = FPaths::GetExtension(Filename);

	if (FileExtension.Compare("MP3", ESearchCase::IgnoreCase) != 0)
		return EReimportResult::Failed;

	//UE_LOG(MP3ImporterLog, Log, TEXT("Reimport triggered for %s"), *Filename);

	if (UFactory::StaticImportObject(SoundWave->GetClass(), SoundWave->GetOuter(), *SoundWave->GetName(), RF_Public | RF_Standalone, *Filename, nullptr, this))
	{
		//UE_LOG(MP3ImporterLog, Log, TEXT("Reimport successfull"));

		SoundWave->AssetImportData->Update(Filename);
		SoundWave->MarkPackageDirty();

		return EReimportResult::Succeeded;
	}
	else
	{
		//UE_LOG(MP3ImporterLog, Warning, TEXT("Reimport failed!"));
		return EReimportResult::Failed;
	}
}

int32 UMP3SoundFactory::GetPriority() const
{
	return ImportPriority;
}

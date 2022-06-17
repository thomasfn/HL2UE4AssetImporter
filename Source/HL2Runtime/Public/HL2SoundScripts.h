#pragma once

#include "CoreMinimal.h"

#include "HL2SoundScripts.generated.h"

class UHL2SoundScapes;

UENUM(BlueprintType)
enum class EHL2SoundScriptEntryFlag : uint8
{
	// * - Streams from the disc, get flushed soon after. Use for one-off dialogue files or music.
    CHAR_STREAM,
	// # - Bypasses DSP and affected by the user's music volume setting.
	CHAR_DRYMIX,
	// @ Non-directional sound; plays "everywhere", similar to SNDLVL_NONE, except it fades with distance from its source based on its sound level.
	CHAR_OMNI,
	// > - Doppler encoded stereo: left for heading towards the listener and right for heading away.
	CHAR_DOPPLER,
	// < - Stereo with direction: left channel for front facing, right channel for rear facing. Mixed based on listener's direction.
	CHAR_DIRECTIONAL,
	// ^ - Distance-variant stereo. Left channel is close, right channel is far. Transition distance is hard-coded.
	CHAR_DISTVARIANT,
	// ) - Spatializes both channels, allowing them to be placed at specific locations within the world; see below.
	CHAR_SPATIALSTEREO,
	// } - Forces low quality, non-interpolated pitch shift.
	CHAR_FAST_PITCH,
	// $ - Memory resident; cache locked. (removed since Counter-Strike: Global Offensive)
	CHAR_CRITICAL,
	// ! - An NPC sentence.
	CHAR_SENTENCE,
	// ? - Voice chat data. You shouldn't ever need to use this.
	CHAR_USERVOX,
	// & - Indicates wav should use HRTF spatialization for all entities (including owners).
	CHAR_HRTF_FORCE,
	// ~ - Indicates wav should use HRTF spatialization for non-owners.
	CHAR_HRTF,
	// ` - Indicates wav should use HRTF spatialization for non-owners, blended with stereo for sounds sufficiently close.
	CHAR_HRTF_BLEND,
	// + - Indicates a 'radio' sound -- should be played without spatialization
	CHAR_RADIO,
	// ( - Indicates directional stereo wav (like doppler)
	CHAR_DIRSTEREO,
	// $ - Indicates the subtitles are forced
	CHAR_SUBTITLED,
	// % Indicates main menu music
	CHAR_MUSIC
};

USTRUCT(BlueprintType)
struct FHL2SoundScriptWave
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	FString Path;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	TSet<EHL2SoundScriptEntryFlag> Flags;

};

USTRUCT(BlueprintType)
struct FHL2SoundScriptEntry
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	FName EntryName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	uint8 Channel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	float VolumeMin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	float VolumeMax;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	uint8 PitchMin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	uint8 PitchMax;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	uint8 SoundLevelMin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	uint8 SoundLevelMax;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	TArray<FHL2SoundScriptWave> Waves;

};

UENUM(BlueprintType)
enum class EHL2SoundScapeRuleType : uint8
{
	PlayLooping,
	PlayRandom,
};

USTRUCT(BlueprintType)
struct FHL2SoundScapeRule
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	EHL2SoundScapeRuleType RuleType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	uint8 Position;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	FHL2SoundScriptEntry SoundScript;

};

USTRUCT(BlueprintType)
struct FHL2SoundScapeSubSoundScapeRule
{
	GENERATED_BODY()

public:


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	float Volume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	uint8 Position;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	FName SubSoundScapeName;

};

USTRUCT(BlueprintType)
struct FHL2SoundScape
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	FName SoundscapeName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	uint8 DSPEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	float DSPVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	TArray<FHL2SoundScapeRule> Rules;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	TArray<FHL2SoundScapeSubSoundScapeRule> SubSoundScapeRules;

public:

	void GetCompiledRules(UHL2SoundScapes* soundScapes, TArray<FHL2SoundScapeRule>& outRules) const;

private:

	void GetCompiledRulesInternal(UHL2SoundScapes* soundScapes, TArray<FHL2SoundScapeRule>& outRules, float volume, uint8 position) const;

};


UCLASS(BlueprintType)
class HL2RUNTIME_API UHL2SoundScripts : public UDataAsset
{
	GENERATED_BODY()

public:


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	TMap<FName, FHL2SoundScriptEntry> Entries;

public:

	

};

UCLASS(BlueprintType)
class HL2RUNTIME_API UHL2SoundScapes : public UDataAsset
{
	GENERATED_BODY()

public:


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HL2")
	TMap<FName, FHL2SoundScape> Entries;

public:

	UFUNCTION(BlueprintCallable, Category = "HL2")
	bool LookupSoundScape(UHL2SoundScapes* soundScapes, const FName soundScapeName, FHL2SoundScape& outSoundScape, TArray<FHL2SoundScapeRule>& outRules);

};

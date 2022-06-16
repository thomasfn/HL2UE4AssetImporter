#include "SoundScriptUtils.h"

const TMap<FString, uint8> FSoundScriptUtils::ChannelNames =
{
	// Default, generic channel.
	{ TEXT("CHAN_AUTO"), 0 },
	// Playerand NPC weapons.
	{ TEXT("CHAN_WEAPON"), 1 },
	// Voiceover dialogue.Used for regular voice lines, etc. Note : If sound with this channel is emit by an entity that has the "mouth" attachment, the sound comes from it.
	{ TEXT("CHAN_VOICE"), 2 },
	// Additional voice channel.Used in Team Fortress 2 for the announcer.
	{ TEXT("CHAN_VOICE2"), 3 },
	// Generic physics impact sounds, health / suit chargers, +use sounds.
	{ TEXT("CHAN_ITEM"), 4 },
	// Clothing, ragdoll impacts, footsteps, knocking / pounding / punching etc.
	{ TEXT("CHAN_BODY"), 5 },
	// Sounds that can be delayed by an async load, i.e.aren't responses to particular events.
	{ TEXT("CHAN_STREAM"), 6 },
	// Used when playing sounds through console commands.
	{ TEXT("CHAN_REPLACE"), 7 },
	// A constant / background sound that doesn't require any reaction.
	{ TEXT("CHAN_STATIC"), 8 },
	// Network voice data(online voice communications)
	{ TEXT("CHAN_VOICE_BASE"), 9 },
	{ TEXT("CHAN_USER_BASE"), 10 }
};

const TMap<FString, float> FSoundScriptUtils::VolumeNames =
{
	{ TEXT("VOL_NORM"), 1.0f }
};

const TMap<FString, uint8> FSoundScriptUtils::PitchNames =
{
	{ TEXT("PITCH_LOW"), 95 },
	{ TEXT("PITCH_NORM"), 100 },
	{ TEXT("PITCH_HIGH"), 120 }
};

const TMap<FString, uint8> FSoundScriptUtils::SoundLevelNames =
{
	// Everywhere
	{ TEXT("SNDLVL_NONE"), 0 },
	// Rustling leaves
	{ TEXT("SNDLVL_20dB"), 20 },
	// Whispering
	{ TEXT("SNDLVL_25dB"), 25 },
	// Library, dishwasher
	{ TEXT("SNDLVL_30dB"), 30 },
	{ TEXT("SNDLVL_35dB"), 35 },
	// Moderate rainfall
	{ TEXT("SNDLVL_40dB"), 40 },
	// Refrigerator
	{ TEXT("SNDLVL_45dB"), 45 },
	// Average home, heavy rainfall
	{ TEXT("SNDLVL_50dB"), 50 },
	{ TEXT("SNDLVL_55dB"), 55 },
	// Normal conversation, clothes dryer
	{ TEXT("SNDLVL_IDLE"), 60 },
	// Washing machine, dishwasher
	{ TEXT("SNDLVL_65dB"), 65 },
	{ TEXT("SNDLVL_STATIC"), 65 },
	// Car, vacuum cleaner, mixer, electric sewing machine
	{ TEXT("SNDLVL_70dB"), 70 },
	{ TEXT("SNDLVL_75dB"), 75 },
	// Busy traffic
	{ TEXT("SNDLVL_NORM"), 75 },
	// Mini - bike, alarm clock, noisy restaurant, office tabulator, outboard motor, passing snowmobile, police siren
	{ TEXT("SNDLVL_80dB"), 80 },
	// Valve's chosen dialogue attenuation
	{ TEXT("SNDLVL_TALKING"), 80 },
	// Average factory, electric shaver	Left 4 Dead 2 : Moustachio strength attract, big fire after airliner crash in C11M5_runway
	{ TEXT("SNDLVL_85dB"), 85 },
	// Screaming child, passing motorcycle, convertible ride on freeway, hair dryer	Left 4 Dead 2 : Moustachio strength attract, gas can bursting
	{ TEXT("SNDLVL_90dB"), 90 },
	{ TEXT("SNDLVL_95dB"), 95 },
	// Subway train, diesel truck, woodworking shop, pneumatic drill, boiler shop, jackhammer	Left 4 Dead 2 : Music coop player in danger AKA tags(ledge hanging, pinned down by SI), Elevators.
	{ TEXT("SNDLVL_100dB"), 100 },
	// Helicopter, power mower
	{ TEXT("SNDLVL_105dB"), 105 },
	// Snowmobile(drivers seat), inboard motorboat, sandblasting, chainsaw
	{ TEXT("SNDLVL_110dB"), 110 },
	// Car horn, propeller aircraft, concert
	{ TEXT("SNDLVL_120dB"), 120 },
	{ TEXT("SNDLVL_125dB"), 125 },
	// Air raid siren, fireworks
	{ TEXT("SNDLVL_130dB"), 130 },
	// Threshold of pain, gunshot, jet engine
	{ TEXT("SNDLVL_GUNFIRE"), 135 },
	// Airplane taking off, jackhammer
	{ TEXT("SNDLVL_140dB"), 140 },
	{ TEXT("SNDLVL_145dB"), 145 },
	// Smithing hammer
	{ TEXT("SNDLVL_150dB"), 150 },
	// Rocket launching
	{ TEXT("SNDLVL_180dB"), 180 }
};

const TMap<TCHAR, EHL2SoundScriptEntryFlag> FSoundScriptUtils::CharToFlag =
{
	{ '*', EHL2SoundScriptEntryFlag::CHAR_STREAM },
	{ '#', EHL2SoundScriptEntryFlag::CHAR_DRYMIX },
	{ '@', EHL2SoundScriptEntryFlag::CHAR_OMNI },
	{ '>', EHL2SoundScriptEntryFlag::CHAR_DOPPLER },
	{ '<', EHL2SoundScriptEntryFlag::CHAR_DIRECTIONAL },
	{ '^', EHL2SoundScriptEntryFlag::CHAR_DISTVARIANT },
	{ ')', EHL2SoundScriptEntryFlag::CHAR_SPATIALSTEREO },
	{ '}', EHL2SoundScriptEntryFlag::CHAR_FAST_PITCH },
	{ '$', EHL2SoundScriptEntryFlag::CHAR_CRITICAL },
	{ '!', EHL2SoundScriptEntryFlag::CHAR_SENTENCE },
	{ '?', EHL2SoundScriptEntryFlag::CHAR_USERVOX },
	{ '&', EHL2SoundScriptEntryFlag::CHAR_HRTF_FORCE },
	{ '~', EHL2SoundScriptEntryFlag::CHAR_HRTF },
	{ '\'', EHL2SoundScriptEntryFlag::CHAR_HRTF_BLEND },
	{ '+', EHL2SoundScriptEntryFlag::CHAR_RADIO },
	{ '(', EHL2SoundScriptEntryFlag::CHAR_DIRSTEREO },
	{ '$', EHL2SoundScriptEntryFlag::CHAR_SUBTITLED },
	{ '%', EHL2SoundScriptEntryFlag::CHAR_MUSIC }
};

void FSoundScriptUtils::ParseSoundScriptEntry(const UValveGroupValue* GroupValue, FHL2SoundScriptEntry& OutEntry)
{
	static const FName fnValveChannel(TEXT("channel"));
	static const FName fnValveVolume(TEXT("volume"));
	static const FName fnValvePitch(TEXT("pitch"));
	static const FName fnValveSoundLevel(TEXT("soundlevel"));
	static const FName fnValveWave(TEXT("wave"));
	static const FName fnValveRndWave(TEXT("rndwave"));

	ParseGenericNumber<uint8>(GroupValue->GetPrimitive(fnValveChannel), ChannelNames, OutEntry.Channel, 0);
	ParseGenericNumber<float>(GroupValue->GetPrimitive(fnValveVolume), VolumeNames, OutEntry.VolumeMin, OutEntry.VolumeMax, 1.0f);
	ParseGenericNumber<uint8>(GroupValue->GetPrimitive(fnValvePitch), PitchNames, OutEntry.PitchMin, OutEntry.PitchMax, 100);
	ParseGenericNumber<uint8>(GroupValue->GetPrimitive(fnValveSoundLevel), SoundLevelNames, OutEntry.SoundLevelMin, OutEntry.SoundLevelMax, 75);
	const UValveGroupValue* rndWaveValue = GroupValue->GetGroup(fnValveRndWave);
	if (rndWaveValue != nullptr)
	{
		OutEntry.Waves.Reserve(rndWaveValue->Items.Num());
		OutEntry.WaveFlags.Reserve(rndWaveValue->Items.Num());
		for (const FValveGroupKeyValue& keyValue : rndWaveValue->Items)
		{
			if (keyValue.Key != fnValveWave) { continue; }
			const UValvePrimitiveValue* itemPrimitive = CastChecked<UValvePrimitiveValue>(keyValue.Value);
			FString wave;
			EHL2SoundScriptEntryFlag flag;
			ParseWave(itemPrimitive, wave, flag);
			OutEntry.Waves.Add(wave);
			OutEntry.WaveFlags.Add(flag);
		}
	}
	else
	{
		const UValvePrimitiveValue* waveValue = GroupValue->GetPrimitive(fnValveWave);
		if (waveValue != nullptr)
		{
			FString wave;
			EHL2SoundScriptEntryFlag flag;
			ParseWave(waveValue, wave, flag);
			OutEntry.Waves.Add(wave);
			OutEntry.WaveFlags.Add(flag);
		}
	}
}

void FSoundScriptUtils::ParseWave(const UValvePrimitiveValue* Value, FString& OutWave, EHL2SoundScriptEntryFlag& OutFlag)
{
	const FString text = Value->AsString();
	check(text.Len() > 0);
	const TCHAR firstChar = text[0];
	const EHL2SoundScriptEntryFlag* flagPtr = CharToFlag.Find(firstChar);
	if (flagPtr != nullptr)
	{
		OutFlag = *flagPtr;
		OutWave = text.Right(text.Len() - 1);
		return;
	}
	OutWave = text;
	OutFlag = EHL2SoundScriptEntryFlag::None;
}

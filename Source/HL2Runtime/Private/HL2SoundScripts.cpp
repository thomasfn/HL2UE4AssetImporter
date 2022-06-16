#include "HL2SoundScripts.h"

void FHL2SoundScape::GetCompiledRules(UHL2SoundScapes* soundScapes, TArray<FHL2SoundScapeRule>& outRules) const
{
	GetCompiledRulesInternal(soundScapes, outRules, 1.0f, 0);
}

void FHL2SoundScape::GetCompiledRulesInternal(UHL2SoundScapes* soundScapes, TArray<FHL2SoundScapeRule>& outRules, float volume, uint8 position) const
{
	outRules.Reserve(Rules.Num());
	for (FHL2SoundScapeRule rule : Rules)
	{
		rule.SoundScript.VolumeMin *= volume;
		rule.SoundScript.VolumeMax *= volume;
		rule.Position += position;
		outRules.Add(rule);
	}
	for (const FHL2SoundScapeSubSoundScapeRule& subSoundScapeRule : SubSoundScapeRules)
	{
		const FHL2SoundScape* soundScapePtr = soundScapes->Entries.Find(subSoundScapeRule.SubSoundScapeName);
		if (soundScapePtr == nullptr) { continue; }
		soundScapePtr->GetCompiledRulesInternal(soundScapes, outRules, subSoundScapeRule.Volume, subSoundScapeRule.Position);
	}
}

bool UHL2SoundScapes::LookupSoundScape(UHL2SoundScapes* soundScapes, const FName soundScapeName, FHL2SoundScape& outSoundScape, TArray<FHL2SoundScapeRule>& outRules)
{
	const FHL2SoundScape* soundScapePtr = soundScapes->Entries.Find(soundScapeName);
	if (soundScapePtr == nullptr) { return false; }
	outSoundScape = *soundScapePtr;
	soundScapePtr->GetCompiledRules(soundScapes, outRules);
	return true;
}

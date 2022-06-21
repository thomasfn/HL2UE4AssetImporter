#include "BaseEntity.h"
#include "BaseEntityComponent.h"
#include "IHL2Runtime.h"
#include "TimerManager.h"
#include "VBSPInfo.h"

DEFINE_LOG_CATEGORY(LogHL2IOSystem);

// The entity that began the current I / O chain.If a player walks into a trigger that fires a logic_relay, the player is the !activator of the relay's output(s).
static const FName tnActivator(TEXT("!activator"));

// The previous entity in the current I / O chain.If a player walks into a trigger that that fires a logic_relay, the trigger is the !caller of the relay's output(s).
static const FName tnCaller(TEXT("!caller"));

// The entity from which the current output originates.If a player walks into a trigger that that fires a logic_relay, the relay is the !self of its output(s).
static const FName tnSelf(TEXT("!self"));

// The player. Only useful in singleplayer.
static const FName tnPlayer(TEXT("!player"));

// The first player found in the entity's Potential Visibility Set. The PVS used is taken from the entity doing the searching, or the activator if no searching entity exists. If no activator exists either, the first player in the game is returned (i.e. !player).
static const FName tnPVSPlayer(TEXT("!pvsplayer"));

// The entity at which the !caller is looking due to a Look At Actor or Face Actor choreography event.
static const FName tnSpeechTarget(TEXT("!speechtarget"));
	
// The first entity under the player's crosshair. Only useful in single-player, and mostly only for debugging. Entities without collision can only be selected by aiming at their origin.
static const FName tnPicker(TEXT("!picker"));

ABaseEntity::ABaseEntity()
{
	
}

void ABaseEntity::BeginPlay()
{
	Super::BeginPlay();
	ResetLogicOutputs();
	static const FName fnParentName(TEXT("parentname"));
	FString parent;
	if (EntityData.TryGetString(fnParentName, parent))
	{
		SetParent(parent, this, nullptr);
	}
}
	

/**
 * Fires a logic input on this entity.
 * Returns true if the logic input was successfully handled.
 * The caller is the entity which is directly responsible for firing the input. For example, if the player walked into a trigger that has fired this input, the trigger is the caller.
 * The activator is the entity which is indirectly responsible for firing the input. For example, if the player walked into a trigger that has fired this input, the player is the activator.
 * Either caller or activator, or both, may be null.
 */
bool ABaseEntity::FireInput(const FName inputName, const TArray<FString>& args, ABaseEntity* caller, ABaseEntity* activator)
{
	UE_LOG(LogHL2IOSystem, Log, TEXT("Entity %s:%s receiving '%s' with %d args"), *TargetName.ToString(), *EntityData.Classname.ToString(), *inputName.ToString(), args.Num());

	// Handle basic inputs
	static const FName inKill(TEXT("Kill"));
	static const FName inKillHierarchy(TEXT("KillHierarchy"));
	static const FName inAddOutput(TEXT("AddOutput"));
	static const FName inFireUser1(TEXT("FireUser1"));
	static const FName inFireUser2(TEXT("FireUser2"));
	static const FName inFireUser3(TEXT("FireUser3"));
	static const FName inFireUser4(TEXT("FireUser4"));
	static const FName inSetParent(TEXT("SetParent"));
	static const FName inSetParentAttachment(TEXT("SetParentAttachment"));
	static const FName inSetParentAttachmentMaintainOffset(TEXT("SetParentAttachmentMaintainOffset"));
	static const FName inClearParent(TEXT("ClearParent"));
	static const FName onFireUser1(TEXT("OnUser1"));
	static const FName onFireUser2(TEXT("OnUser2"));
	static const FName onFireUser3(TEXT("OnUser3"));
	static const FName onFireUser4(TEXT("OnUser4"));
	if ((inputName == inKill || inputName == inKillHierarchy) && !CustomKill)
	{
		Destroy();
		return true;
	}
	else if (inputName == inAddOutput)
	{
		// Not yet supported!
		UE_LOG(LogHL2IOSystem, Log, TEXT("Entity %s:%s receiving 'AddOutput' which is not yet implemented"), *TargetName.ToString(), *EntityData.Classname.ToString());
		return false;
	}
	else if (inputName == inFireUser1)
	{
		FireOutput(onFireUser1, args, this, caller);
		return true;
	}
	else if (inputName == inFireUser2)
	{
		FireOutput(onFireUser2, args, this, caller);
		return true;
	}
	else if (inputName == inFireUser3)
	{
		FireOutput(onFireUser3, args, this, caller);
		return true;
	}
	else if (inputName == inFireUser4)
	{
		FireOutput(onFireUser4, args, this, caller);
		return true;
	}
	else if (inputName == inSetParent)
	{
		if (args.Num() == 0)
		{
			SetParent(TEXT(""), caller, activator);
		}
		else
		{
			SetParent(args[0], caller, activator);
		}
		return true;
	}
	else if (inputName == inSetParentAttachment)
	{
		AActor* attachedActor = GetAttachParentActor();
		if (!IsValid(attachedActor) || args.Num() == 0) { return true; }
		AttachToActor(attachedActor, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false), FName(*args[0]));
		return true;
	}
	else if (inputName == inSetParentAttachmentMaintainOffset)
	{
		AActor* attachedActor = GetAttachParentActor();
		if (!IsValid(attachedActor) || args.Num() == 0) { return true; }
		AttachToActor(attachedActor, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false), FName(*args[0]));
		return true;
	}
	else if (inputName == inClearParent)
	{
		SetParent(TEXT(""), caller, activator);
		return true;
	}
	else
	{
		// Let components handle it
		for (UActorComponent* component : GetComponentsByClass(UBaseEntityComponent::StaticClass()))
		{
			UBaseEntityComponent* baseEntityComponent = CastChecked<UBaseEntityComponent>(component);
			baseEntityComponent->OnInputFired(inputName, args, caller, activator);
		}

		// Let derived blueprint handle it
		OnInputFired(inputName, args, caller, activator);

		// Assume success - we're not bothering with having OnInputFired return a bool yet
		return true;
	}
}

/**
 * Fires a logic output on this entity.
 * Returns the number of entities that succesfully handled the output.
 */
int ABaseEntity::FireOutput(const FName outputName, const TArray<FString>& args, ABaseEntity* caller, ABaseEntity* activator)
{
	UE_LOG(LogHL2IOSystem, Log, TEXT("Entity %s:%s firing '%s' with %d args"), *TargetName.ToString(), *EntityData.Classname.ToString(), *outputName.ToString(), args.Num());

	// Gather all relevant outputs to fire
	TArray<FEntityLogicOutput> toFire;
	for (int i = LogicOutputs.Num() - 1; i >= 0; --i)
	{
		const FEntityLogicOutput& logicOutput = LogicOutputs[i];
		if (logicOutput.OutputName == outputName)
		{
			toFire.Insert(logicOutput, 0);
			if (logicOutput.Once)
			{
				LogicOutputs.RemoveAt(i);
			}
		}
	}

	// Iterate all
	int result = 0;
	for (const FEntityLogicOutput& logicOutput : toFire)
	{
		// Prepare arguments
		TArray<FString> arguments = logicOutput.Params;
		arguments.Reserve(args.Num());
		for (int i = 0; i < args.Num(); ++i)
		{
			if (i > arguments.Num())
			{
				arguments.Add(args[i]);
			}
			else if (arguments[i].IsEmpty())
			{
				arguments[i] = args[i];
			}
		}
		arguments.Reserve(FMath::Max(args.Num(), logicOutput.Params.Num()));

		// Setup pending fire
		FPendingFireOutput pendingFireOutput;
		pendingFireOutput.Output = logicOutput;
		pendingFireOutput.Args = arguments;
		pendingFireOutput.Caller = caller;
		pendingFireOutput.Activator = activator;

		// If the delay is zero, fire it immediately
		if (logicOutput.Delay <= 0.0f)
		{
			FireOutputInternal(pendingFireOutput);
		}
		else
		{
			// Setup a timer
			static const FName fnFireOutputInternal("FireOutputInternal");
			FTimerDelegate timerDelegate;
			timerDelegate.BindUFunction(this, fnFireOutputInternal, pendingFireOutput);
			GetWorldTimerManager().SetTimer(pendingFireOutput.TimerHandle, timerDelegate, logicOutput.Delay, false);
			PendingOutputs.Add(pendingFireOutput.TimerHandle);
		}
	}

	return result;
}

/**
 * Resets all logic outputs to their initial state.
 */
void ABaseEntity::ResetLogicOutputs()
{
	LogicOutputs = EntityData.LogicOutputs;
}

/**
 * Resolves a target name into an array of targets.
 * Supports wildcards and special target names.
 */
void ABaseEntity::ResolveTargetName(const FName targetNameToResolve, TArray<ABaseEntity*>& out, ABaseEntity* caller, ABaseEntity* activator) const
{
	// Test for special targetnames
	if (targetNameToResolve == tnActivator)
	{
		if (activator != nullptr) { out.Add(activator); }
	}
	else if (targetNameToResolve == tnCaller)
	{
		if (caller != nullptr) { out.Add(caller); }
	}
	else if (targetNameToResolve == tnSelf)
	{
		out.Add(const_cast<ABaseEntity*>(this));
	}
	else if (targetNameToResolve == tnPlayer)
	{
		// This depends on the player entity BP having the targetname "!player" set
		IHL2Runtime::Get().FindEntitiesByTargetName(GetWorld(), tnPlayer, out);
	}
	else if (targetNameToResolve == tnPVSPlayer)
	{
		// This depends on the player entity BP having the targetname "!player" set
		IHL2Runtime::Get().FindEntitiesByTargetName(GetWorld(), tnPlayer, out);
	}
	else if (targetNameToResolve == tnSpeechTarget)
	{
		// TODO
	}
	else if (targetNameToResolve == tnPicker)
	{
		// TODO
	}
	else
	{
		IHL2Runtime::Get().FindEntitiesByTargetName(GetWorld(), targetNameToResolve, out);
	}
}

/**
 * Sets the parent of this entity to a given target name.
 * The target name may also refer to an attachment, e.g. "parent,attachment".
 */
void ABaseEntity::SetParent(const FString& parent, ABaseEntity* caller, ABaseEntity* activator)
{
	TArray<FString> segments;
	parent.ParseIntoArray(segments, TEXT(","), true);
	ABaseEntity* newParent = nullptr;
	FName attachment = NAME_None;
	if (segments.Num() > 0)
	{
		for (FString& segment : segments)
		{
			segment.TrimStartAndEndInline();
		}
		TArray<ABaseEntity*> candidates;
		ResolveTargetName(*segments[0], candidates, caller, activator);
		for (ABaseEntity* candidate : candidates)
		{
			if (candidate == this) { continue; }
			newParent = candidate;
			break;
		}
		if (segments.Num() > 1)
		{
			attachment = *segments[1];
		}
	}
	if (newParent == nullptr)
	{
		DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
	}
	else
	{
		AttachToActor(newParent, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false), attachment);
	}
}

/**
 * Gets if this entity has the specified spawnflag.
 * Shortcut for fetching the spawnflag int from entity data and performing bitwise and.
 */
bool ABaseEntity::HasSpawnFlag(int flag) const
{
	const int spawnFlags = EntityData.GetInt("spawnflags");
	return (spawnFlags & flag) != 0;
}

void ABaseEntity::FireOutputInternal(const FPendingFireOutput& data)
{
	// Remove timer
	PendingOutputs.Remove(data.TimerHandle);

	// Resolve targetname
	TArray<ABaseEntity*> targets;
	ResolveTargetName(data.Output.TargetName, targets, data.Caller, data.Activator);

	// Iterate all target entities
	for (ABaseEntity* targetEntity : targets)
	{
		// Fire the input!
		targetEntity->FireInput(data.Output.InputName, data.Args, this, data.Caller);
	}
}

void ABaseEntity::OnInputFired_Implementation(const FName inputName, const TArray<FString>& args, ABaseEntity* caller, ABaseEntity* activator)
{
	// Left for blueprint to override and handle custom inputs
}
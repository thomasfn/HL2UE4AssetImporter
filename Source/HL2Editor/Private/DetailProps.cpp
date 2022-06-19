#include "DetailProps.h"

void FDetailProps::ParseAllDetailProps(const UValveDocument* document, TMap<FName, FDetailProps>& outProps)
{
	static const FName fnDetail(TEXT("detail"));
	const UValveGroupValue* detailGroup = document->Root->GetGroup(fnDetail);
	if (detailGroup == nullptr) { return; }
	for (const FValveGroupKeyValue& keyValue : detailGroup->Items)
	{
		FDetailProps detailProps = ParseDetailProps(CastChecked<UValveGroupValue>(keyValue.Value));
		detailProps.Name = keyValue.Key;
		outProps.Add(detailProps.Name, detailProps);
	}

}

FDetailProps FDetailProps::ParseDetailProps(const UValveGroupValue* groupValue)
{
	FDetailProps detailProps;
	float tmpFloat;
	static const FName fnDensity(TEXT("density"));
	if (groupValue->GetFloat(fnDensity, tmpFloat)) { detailProps.Density = tmpFloat; }
	for (const FValveGroupKeyValue& keyValue : groupValue->Items)
	{
		const UValveGroupValue* value = Cast<UValveGroupValue>(keyValue.Value);
		if (value == nullptr) { continue; }
		FDetailPropGroup propGroup = ParseGroup(value);
		propGroup.GroupName = keyValue.Key;
		detailProps.Groups.Add(propGroup);
	}
	return detailProps;
}

FDetailPropGroup FDetailProps::ParseGroup(const UValveGroupValue* groupValue)
{
	FDetailPropGroup detailPropGroup;
	float tmpFloat;
	static const FName fnAlpha(TEXT("alpha"));
	if (groupValue->GetFloat(fnAlpha, tmpFloat)) { detailPropGroup.Alpha = tmpFloat; }
	for (const FValveGroupKeyValue& keyValue : groupValue->Items)
	{
		const UValveGroupValue* value = Cast<UValveGroupValue>(keyValue.Value);
		if (value == nullptr) { continue; }
		FDetailPropGroupProp prop = ParseProp(value);
		prop.Name = keyValue.Key;
		detailPropGroup.Props.Add(prop);
	}
	return detailPropGroup;
}

FDetailPropGroupProp FDetailProps::ParseProp(const UValveGroupValue* groupValue)
{
	FDetailPropGroupProp detailProp;
	float tmpFloat;
	static const FName fnAmount(TEXT("amount"));
	if (groupValue->GetFloat(fnAmount, tmpFloat)) { detailProp.Amount = tmpFloat; }
	static const FName fnUpright(TEXT("upright"));
	if (groupValue->GetPrimitive(fnUpright) != nullptr) { detailProp.Upright = true; }
	static const FName fnMinAngle(TEXT("minangle"));
	if (groupValue->GetFloat(fnMinAngle, tmpFloat)) { detailProp.MinAngle = tmpFloat; }
	static const FName fnMaxAngle(TEXT("maxangle"));
	if (groupValue->GetFloat(fnMaxAngle, tmpFloat)) { detailProp.MaxAngle = tmpFloat; }
	static const FName fnModel(TEXT("model"));
	const UValvePrimitiveValue* modelValue = groupValue->GetPrimitive(fnModel);
	if (modelValue != nullptr)
	{
		detailProp.Type = EDetailPropGroupPropType::Model;
		detailProp.Model = modelValue->AsString();
	}
	static const FName fnSprite(TEXT("sprite"));
	const UValvePrimitiveValue* spriteValue = groupValue->GetPrimitive(fnSprite);
	if (spriteValue != nullptr)
	{
		detailProp.Type = EDetailPropGroupPropType::Sprite;
		
	}
	return detailProp;
}

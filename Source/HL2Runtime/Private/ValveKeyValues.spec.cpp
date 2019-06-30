#include "HL2RuntimePrivatePCH.h"

#include "Misc/AutomationTest.h"
#include "ValveKeyValues.h"

BEGIN_DEFINE_SPEC(ValveKeyValuesSpec, "HL2.ValveKeyValues.Spec", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
UValveDocument* Document = nullptr;
END_DEFINE_SPEC(ValveKeyValuesSpec)
void ValveKeyValuesSpec::Define()
{
	Describe("UValveDocument", [this]()
		{
			Describe("Parse", [this]()
				{
					It("will return an empty document when parsing a blank string", [this]()
						{
							Document = UValveDocument::Parse(TEXT(""));
							TestNotNull("Document", Document);
							TestNull("Document->Root", Document->Root);
						});

					AfterEach([this]()
						{
							if (Document != nullptr)
							{
								Document->MarkPendingKill();
								Document = nullptr;
							}
						});
				});
		});
}
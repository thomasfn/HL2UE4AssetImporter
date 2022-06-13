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
					It("will return an empty group when parsing a blank string", [this]()
						{
							Document = UValveDocument::Parse(TEXT(""));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const UValveGroupValue* root = Cast<UValveGroupValue>(Document->Root);
							TestNotNull("(UValveGroupValue*)Document->Root", root);
							if (root == nullptr) { return; }

							TestEqual("Document->Root->Items.Num()", root->Items.Num(), 0);
						});

					It("will return a non-empty group when parsing a single key-value pair", [this]()
						{
							Document = UValveDocument::Parse(TEXT("key \"test\""));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const UValveGroupValue* root = Cast<UValveGroupValue>(Document->Root);
							TestNotNull("(UValveGroupValue*)Document->Root", root);
							if (root == nullptr) { return; }

							TestEqual("Document->Root->Items.Num()", root->Items.Num(), 1);
							if (root->Items.Num() < 0) { return; }

							static const FName fnKey(TEXT("key"));
							TestEqual("Document->Root->Items[0].Key", root->Items[0].Key, fnKey);
							TestNotNull("Document->Root->Items[0].Value", root->Items[0].Value);
							const UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(root->Items[0].Value);
							TestNotNull("(UValveStringValue*)Document->Root->Items[0].Value", primValue);
							if (primValue == nullptr) { return; }
							TestEqual("Document->Root->Items[0].Value->Value", primValue->Value, TEXT("test"));
						});

					It("will ignore single-line comments", [this]()
						{
							Document = UValveDocument::Parse(TEXT("//key1 10\nkey2 20"));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const UValveGroupValue* root = Cast<UValveGroupValue>(Document->Root);
							TestNotNull("(UValveGroupValue*)Document->Root", root);
							if (root == nullptr) { return; }

							TestEqual("Document->Root->Items.Num()", root->Items.Num(), 1);
							if (root->Items.Num() < 1) { return; }
						});

					It("will parse quoted and unquoted strings as keys and values", [this]()
						{
							Document = UValveDocument::Parse(TEXT("key1 \"test1\"  \"key2\" \"test2\"  key3 test3  \"key4\" .3 .3 .3  \"key5\" test4 \n key6 test5"));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const UValveGroupValue* root = Cast<UValveGroupValue>(Document->Root);
							TestNotNull("(UValveGroupValue*)Document->Root", root);
							if (root == nullptr) { return; }

							TestEqual("Document->Root->Items.Num()", root->Items.Num(), 6);
							if (root->Items.Num() < 6) { return; }

							{
								static const FName fnKey(TEXT("key1"));
								TestEqual("Document->Root->Items[0].Key", root->Items[0].Key, fnKey);
								TestNotNull("Document->Root->Items[0].Value", root->Items[0].Value);
								const UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(root->Items[0].Value);
								TestNotNull("(UValveStringValue*)Document->Root->Items[0].Value", primValue);
								if (primValue == nullptr) { return; }
								TestEqual("Document->Root->Items[0].Value->Value", primValue->Value, TEXT("test1"));
							}
							{
								static const FName fnKey(TEXT("key2"));
								TestEqual("Document->Root->Items[1].Key", root->Items[1].Key, fnKey);
								TestNotNull("Document->Root->Items[1].Value", root->Items[1].Value);
								const UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(root->Items[1].Value);
								TestNotNull("(UValveStringValue*)Document->Root->Items[1].Value", primValue);
								if (primValue == nullptr) { return; }
								TestEqual("Document->Root->Items[1].Value->Value", primValue->Value, TEXT("test2"));
							}
							{
								static const FName fnKey(TEXT("key3"));
								TestEqual("Document->Root->Items[2].Key", root->Items[2].Key, fnKey);
								TestNotNull("Document->Root->Items[2].Value", root->Items[2].Value);
								const UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(root->Items[2].Value);
								TestNotNull("(UValveStringValue*)Document->Root->Items[2].Value", primValue);
								if (primValue == nullptr) { return; }
								TestEqual("Document->Root->Items[2].Value->Value", primValue->Value, TEXT("test3"));
							}
							{
								static const FName fnKey(TEXT("key4"));
								TestEqual("Document->Root->Items[3].Key", root->Items[3].Key, fnKey);
								TestNotNull("Document->Root->Items[3].Value", root->Items[3].Value);
								const UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(root->Items[3].Value);
								TestNotNull("(UValveStringValue*)Document->Root->Items[3].Value", primValue);
								if (primValue == nullptr) { return; }
								TestEqual("Document->Root->Items[3].Value->Value", primValue->Value, TEXT(".3 .3 .3"));
							}
							{
								static const FName fnKey(TEXT("key5"));
								TestEqual("Document->Root->Items[4].Key", root->Items[4].Key, fnKey);
								TestNotNull("Document->Root->Items[4].Value", root->Items[4].Value);
								const UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(root->Items[4].Value);
								TestNotNull("(UValveStringValue*)Document->Root->Items[4].Value", primValue);
								if (primValue == nullptr) { return; }
								TestEqual("Document->Root->Items[4].Value->Value", primValue->Value, TEXT("test4"));
							}
							{
								static const FName fnKey(TEXT("key6"));
								TestEqual("Document->Root->Items[5].Key", root->Items[5].Key, fnKey);
								TestNotNull("Document->Root->Items[5].Value", root->Items[5].Value);
								const UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(root->Items[5].Value);
								TestNotNull("(UValveStringValue*)Document->Root->Items[5].Value", primValue);
								if (primValue == nullptr) { return; }
								TestEqual("Document->Root->Items[5].Value->Value", primValue->Value, TEXT("test5"));
							}
						});

					It("will parse groups as values", [this]()
						{
							Document = UValveDocument::Parse(TEXT("key1\n{\ninnerkey1 10\n}"));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const UValveGroupValue* root = Cast<UValveGroupValue>(Document->Root);
							TestNotNull("(UValveGroupValue*)Document->Root", root);
							if (root == nullptr) { return; }

							TestEqual("Document->Root->Items.Num()", root->Items.Num(), 1);
							if (root->Items.Num() < 1) { return; }

							static const FName fnKey(TEXT("key1"));
							TestEqual("Document->Root->Items[0].Key", root->Items[0].Key, fnKey);
							TestNotNull("Document->Root->Items[0].Value", root->Items[0].Value);
							const UValveGroupValue* groupValue = Cast<UValveGroupValue>(root->Items[0].Value);
							TestNotNull("(UValveGroupValue*)Document->Root->Items[0].Value", groupValue);
							if (groupValue == nullptr) { return; }

							TestEqual("Document->Root->Items[0].Value.Num()", groupValue->Items.Num(), 1);
							if (groupValue->Items.Num() < 1) { return; }

							static const FName fnInnerKey(TEXT("innerkey1"));
							TestEqual("Document->Root->Items[0].Value->Items[0].Key", groupValue->Items[0].Key, fnInnerKey);
							TestNotNull("Document->Root->Items[0].Value->Items[0].Value", groupValue->Items[0].Value);
							const UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(groupValue->Items[0].Value);
							TestNotNull("(UValveIntegerValue*)Document->Root->Items[0].Value->Items[0].Value", primValue);
							if (primValue == nullptr) { return; }
							TestEqual("Document->Root->Items[0].Value->Items[0].Value->Value", primValue->Value, FString(TEXT("10")));
							
						});

					It("will parse arrays as values", [this]()
						{
							Document = UValveDocument::Parse(TEXT("{ innerkey1 10 } { innerkey2 20 }"));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const UValveArrayValue* root = Cast<UValveArrayValue>(Document->Root);
							TestNotNull("(UValveArrayValue*)Document->Root", root);
							if (root == nullptr) { return; }

							TestEqual("Document->Root->Items.Num()", root->Items.Num(), 2);
							if (root->Items.Num() < 2) { return; }

							{
								const UValveGroupValue* groupValue = Cast<UValveGroupValue>(root->Items[0]);
								TestNotNull("(UValveGroupValue*)Document->Root->Items[0]", groupValue);
								if (groupValue == nullptr) { return; }

								TestEqual("Document->Root->Items[0].Num()", groupValue->Items.Num(), 1);
								if (groupValue->Items.Num() < 1) { return; }

								static const FName fnInnerKey(TEXT("innerkey1"));
								TestEqual("Document->Root->Items[0]->Items[0].Key", groupValue->Items[0].Key, fnInnerKey);
								TestNotNull("Document->Root->Items[0]->Items[0].Value", groupValue->Items[0].Value);
								const UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(groupValue->Items[0].Value);
								TestNotNull("(UValveIntegerValue*)Document->Root->Items[0]->Items[0].Value", primValue);
								if (primValue == nullptr) { return; }
								TestEqual("Document->Root->Items[0]->Items[0].Value->Value", primValue->Value, FString(TEXT("10")));
							}
							{
								const UValveGroupValue* groupValue = Cast<UValveGroupValue>(root->Items[1]);
								TestNotNull("(UValveGroupValue*)Document->Root->Items[1]", groupValue);
								if (groupValue == nullptr) { return; }

								TestEqual("Document->Root->Items[1].Num()", groupValue->Items.Num(), 1);
								if (groupValue->Items.Num() < 1) { return; }

								static const FName fnInnerKey(TEXT("innerkey2"));
								TestEqual("Document->Root->Items[1]->Items[0].Key", groupValue->Items[0].Key, fnInnerKey);
								TestNotNull("Document->Root->Items[1]->Items[0].Value", groupValue->Items[0].Value);
								const UValvePrimitiveValue* primValue = Cast<UValvePrimitiveValue>(groupValue->Items[0].Value);
								TestNotNull("(UValveIntegerValue*)Document->Root->Items[1]->Items[0].Value", primValue);
								if (primValue == nullptr) { return; }
								TestEqual("Document->Root->Items[1]->Items[0].Value->Value", primValue->Value, FString(TEXT("20")));
							}
						});

					It("will query primitive top-level values", [this]()
						{
							Document = UValveDocument::Parse(TEXT("\"intkey\" 10  \"floatkey\" 12.5  \"stringkey\" hello"));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const static FName fnIntKey(TEXT("intkey"));
							const static FName fnFloatKey(TEXT("floatkey"));
							const static FName fnStringKey(TEXT("stringkey"));
							const static FName fnBadKey(TEXT("badkey"));
							{
								int value;
								TestTrue("int:intkey", Document->GetInt(fnIntKey, value));
								TestEqual("int:intkey", value, 10);
								TestTrue("int:floatkey", Document->GetInt(fnFloatKey, value));
								TestEqual("int:floatkey", value, 12);
								TestTrue("int:stringkey", Document->GetInt(fnStringKey, value));
								TestEqual("int:floatkey", value, 0);
								TestFalse("int:badkey", Document->GetInt(fnBadKey, value));
							}
							{
								float value;
								TestTrue("float:intkey", Document->GetFloat(fnIntKey, value));
								TestEqual("float:intkey", value, 10.0f);
								TestTrue("float:floatkey", Document->GetFloat(fnFloatKey, value));
								TestEqual("float:floatkey", value, 12.5f);
								TestTrue("float:stringkey", Document->GetFloat(fnStringKey, value));
								TestEqual("float:floatkey", value, 0.0f);
								TestFalse("float:badkey", Document->GetFloat(fnBadKey, value));
							}
							{
								FString value;
								TestTrue("string:intkey", Document->GetString(fnIntKey, value));
								TestEqual("string:intkey", value, FString(TEXT("10")));
								TestTrue("string:floatkey", Document->GetString(fnFloatKey, value));
								TestEqual("string:floatkey", value, FString(TEXT("12.5")));
								TestTrue("string:stringkey", Document->GetString(fnStringKey, value));
								TestEqual("string:floatkey", value, FString(TEXT("hello")));
								TestFalse("string:badkey", Document->GetString(fnBadKey, value));
							}
							{
								FName value;
								TestTrue("name:intkey", Document->GetName(fnIntKey, value));
								TestEqual("name:intkey", value, FName(TEXT("10")));
								TestTrue("name:floatkey", Document->GetName(fnFloatKey, value));
								TestEqual("name:floatkey", value, FName(TEXT("12.5")));
								TestTrue("name:stringkey", Document->GetName(fnStringKey, value));
								TestEqual("name:floatkey", value, FName(TEXT("hello")));
								TestFalse("name:badkey", Document->GetName(fnBadKey, value));
							}
						});

					It("will query into key-value groups", [this]()
						{
							Document = UValveDocument::Parse(TEXT("group1 { \"intkey\" 10 }  group2 { \"intkey\" 20  \"innergroup\" { intkey 30 } }"));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const static FName fnGroup1Path(TEXT("group1.intkey"));
							const static FName fnGroup2Path(TEXT("group2.intkey"));
							const static FName fnInnerGroupPath(TEXT("group2.innergroup.intkey"));

							const static FName fnBadPath1(TEXT("group0"));
							const static FName fnBadPath2(TEXT("group1"));
							const static FName fnBadPath3(TEXT("group3.intkey"));

							int value;
							TestTrue("int:group1.intkey", Document->GetInt(fnGroup1Path, value));
							TestEqual("int:group1.intkey", value, 10);
							TestTrue("int:group2.intkey", Document->GetInt(fnGroup2Path, value));
							TestEqual("int:group2.intkey", value, 20);
							TestTrue("int:group2.innergroup.intkey", Document->GetInt(fnInnerGroupPath, value));
							TestEqual("int:group2.innergroup.intkey", value, 30);

							TestFalse("int:group0", Document->GetInt(fnBadPath1, value));
							TestFalse("int:group1", Document->GetInt(fnBadPath2, value));
							TestFalse("int:group3.intkey", Document->GetInt(fnBadPath3, value));
							
						});

					It("will query into array groups", [this]()
						{
							Document = UValveDocument::Parse(TEXT("{ intkey 10 }  { intkey 20 }"));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const static FName fnGroup1Path(TEXT("[0].intkey"));
							const static FName fnGroup2Path(TEXT("[1].intkey"));

							const static FName fnBadPath1(TEXT("[0]"));
							const static FName fnBadPath2(TEXT("[2].intkey"));
							const static FName fnBadPath3(TEXT("[-1].intkey"));

							int value;
							TestTrue("int:[0].intkey", Document->GetInt(fnGroup1Path, value));
							TestEqual("int:[0].intkey", value, 10);
							TestTrue("int:[1].intkey", Document->GetInt(fnGroup2Path, value));
							TestEqual("int:[1].intkey", value, 20);

							TestFalse("int:[0]", Document->GetInt(fnBadPath1, value));
							TestFalse("int:[2].intkey", Document->GetInt(fnBadPath2, value));
							TestFalse("int:[-1].intkey", Document->GetInt(fnBadPath3, value));

						});

					It("will query non-unique keys in key-value groups", [this]()
						{
							Document = UValveDocument::Parse(TEXT("intkey 10 \n intkey 20"));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const static FName fnGoodPath1(TEXT("intkey"));
							const static FName fnGoodPath2(TEXT("intkey[0]"));
							const static FName fnGoodPath3(TEXT("intkey[1]"));

							const static FName fnBadPath1(TEXT("intkey[-1]"));
							const static FName fnBadPath2(TEXT("intkey[2]"));
							const static FName fnBadPath3(TEXT("antkey[0]"));

							int value;
							TestTrue("int:intkey", Document->GetInt(fnGoodPath1, value));
							TestEqual("int:intkey", value, 10);
							TestTrue("int:intkey[0]", Document->GetInt(fnGoodPath2, value));
							TestEqual("int:intkey[0]", value, 10);
							TestTrue("int:intkey[1]", Document->GetInt(fnGoodPath3, value));
							TestEqual("int:intkey[1]", value, 20);

							TestFalse("int:intkey[-1]", Document->GetInt(fnBadPath1, value));
							TestFalse("int:intkey[2]", Document->GetInt(fnBadPath2, value));
							TestFalse("int:antkey[0]", Document->GetInt(fnBadPath3, value));

						});

					It("will query into 2D array groups", [this]()
						{
							Document = UValveDocument::Parse(TEXT("{ { intkey 10 }  { intkey 20 } }  { { intkey 30 } }"));
							TestNotNull("Document", Document);
							if (Document == nullptr) { return; }
							TestNotNull("Document->Root", Document->Root);
							if (Document->Root == nullptr) { return; }

							const static FName fnGoodPath1(TEXT("[0][0].intkey"));
							const static FName fnGoodPath2(TEXT("[0][1].intkey"));
							const static FName fnGoodPath3(TEXT("[1][0].intkey"));

							const static FName fnBadPath1(TEXT("[0]"));
							const static FName fnBadPath2(TEXT("[0].intkey"));
							const static FName fnBadPath3(TEXT("[0][2].intkey"));
							const static FName fnBadPath4(TEXT("[1][1].intkey"));

							int value;
							TestTrue("int:[0][0].intkey", Document->GetInt(fnGoodPath1, value));
							TestEqual("int:[0][0].intkey", value, 10);
							TestTrue("int:[0][1].intkey", Document->GetInt(fnGoodPath2, value));
							TestEqual("int:[0][1].intkey", value, 20);
							TestTrue("int:[1][0].intkey", Document->GetInt(fnGoodPath3, value));
							TestEqual("int:[1][0].intkey", value, 30);

							TestFalse("int:[0]", Document->GetInt(fnBadPath1, value));
							TestFalse("int:[0].intkey", Document->GetInt(fnBadPath2, value));
							TestFalse("int:[0][2].intkey", Document->GetInt(fnBadPath3, value));
							TestFalse("int:[1][1].intkey", Document->GetInt(fnBadPath4, value));

						});

					AfterEach([this]()
						{
							if (Document != nullptr)
							{
								Document->MarkAsGarbage();
								Document = nullptr;
							}
						});
				});
		});
}
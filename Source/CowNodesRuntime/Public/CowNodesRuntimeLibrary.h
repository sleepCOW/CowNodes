// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CowNodesRuntimeLibrary.generated.h"

/**
 * 
 */
UCLASS()
class COWNODESRUNTIME_API UCowNodesRuntimeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(BlueprintInternalUseOnly = "true", ArrayParm = "TargetArray", ArrayTypeDependentParams = "Item", BlueprintThreadSafe), Category="Utilities|Array")
	static void Array_FastForEach(const TArray<int32>& TargetArray, int32& Item);
	DECLARE_FUNCTION(execArray_FastForEach)
	{
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
		if (!ArrayProperty)
		{
			Stack.bArrayContextFailed = true;
			return;
		}
		P_GET_PROPERTY(FIntProperty, Index);

		// Since Item isn't really an int, step the stack manually
		const FProperty* InnerProp = ArrayProperty->Inner;
		const int32 PropertySize = InnerProp->GetElementSize() * InnerProp->ArrayDim;
		void* StorageSpace = FMemory_Alloca(PropertySize);
		InnerProp->InitializeValue(StorageSpace);

		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentPropertyContainer = nullptr;
		Stack.StepCompiledIn<FProperty>(StorageSpace);
		const FFieldClass* InnerPropClass = InnerProp->GetClass();
		const FFieldClass* MostRecentPropClass = Stack.MostRecentProperty->GetClass();
		void* ItemPtr;
		// If the destination and the inner type are identical in size and their field classes derive from one another, then permit the writing out of the array element to the destination memory
		if (Stack.MostRecentPropertyAddress != NULL && (PropertySize == Stack.MostRecentProperty->GetElementSize()*Stack.MostRecentProperty->ArrayDim) &&
			(MostRecentPropClass->IsChildOf(InnerPropClass) || InnerPropClass->IsChildOf(MostRecentPropClass)))
		{
			ItemPtr = Stack.MostRecentPropertyAddress;
		}
		else
		{
			ItemPtr = StorageSpace;
		}

		P_FINISH;
		P_NATIVE_BEGIN;
		//GenericArray_Get(ArrayAddr, ArrayProperty, Index, ItemPtr);
		P_NATIVE_END;
		InnerProp->DestroyValue(StorageSpace);
	}
};

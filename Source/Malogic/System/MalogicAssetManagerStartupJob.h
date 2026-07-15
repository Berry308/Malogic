// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/StreamableManager.h"

DECLARE_DELEGATE_OneParam(FMalogicAssetManagerStartupJobSubstepProgress, float /* NewProgress */);

/** A startup loading task with optional streamable-handle progress reporting. */
struct FMalogicAssetManagerStartupJob
{
	FMalogicAssetManagerStartupJobSubstepProgress SubstepProgressDelegate;
	TFunction<void(const FMalogicAssetManagerStartupJob&, TSharedPtr<FStreamableHandle>&)> JobFunc;
	FString JobName;
	float JobWeight;
	mutable double LastUpdateTime = 0.0;

	FMalogicAssetManagerStartupJob(
		const FString& InJobName,
		const TFunction<void(const FMalogicAssetManagerStartupJob&, TSharedPtr<FStreamableHandle>&)>& InJobFunc,
		const float InJobWeight)
		: JobFunc(InJobFunc)
		, JobName(InJobName)
		, JobWeight(InJobWeight)
	{
	}

	TSharedPtr<FStreamableHandle> DoJob() const;

	void UpdateSubstepProgress(const float NewProgress) const
	{
		SubstepProgressDelegate.ExecuteIfBound(NewProgress);
	}

	void UpdateSubstepProgressFromStreamable(TSharedRef<FStreamableHandle> StreamableHandle) const;
};

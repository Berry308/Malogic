// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/MalogicAssetManagerStartupJob.h"

#include "MalogicLogChannels.h"

TSharedPtr<FStreamableHandle> FMalogicAssetManagerStartupJob::DoJob() const
{
	const double JobStartTime = FPlatformTime::Seconds();

	TSharedPtr<FStreamableHandle> Handle;
	UE_LOG(LogMalogic, Display, TEXT("Asset manager startup job \"%s\" starting"), *JobName);
	JobFunc(*this, Handle);

	if (Handle.IsValid())
	{
		Handle->BindUpdateDelegate(FStreamableUpdateDelegate::CreateRaw(this, &FMalogicAssetManagerStartupJob::UpdateSubstepProgressFromStreamable));
		Handle->WaitUntilComplete(0.0f, false);
		Handle->BindUpdateDelegate(FStreamableUpdateDelegate());
	}

	UE_LOG(LogMalogic, Display, TEXT("Asset manager startup job \"%s\" completed in %.2f seconds"), *JobName, FPlatformTime::Seconds() - JobStartTime);
	return Handle;
}

void FMalogicAssetManagerStartupJob::UpdateSubstepProgressFromStreamable(TSharedRef<FStreamableHandle> StreamableHandle) const
{
	if (SubstepProgressDelegate.IsBound())
	{
		const double CurrentTime = FPlatformTime::Seconds();
		if (CurrentTime - LastUpdateTime >= 1.0 / 60.0)
		{
			SubstepProgressDelegate.Execute(StreamableHandle->GetProgress());
			LastUpdateTime = CurrentTime;
		}
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/MalogicAssetManager.h"

#include "Engine/Engine.h"
#include "MalogicLogChannels.h"
#include "Misc/App.h"
#include "Misc/ScopedSlowTask.h"
#include "Stats/StatsMisc.h"
#include "System/MalogicGameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicAssetManager)

#define MALOGIC_STARTUP_JOB_WEIGHTED(JobFunction, JobWeight) \
	StartupJobs.Emplace(TEXT(#JobFunction), [this](const FMalogicAssetManagerStartupJob&, TSharedPtr<FStreamableHandle>&) { JobFunction; }, JobWeight)

UMalogicAssetManager::UMalogicAssetManager() = default;

UMalogicAssetManager& UMalogicAssetManager::Get()
{
	check(GEngine);

	if (UMalogicAssetManager* Singleton = Cast<UMalogicAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogMalogic, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini. It must be set to /Script/Malogic.MalogicAssetManager."));
	return *NewObject<UMalogicAssetManager>();
}

UObject* UMalogicAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (!AssetPath.IsValid())
	{
		return nullptr;
	}

	TUniquePtr<FScopeLogTime> LogTime;
	if (ShouldLogAssetLoads())
	{
		LogTime = MakeUnique<FScopeLogTime>(*FString::Printf(TEXT("Synchronously loaded asset [%s]"), *AssetPath.ToString()), nullptr, FScopeLogTime::ScopeLog_Seconds);
	}

	if (UAssetManager::IsInitialized())
	{
		return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
	}

	return AssetPath.TryLoad();
}

bool UMalogicAssetManager::ShouldLogAssetLoads()
{
	static const bool bLogAssetLoads = FParse::Param(FCommandLine::Get(), TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}

void UMalogicAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		//FScopeLock 是 RAII 写法：创建时加锁，离开当前 {} 作用域时自动解锁，即使中间提前 return 也不会遗留死锁。
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

void UMalogicAssetManager::DumpLoadedAssets()
{
	UMalogicAssetManager& AssetManager = Get();

	FScopeLock LoadedAssetsLock(&AssetManager.LoadedAssetsCritical);

	UE_LOG(LogMalogic, Log, TEXT("========== Start Dumping Asset Manager Loaded Assets =========="));
	for (const UObject* LoadedAsset : AssetManager.LoadedAssets)
	{
		UE_LOG(LogMalogic, Log, TEXT("  %s"), *GetNameSafe(LoadedAsset));
	}
	UE_LOG(LogMalogic, Log, TEXT("... %d assets in loaded pool"), AssetManager.LoadedAssets.Num());
	UE_LOG(LogMalogic, Log, TEXT("========== Finish Dumping Asset Manager Loaded Assets =========="));
}

void UMalogicAssetManager::StartInitialLoading()
{
	SCOPED_BOOT_TIMING("UMalogicAssetManager::StartInitialLoading");
	Super::StartInitialLoading();

	if (MalogicGameDataPath.IsNull())
	{
		UE_LOG(LogMalogic, Warning, TEXT("MalogicGameDataPath is not configured. Global game data will be loaded on demand after it is configured."));
	}
	else
	{
		MALOGIC_STARTUP_JOB_WEIGHTED(GetGameData(), 25.0f);
	}

	DoAllStartupJobs();
}

const UMalogicGameData& UMalogicAssetManager::GetGameData()
{
	return GetOrLoadTypedGameData<UMalogicGameData>(MalogicGameDataPath);
}

UPrimaryDataAsset* UMalogicAssetManager::LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	UPrimaryDataAsset* Asset = nullptr;

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading GameData Object"), STAT_GameData, STATGROUP_LoadTime);
	if (!DataClassPath.IsNull())
	{
#if WITH_EDITOR
		FScopedSlowTask SlowTask(0, FText::Format(NSLOCTEXT("MalogicEditor", "BeginLoadingGameDataTask", "Loading GameData {0}"), FText::FromName(DataClass->GetFName())));
		SlowTask.MakeDialog(false, true);
#endif
		UE_LOG(LogMalogic, Log, TEXT("Loading GameData: %s ..."), *DataClassPath.ToString());
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... GameData loaded!"), nullptr);

		if (GIsEditor)
		{
			Asset = DataClassPath.LoadSynchronous();
			LoadPrimaryAssetsWithType(PrimaryAssetType);
		}
		else if (TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType))
		{
			Handle->WaitUntilComplete(0.0f, false);
			Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
		}
	}

	if (Asset)
	{
		GameDataMap.Add(DataClass, Asset);
	}
	else
	{
		UE_LOG(LogMalogic, Fatal, TEXT("Failed to load GameData asset at %s. Type %s. This is not recoverable and likely means the project data is incomplete for %s."), *DataClassPath.ToString(), *PrimaryAssetType.ToString(), FApp::GetProjectName());
	}

	return Asset;
}

void UMalogicAssetManager::DoAllStartupJobs()
{
	SCOPED_BOOT_TIMING("UMalogicAssetManager::DoAllStartupJobs");
	const double StartTime = FPlatformTime::Seconds();

	if (IsRunningDedicatedServer())
	{
		for (const FMalogicAssetManagerStartupJob& StartupJob : StartupJobs)
		{
			StartupJob.DoJob();
		}
	}
	else if (StartupJobs.Num() > 0)
	{
		float TotalWeight = 0.0f;
		for (const FMalogicAssetManagerStartupJob& StartupJob : StartupJobs)
		{
			TotalWeight += StartupJob.JobWeight;
		}

		float CompletedWeight = 0.0f;
		for (FMalogicAssetManagerStartupJob& StartupJob : StartupJobs)
		{
			const float JobWeight = StartupJob.JobWeight;
			StartupJob.SubstepProgressDelegate.BindLambda([this, CompletedWeight, JobWeight, TotalWeight](const float Progress)
			{
				UpdateInitialGameContentLoadPercent((CompletedWeight + FMath::Clamp(Progress, 0.0f, 1.0f) * JobWeight) / TotalWeight);
			});

			StartupJob.DoJob();
			StartupJob.SubstepProgressDelegate.Unbind();
			CompletedWeight += JobWeight;
			UpdateInitialGameContentLoadPercent(CompletedWeight / TotalWeight);
		}
	}
	else
	{
		UpdateInitialGameContentLoadPercent(1.0f);
	}

	StartupJobs.Empty();
	UE_LOG(LogMalogic, Display, TEXT("All asset manager startup jobs completed in %.2f seconds"), FPlatformTime::Seconds() - StartTime);
}

void UMalogicAssetManager::UpdateInitialGameContentLoadPercent(float GameContentPercent)
{
	// Reserved for an early startup loading screen integration.
}

#if WITH_EDITOR
void UMalogicAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);

	if (!MalogicGameDataPath.IsNull())
	{
		FScopedSlowTask SlowTask(0, NSLOCTEXT("MalogicEditor", "BeginLoadingPIEData", "Loading PIE Data"));
		SlowTask.MakeDialog(false, true);
		GetGameData();
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("PreBeginPIE asset preloading complete"), nullptr);
	}
}
#endif

#undef MALOGIC_STARTUP_JOB_WEIGHTED

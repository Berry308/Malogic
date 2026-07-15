// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/AssetManager.h"
#include "System/MalogicAssetManagerStartupJob.h"
#include "Templates/SubclassOf.h"

#include "MalogicAssetManager.generated.h"

class UMalogicGameData;
class UPrimaryDataAsset;

/**
 * Project asset manager responsible for early game-data loading and explicit soft-reference loads.
 * Configure this class through AssetManagerClassName in DefaultEngine.ini.
 */
UCLASS(Config = Game)
class MALOGIC_API UMalogicAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	UMalogicAssetManager();

	static UMalogicAssetManager& Get();

	// Synchronously resolves a soft object reference and optionally keeps it resident.
	template<typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	// Synchronously resolves a soft class reference and optionally keeps it resident.
	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	//将所有已加载的资源打印到日志中，供调试使用。
	static void DumpLoadedAssets();

	const UMalogicGameData& GetGameData();

protected:
	template <typename GameDataClass>
	const GameDataClass& GetOrLoadTypedGameData(const TSoftObjectPtr<GameDataClass>& DataPath)
	{
		if (TObjectPtr<UPrimaryDataAsset> const* Result = GameDataMap.Find(GameDataClass::StaticClass()))
		{
			return *CastChecked<GameDataClass>(*Result);
		}

		return *CastChecked<const GameDataClass>(LoadGameDataOfClass(GameDataClass::StaticClass(), DataPath, GameDataClass::StaticClass()->GetFName()));
	}

	//同步加载
	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);
	static bool ShouldLogAssetLoads();

	void AddLoadedAsset(const UObject* Asset);

	virtual void StartInitialLoading() override;

#if WITH_EDITOR
	virtual void PreBeginPIE(bool bStartSimulate) override;
#endif

	UPrimaryDataAsset* LoadGameDataOfClass(
		TSubclassOf<UPrimaryDataAsset> DataClass,
		const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath,
		FPrimaryAssetType PrimaryAssetType);

	UPROPERTY(Config)
	TSoftObjectPtr<UMalogicGameData> MalogicGameDataPath;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap;

private:
	void DoAllStartupJobs();
	void UpdateInitialGameContentLoadPercent(float GameContentPercent);

	TArray<FMalogicAssetManagerStartupJob> StartupJobs;

	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;
	//FCriticalSection LoadedAssetsCritical 是互斥锁，用于保护 LoadedAssets 集合的并发访问。
	FCriticalSection LoadedAssetsCritical;
};

template<typename AssetType>
AssetType* UMalogicAssetManager::GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;
	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (!LoadedAsset)
		{
			LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
		}

		if (LoadedAsset && bKeepInMemory)
		{
			Get().AddLoadedAsset(LoadedAsset);
		}
	}

	return LoadedAsset;
}

template<typename AssetType>
TSubclassOf<AssetType> UMalogicAssetManager::GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;
	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (!LoadedSubclass)
		{
			LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedSubclass, TEXT("Failed to load asset class [%s]"), *AssetPointer.ToString());
		}

		if (LoadedSubclass && bKeepInMemory)
		{
			Get().AddLoadedAsset(LoadedSubclass.Get());
		}
	}

	return LoadedSubclass;
}

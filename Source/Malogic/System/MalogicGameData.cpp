// Fill out your copyright notice in the Description page of Project Settings.


#include "MalogicGameData.h"
#include "MalogicAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicGameData)

UMalogicGameData::UMalogicGameData()
{
}

const UMalogicGameData& UMalogicGameData::Get()
{
	return UMalogicAssetManager::Get().GetGameData();
}

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MalogicLogChannels.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "MalogicMessageSubsystem.generated.h"

/** A stable identifier for a listener registered with UMalogicMessageSubsystem. */
USTRUCT(BlueprintType)
struct MALOGIC_API FMalogicMessageListenerHandle
{
	GENERATED_BODY()

public:
	FMalogicMessageListenerHandle() = default;

	bool IsValid() const { return HandleId != 0; }

	void Reset() { HandleId = 0; }

	friend bool operator==(const FMalogicMessageListenerHandle& Left, const FMalogicMessageListenerHandle& Right)
	{
		return Left.HandleId == Right.HandleId;
	}

	friend bool operator!=(const FMalogicMessageListenerHandle& Left, const FMalogicMessageListenerHandle& Right)
	{
		return !(Left == Right);
	}

private:
	friend class UMalogicMessageSubsystem;

	explicit FMalogicMessageListenerHandle(int64 InHandleId)
		: HandleId(InHandleId)
	{
	}

	UPROPERTY()
	int64 HandleId = 0;
};

/** Non-reflected base used to store listeners with different payload types together. */
struct MALOGIC_API FMalogicMessageListenerBase
{
	FMalogicMessageListenerBase(int64 InHandleId, UObject* InListenerOwner)
		: HandleId(InHandleId)
		, ListenerOwner(InListenerOwner)
	{
	}

	virtual ~FMalogicMessageListenerBase() = default;

	UScriptStruct* GetPayloadStruct() const { return PayloadStruct; }
	int64 GetHandleId() const { return HandleId; }
	bool IsRegistered() const { return bRegistered; }
	bool IsInvocationAllowed() const { return bRegistered && ListenerOwner.IsValid(); }

	void Deactivate() { bRegistered = false; }

	virtual void Invoke(FGameplayTag Channel, const void* Payload) = 0;

protected:
	void SetPayloadStruct(UScriptStruct* InPayloadStruct) { PayloadStruct = InPayloadStruct; }

private:
	int64 HandleId = 0;
	TWeakObjectPtr<UObject> ListenerOwner;
	TObjectPtr<UScriptStruct> PayloadStruct = nullptr;
	bool bRegistered = true;
};

template <typename T>
struct TMalogicMessageListener final : FMalogicMessageListenerBase
{
	TMalogicMessageListener(
		int64 InHandleId,
		UObject* InListenerOwner,
		TFunction<void(FGameplayTag, const T&)> InCallback)
		: FMalogicMessageListenerBase(InHandleId, InListenerOwner)
		, Callback(MoveTemp(InCallback))
	{
		SetPayloadStruct(T::StaticStruct());
	}

	virtual void Invoke(FGameplayTag Channel, const void* Payload) override
	{
		if (Callback && Payload)
		{
			Callback(Channel, *static_cast<const T*>(Payload));
		}
	}

private:
	TFunction<void(FGameplayTag, const T&)> Callback;
};

/** Local, synchronous publish/subscribe router for gameplay modules. */
UCLASS()
class MALOGIC_API UMalogicMessageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	template <typename T>
	void BroadcastMessage(FGameplayTag Channel, const T& Payload)
	{
		if (!Channel.IsValid())
		{
			UE_LOG(LogMalogic, Warning, TEXT("Message broadcast rejected because the channel is invalid."));
			return;
		}

		const TArray<TSharedPtr<FMalogicMessageListenerBase>>* ChannelListeners = ListenerMap.Find(Channel);
		if (!ChannelListeners)
		{
			return;
		}

		// Invoke a snapshot so callbacks may register or unregister listeners safely.
		const TArray<TSharedPtr<FMalogicMessageListenerBase>> ListenerSnapshot = *ChannelListeners;
		UScriptStruct* PayloadStruct = T::StaticStruct();
		for (const TSharedPtr<FMalogicMessageListenerBase>& Listener : ListenerSnapshot)
		{
			if (Listener.IsValid()
				&& Listener->IsInvocationAllowed()
				&& Listener->GetPayloadStruct() == PayloadStruct)
			{
				Listener->Invoke(Channel, &Payload);
			}
		}
	}

	template <typename T>
	FMalogicMessageListenerHandle RegisterListener(
		FGameplayTag Channel,
		UObject* ListenerOwner,
		TFunction<void(FGameplayTag, const T&)> InCallback)
	{
		if (!Channel.IsValid() || !IsValid(ListenerOwner) || !InCallback)
		{
			UE_LOG(LogMalogic, Warning, TEXT("Message listener registration rejected for channel [%s]."), *Channel.ToString());
			return FMalogicMessageListenerHandle();
		}

		const int64 NewHandleId = NextHandleId++;
		//通过基类结构体封装不同的回调函数
		TSharedPtr<FMalogicMessageListenerBase> NewListener = MakeShared<TMalogicMessageListener<T>>(
			NewHandleId,
			ListenerOwner,
			MoveTemp(InCallback));
		ListenerMap.FindOrAdd(Channel).Add(MoveTemp(NewListener));
		return FMalogicMessageListenerHandle(NewHandleId);
	}

	/** Removes a listener. Safe to call more than once with the same handle. */
	bool UnregisterListener(FMalogicMessageListenerHandle& Handle);

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TMap<FGameplayTag, TArray<TSharedPtr<FMalogicMessageListenerBase>>> ListenerMap;
	int64 NextHandleId = 1;
};

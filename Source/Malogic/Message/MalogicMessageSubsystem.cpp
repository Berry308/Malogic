#include "Message/MalogicMessageSubsystem.h"

#include "MalogicLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicMessageSubsystem)

void UMalogicMessageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	NextHandleId = 1;
}

void UMalogicMessageSubsystem::Deinitialize()
{
	for (TPair<FGameplayTag, TArray<TSharedPtr<FMalogicMessageListenerBase>>>& ChannelEntry : ListenerMap)
	{
		for (const TSharedPtr<FMalogicMessageListenerBase>& Listener : ChannelEntry.Value)
		{
			if (Listener.IsValid())
			{
				Listener->Deactivate();
			}
		}
	}

	ListenerMap.Reset();
	NextHandleId = 1;
	Super::Deinitialize();
}

bool UMalogicMessageSubsystem::UnregisterListener(FMalogicMessageListenerHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	const int64 HandleId = Handle.HandleId;
	bool bRemoved = false;
	for (auto ChannelIt = ListenerMap.CreateIterator(); ChannelIt; ++ChannelIt)
	{
		TArray<TSharedPtr<FMalogicMessageListenerBase>>& Listeners = ChannelIt.Value();
		const int32 RemovedCount = Listeners.RemoveAll(
			[HandleId](const TSharedPtr<FMalogicMessageListenerBase>& Listener)
			{
				if (Listener.IsValid() && Listener->GetHandleId() == HandleId)
				{
					Listener->Deactivate();
					return true;
				}
				return false;
			});

		if (RemovedCount > 0)
		{
			bRemoved = true;
			if (Listeners.Num() == 0)
			{
				ChannelIt.RemoveCurrent();
			}
			break;
		}
	}

	Handle.Reset();
	return bRemoved;
}

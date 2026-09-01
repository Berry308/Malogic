# 提问一：说说Lyra的Message系统
在 Lyra Starter Game 中，Gameplay Message 系统（由 GameplayMessageRouter 插件提供）是一个高度解耦的**发布/订阅（Pub-Sub）**通讯框架。
它是 Lyra 架构的核心支柱之一，主要用于解决传统“委托（Delegates）”或“接口（Interfaces）”在大型逻辑中导致的强耦合问题。例如：UI 需要知道玩家什么时候完成了一次“淘汰（Elimination）”，在传统做法中，UI 必须引用玩家类；而在 Lyra 中，UI 只需要监听一个特定的频道即可。
以下是该系统的深度解析：
1. 核心概念
该系统基于 UGameplayMessageSubsystem（一个 UGameInstanceSubsystem）运行，包含三个核心元素：

频道（Channel）： 使用 Gameplay Tags 标识。例如 Lyra.Elimination.Message。
载荷（Payload）： 使用普通的 原生 C++ 结构体（USTRUCT）。
解耦性： 发送者（Sender）不知道谁在听，接收者（Listener）不知道谁在发。

2. C++ 实战用法
定义消息载荷
所有的消息必须定义为 USTRUCT。Lyra 经常使用 FLyraVerbMessage 来传递包含“谁对谁做了什么”的消息：
// 示例：自定义经验值获取消息
USTRUCT(BlueprintType)
struct FMyPlayerXpMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<APlayerState> Player = nullptr;

    UPROPERTY(BlueprintReadWrite)
    int32 XpGained = 0;
};
广播消息（发送）
在 C++ 中，通过子系统调用 BroadcastMessage：
#include "GameInstance/GameplayMessageSubsystem.h"

void AMyPlayerState::AddExperience(int32 Amount)
{
    UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
    
    FMyPlayerXpMessage Message;
    Message.Player = this;
    Message.XpGained = Amount;

    FGameplayTag Channel = FGameplayTag::RequestGameplayTag("MyGame.Event.XpGained");
    MessageSubsystem.BroadcastMessage(Channel, Message);
}
监听消息（接收）
监听会返回一个 FGameplayMessageListenerHandle，必须妥善保存以进行销毁时的清理：
// 在监听者类中（如 UI 逻辑）
FGameplayMessageListenerHandle ListenerHandle;

void UMyXpWidget::StartListening()
{
    UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
    
    // 注册监听器
    ListenerHandle = MessageSubsystem.RegisterListener<FMyPlayerXpMessage>(
        FGameplayTag::RequestGameplayTag("MyGame.Event.XpGained"),
        this,
        &UMyXpWidget::OnXpReceived
    );
}

void UMyXpWidget::OnXpReceived(FGameplayTag Channel, const FMyPlayerXpMessage& Payload)
{
    // 处理逻辑：例如更新 UI 上的经验条
}
3. 蓝图用法
Lyra 为蓝图提供了非常直观的节点：

Broadcast Message： 发送消息。需要选择 Message Type 和 Channel。
Register Gameplay Message Observer： 这是一个异步节点。当指定的频道有消息时，会触发其输出执行线，并提供 Payload 数据。

4. 为什么 Lyra 使用它？

消除强引用： Combat 模块（战斗）不需要知道 UI 模块的存在。战斗只管广播“有人被 eliminated 了”，UI 自动拦截并显示击杀反馈。
模块化： 方便将功能拆分为不同的 GameFeature 插件。插件可以监听主模块的消息而不需要主模块对其有任何感知。
性能优化： 相比于在 Tick 中不断检查状态，基于事件的消息系统仅在必要时执行代码。
多人游戏友好： 虽然该系统本身是在本地运行的，但 Lyra 经常在服务器接收到同步属性变化后，在本地触发 Message 以驱动 UI 和特效表现。

5. 最佳实践与注意事项

生命周期管理： 在 C++ 中，务必在 EndPlay 或 BeginDestroy 中调用 UnregisterListener，否则会导致内存泄漏或野指针崩溃。
避免高频 Tick 事件： 消息系统存在一定的反射和标签查找开销，不建议用于每帧触发数千次的物理同步。
命名规范： 建议 Gameplay Tags 采用 Message.Category.EventName 的层级结构，便于在编辑器中过滤。

如果你正在开发类似 Lyra 的模块化项目，GameplayMessageRouter 是实现业务逻辑与表现层分离的最优选工具。

# 提问二：我想知道该系统更底层的实现，我暂时不想引入自己未完全掌握的系统，所以可能会根据它的思想考虑自己构建一个较简单的架构
理解 Lyra 的 Gameplay Message 系统（GameplayMessageRouter 插件）是掌握 Unreal Engine 模块化架构的重要一步。
如果你想根据其思想构建一个较简单的架构，你需要掌握三个核心底层技术：Gameplay Tags（地址寻址）、UScriptStruct（泛型载荷）以及类型抹除（Type Erasure，用于存储泛型回调）。
以下是该系统的深度逻辑剖析及构建建议：
1. 底层逻辑剖析
A. 寻址系统：Gameplay Tags
Lyra 不使用字符串或枚举，而是使用 FGameplayTag 作为“频道”。

优势： 标签是全局唯一的，支持层级结构（如 Message.Combat.Elimination），且底层是 FName 和整数 ID，比字符串比较快得多。
设计思路： 所有的通讯都指向一个 Tag，而不是指向某个具体的类。

B. 泛型载荷：UScriptStruct
这是该系统最精妙的地方。它利用 Unreal 的反射系统将任何 USTRUCT 视为数据块。

核心原理： USTRUCT 在 C++ 层面没有共同的基类，但在反射层面，它们都可以被描述为 UScriptStruct。
广播流程： 发送端传入一个结构体实例，系统通过模板 T::StaticStruct() 获取其元数据，并在内部以 void*（原始指针）的形式传递。

C. 类型抹除：回调函数的存储
系统需要在一个 TMap 中存储不同类型的回调函数。但 C++ 模板要求类型明确，如何在同一个容器里存 void(FMyA) 和 void(FMyB)？

做法： 定义一个抽象基类 FMessageListenerBase，然后创建一个派生模板类 TMessageListener<T>。
容器定义： TMap<FGameplayTag, TArray<TSharedPtr<FMessageListenerBase>>>。

2. 构建“简版”系统的代码框架
为了方便你理解并复刻，我为你写了一个简化版的“Lite 路由系统”。它保留了 Lyra 的核心解耦思想，但去掉了复杂的插件依赖和蓝图动态转换逻辑。
头文件 (LiteMessageSubsystem.h)
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LiteMessageSubsystem.generated.h"

// 1. 内部使用的抽象基类，用于抹除模板类型
struct FLiteMessageListenerBase
{
    virtual ~FLiteMessageListenerBase() {}
    virtual UScriptStruct* GetStructType() const = 0;
    virtual void Invoke(const void* PayloadPtr) = 0;
};

// 2. 存储具体回调的模板类
template<typename T>
struct TLiteMessageListener : public FLiteMessageListenerBase
{
    TFunction<void(const T&)> Callback;

    virtual UScriptStruct* GetStructType() const override { return T::StaticStruct(); }
    virtual void Invoke(const void* PayloadPtr) override 
    {
        // 关键点：将 void* 强转回原始结构体类型
        Callback(*reinterpret_cast<const T*>(PayloadPtr));
    }
};

UCLASS()
class ULiteMessageSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // 广播消息：利用泛型推导类型
    template<typename T>
    void BroadcastMessage(FGameplayTag Channel, const T& Payload)
    {
        if (auto* Listeners = ListenerMap.Find(Channel))
        {
            UScriptStruct* ExpectedType = T::StaticStruct();
            for (auto& Listener : *Listeners)
            {
                // 类型检查：确保监听者想要的类型与广播的类型一致
                if (Listener->GetStructType() == ExpectedType)
                {
                    Listener->Invoke(&Payload);
                }
            }
        }
    }

    // 注册监听
    template<typename T>
    void RegisterListener(FGameplayTag Channel, TFunction<void(const T&)> InCallback)
    {
        TSharedRef<TLiteMessageListener<T>> NewListener = MakeShared<TLiteMessageListener<T>>();
        NewListener->Callback = InCallback;
        
        ListenerMap.FindOrAdd(Channel).Add(NewListener);
    }

private:
    // 使用 Tag 映射一组抹除了类型的监听器
    TMap<FGameplayTag, TArray<TSharedPtr<FLiteMessageListenerBase>>> ListenerMap;
};

3. 构建自己架构的 3 条核心建议
如果你根据此思想构建系统，请务必处理以下 Lyra 解决得很好但在上述简化版中忽略的细节：
1. 句柄管理（Handles）
上面的简化版没有注销机制。

建议： 引入一个 FMessageListenerHandle（内部存一个递增的 int64）。Register 时返回 Handle，Unregister 时根据 ID 从 Map 中删除对应的 Listener。

2. 广播安全性
如果在消息回调函数里又广播了一个新消息，可能会导致迭代器失效或无限递归。

建议： 在广播开始前，先通过 TArray 拷贝一份当前频道的监听者列表，然后再循环执行回调，这样即使回调里有“注册/注销”操作也不会导致奔溃。

3. 避免“Elimination”循环引用
在 Lyra 这种多人游戏中，Elimination（淘汰）事件通常由服务器计算，然后通过此系统在本地 UI 展示。

建议： 你的架构应该明确哪些消息是 Server-Only，哪些是 Client-Only。Lyra 的消息系统是不跨网的（Non-replicated），它只负责本地各模块（如 UI 和战斗逻辑）的解耦。

总结
Lyra 消息系统的本质是一个以 Gameplay Tag 为索引的泛型回调仓库。
如果你只需在 C++ 中使用，上述 TMap + ListenerBase 方案就是最高效的实现；如果你需要蓝图支持，则需要使用 Unreal 的 CustomThunk 和 Stack.StepCompiledIn 来处理蓝图的通配符节点，那会显著增加复杂度。建议先从 C++ 版本入手。
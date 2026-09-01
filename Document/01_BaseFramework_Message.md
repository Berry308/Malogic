# Malogic Message System

## 文档定位

本文只总结当前已经实现的 Message 系统代码，不描述伤害数字 UI、GameplayCue、MVVM、Widget 管理或其它业务系统。

当前实现位于：

```text
Source/Malogic/Message/MalogicMessageSubsystem.h
Source/Malogic/Message/MalogicMessageSubsystem.cpp
```

它是一个运行在本地进程内、同步执行的 C++ 发布/订阅（Publish/Subscribe）路由器。发布者和监听者通过 Gameplay Tag 频道解耦：发布者只需要提供频道和消息载荷，监听者只需要订阅频道并处理指定类型的载荷。

当前版本不负责网络复制，不提供 Blueprint 通配消息节点，也不包含任何具体业务消息结构体。

## 文件职责

### `MalogicMessageSubsystem.h`

头文件定义：

- `FMalogicMessageListenerHandle`：监听器句柄；
- `FMalogicMessageListenerBase`：类型擦除监听器的非反射基类；
- `TMalogicMessageListener<T>`：保存具体消息类型和回调的模板监听器；
- `UMalogicMessageSubsystem`：消息注册、广播和注销接口，以及内部监听器容器。

模板函数 `RegisterListener` 和 `BroadcastMessage` 必须在头文件中定义，因为调用方需要在编译期实例化具体的消息类型 `T`。

### `MalogicMessageSubsystem.cpp`

实现非模板的 Subsystem 生命周期和监听器注销逻辑：

- `Initialize` 初始化句柄 ID；
- `Deinitialize` 停用所有监听器并清空容器；
- `UnregisterListener` 按句柄删除监听器。

## `UMalogicMessageSubsystem`

```cpp
UCLASS()
class MALOGIC_API UMalogicMessageSubsystem : public UGameInstanceSubsystem
```

Subsystem 的实例由 Unreal 按 GameInstance 创建和管理。它只维护消息监听器，不保存消息历史，不保存业务状态，也不持有业务 Actor、Widget 或 ViewModel。

内部数据结构为：

```cpp
TMap<FGameplayTag, TArray<TSharedPtr<FMalogicMessageListenerBase>>> ListenerMap;
int64 NextHandleId = 1;
```

其中：

- `FGameplayTag` 是消息频道；
- 一个频道可以有多个监听器；
- `TSharedPtr<FMalogicMessageListenerBase>` 让不同消息类型的监听器可以放入同一个数组；
- `NextHandleId` 为注册的新监听器生成 ID。

频道使用精确匹配。广播 `Message.Combat.Damage` 时，只查找完全相同的频道，不自动匹配 Gameplay Tag 的父级或子级频道。

## 频道和命名

### 频道

消息频道使用 `Message.*` 前缀，并按领域分组：

```text
Message.Combat.Damage
Message.Combat.Heal
Message.Combat.Elimination
Message.UI.Toast
```

建议将正式使用的频道定义为 `MalogicGameplayTags.h/.cpp` 中的原生 Gameplay Tag，而不是在业务代码中重复调用字符串查找。例如：

```text
Message_Combat_Damage -> Message.Combat.Damage
```

GameplayCue 标签与 Message 频道分开命名：

```text
GameplayCue.UI.Damage
GameplayCue.UI.Damage.Light
Message.Combat.Damage
```

前两者表示 GAS 表现通知及其具体变体，后者表示本地模块之间的消息路由。不要因为两个 Tag 看起来相似就把它们混用。

## 消息载荷约束

系统通过模板参数 `T` 表示消息载荷：

```cpp
template <typename T>
void BroadcastMessage(FGameplayTag Channel, const T& Payload);
```

```cpp
template <typename T>
FMalogicMessageListenerHandle RegisterListener(
    FGameplayTag Channel,
    UObject* ListenerOwner,
    TFunction<void(FGameplayTag, const T&)> InCallback);
```

`T` 必须提供 `T::StaticStruct()`，因此当前设计要求消息载荷是能够被 Unreal 反射系统识别的 `USTRUCT`。载荷以 `const T&` 传给发布接口和回调，监听器不应保存该引用供广播结束后继续使用。

系统当前没有定义具体的消息载荷。例如，伤害消息需要由后续业务代码自行定义 `USTRUCT`，Message 系统本身不会解释其中的字段。

## 类型擦除

不同消息结构体的回调类型不同，不能直接放入同一个 C++ 容器。当前实现通过两层类型完成统一存储：

```text
FMalogicMessageListenerBase
    |
    +-- TMalogicMessageListener<T>
```

### `FMalogicMessageListenerBase`

基类保存所有监听器共有的信息：

```cpp
int64 HandleId;
TWeakObjectPtr<UObject> ListenerOwner;
TObjectPtr<UScriptStruct> PayloadStruct;
bool bRegistered;
```

它提供：

- `GetPayloadStruct()`：返回监听器对应的载荷反射类型；
- `GetHandleId()`：返回监听器句柄 ID；
- `IsRegistered()`：返回监听器是否仍处于注册状态；
- `IsInvocationAllowed()`：同时检查注册状态和 Owner 是否有效；
- `Deactivate()`：将监听器标记为不可调用；
- `Invoke(FGameplayTag, const void*)`：由派生类实现实际回调。

### `TMalogicMessageListener<T>`

模板监听器在构造时保存：

```cpp
TFunction<void(FGameplayTag, const T&)> Callback;
```

同时将 `T::StaticStruct()`记录到基类的 `PayloadStruct` 中。调用 `Invoke` 时，只有广播流程已经确认反射类型完全一致，才把 `const void*` 转换为 `const T*`，再调用具体回调。

## 监听器句柄

```cpp
USTRUCT(BlueprintType)
struct MALOGIC_API FMalogicMessageListenerHandle
```

句柄内部保存一个 `int64 HandleId`：

- `0` 表示无效句柄；
- 注册成功后从 Subsystem 获取一个递增的非零 ID；
- `IsValid()` 判断句柄是否有效；
- `Reset()` 将句柄设为无效；
- 支持 `==` 和 `!=` 比较。

句柄不拥有监听器，也不负责自动注销。调用方应保存注册返回的句柄，并在不再需要监听时调用 `UnregisterListener`。

## 注册监听器

注册接口如下：

```cpp
template <typename T>
FMalogicMessageListenerHandle RegisterListener(
    FGameplayTag Channel,
    UObject* ListenerOwner,
    TFunction<void(FGameplayTag, const T&)> InCallback);
```

注册过程：

```text
检查 Channel 有效
检查 ListenerOwner 有效
检查 InCallback 有效
    |
    +-- 任一检查失败：记录 Warning，返回无效句柄
    |
    +-- 分配递增 HandleId
    +-- 创建 TMalogicMessageListener<T>
    +-- 保存 PayloadStruct = T::StaticStruct()
    +-- 添加到 ListenerMap[Channel]
    +-- 返回 FMalogicMessageListenerHandle
```

`ListenerOwner` 被保存为 `TWeakObjectPtr<UObject>`。Owner 被销毁后，监听器仍可能暂时存在于数组中，但广播时 `IsInvocationAllowed()` 会阻止回调执行。正常使用仍应由调用方在自身生命周期结束时显式注销。

## 广播消息

广播接口如下：

```cpp
template <typename T>
void BroadcastMessage(FGameplayTag Channel, const T& Payload);
```

广播过程：

```text
检查 Channel 是否有效
    |
    +-- 无效：记录 Warning 并返回
    |
查找 ListenerMap[Channel]
    |
    +-- 没有监听器：直接返回
    |
复制当前频道的监听器数组
    |
遍历监听器快照
    |
检查：监听器有效、仍注册、Owner 有效、载荷类型完全匹配
    |
调用监听器回调
```

载荷类型的判断方式是：

```cpp
Listener->GetPayloadStruct() == T::StaticStruct()
```

因此，同一频道下可以注册不同载荷类型的监听器，但广播某种类型时只会调用该类型匹配的监听器。

广播是同步调用。`BroadcastMessage` 返回时，本次广播中所有符合条件的回调已经执行完毕。

## 广播快照安全

广播不会直接遍历 `ListenerMap` 中的原始数组，而是先复制一份当前频道的监听器快照：

```cpp
const TArray<TSharedPtr<FMalogicMessageListenerBase>> ListenerSnapshot = *ChannelListeners;
```

这样可以允许回调中注册或注销监听器，不会因为修改原数组导致当前遍历失效。

当前语义如下：

- 广播开始后新注册的监听器不接收当前消息；
- 回调中注销自己或其它监听器不会破坏当前快照；
- 已从系统注销的监听器在轮到它执行前会因 `IsInvocationAllowed()` 检查失败而被跳过；
- 回调中再次广播是允许的，但系统不限制业务代码形成递归或循环广播。

快照中的 `TSharedPtr` 可以保证监听器对象在本次遍历期间仍然存在；`bRegistered` 则保证已经注销的监听器不会继续执行。

## 注销监听器

```cpp
bool UnregisterListener(FMalogicMessageListenerHandle& Handle);
```

注销过程：

1. 如果传入句柄无效，返回 `false`。
2. 读取句柄 ID。
3. 遍历频道 Map，查找相同 ID 的监听器。
4. 找到后调用 `Deactivate()` 并从数组移除。
5. 如果频道数组为空，则移除该频道。
6. 无论是否找到监听器，都将传入句柄 `Reset()`。

返回值表示是否实际移除了监听器。由于函数会修改句柄，所以参数是非常量引用。

重复注销同一个句柄是安全的：第一次注销后句柄变为无效，后续调用直接返回 `false`。

## Subsystem 生命周期

### 初始化

```cpp
void UMalogicMessageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
```

实现首先调用父类 `Initialize`，然后将 `NextHandleId` 重置为 `1`。监听器 Map 默认为空。

### 反初始化

```cpp
void UMalogicMessageSubsystem::Deinitialize()
```

反初始化时：

1. 遍历所有频道和监听器；
2. 对每个有效监听器调用 `Deactivate()`；
3. 清空 `ListenerMap`；
4. 将 `NextHandleId` 重置为 `1`；
5. 调用父类 `Deinitialize`。

外部保存的句柄不会因为 Subsystem 清理而自动同步修改，但 Subsystem 内部的监听器已经不可调用。调用方不应在 Subsystem 反初始化后继续使用旧句柄注册或广播。

## 当前使用方式

调用方需要完成以下步骤：

```cpp
UMalogicMessageSubsystem* MessageSubsystem =
    GetGameInstance()->GetSubsystem<UMalogicMessageSubsystem>();

FMalogicMessageListenerHandle Handle =
    MessageSubsystem->RegisterListener<FMyMessage>(
        Channel,
        ListenerOwner,
        [](FGameplayTag MessageChannel, const FMyMessage& Payload)
        {
            // 处理消息
        });

MessageSubsystem->BroadcastMessage<FMyMessage>(Channel, Payload);

MessageSubsystem->UnregisterListener(Handle);
```

实际调用时应先确认 Subsystem 指针有效。监听器 Owner 应是拥有该监听生命周期的有效 UObject，句柄应由该 Owner 或对应的管理对象保存。

## 当前边界和限制

- 只支持 C++ 模板接口；当前没有 Blueprint `Broadcast` 或 `Register` 节点。
- 不支持蓝图通配载荷，因此没有 `CustomThunk`、`Stack.StepCompiledIn` 或动态结构体转换逻辑。
- 频道必须是有效的 `FGameplayTag`，无效频道不会注册或广播。
- 载荷必须能通过 `T::StaticStruct()`获得反射结构类型。
- 注册必须提供有效的 `UObject` Owner 和有效回调。
- 广播、注册和注销应在游戏线程执行；当前容器没有线程同步机制。
- 广播是同步的，不提供队列、延迟执行、优先级或异步调度。
- 消息不会跨网络复制，也不会替代 RPC、属性复制或其它网络通信机制。
- 系统不保存消息历史；没有订阅关系建立前广播的消息不会被补发。
- 系统不自动按 Gameplay Tag 父子层级匹配频道。
- 系统不限制同一频道的监听器数量，也不限制回调递归深度。

## 当前代码的设计原则

1. 使用 `FGameplayTag` 作为频道地址，避免发布者直接依赖接收者类型。
2. 使用 `USTRUCT` 反射类型检查，避免不同消息载荷被错误地传给回调。
3. 使用类型擦除基类，让不同模板载荷共享一个监听器容器。
4. 使用句柄管理监听器注册关系，支持显式注销。
5. 使用 `TWeakObjectPtr` 检查 Owner 生命周期，避免向已销毁 UObject 执行回调。
6. 使用广播快照，允许回调在广播过程中修改监听器集合。
7. 保持 MessageSubsystem 只负责路由，不承担任何具体业务逻辑。

## 后续扩展时的兼容边界

后续新增具体消息时，只需要定义新的 `USTRUCT` 并选择频道，不应修改 MessageSubsystem 来加入具体业务字段。后续若要增加 Blueprint 支持、父子频道匹配、异步队列或线程安全，需要单独扩展接口和语义，并重新评估当前精确匹配、同步广播和游戏线程限制。

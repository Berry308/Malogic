# 策划关键需求
伤害UI只会在造成伤害的客户端显示，而不会同步到其它客户端（在GCN中进行检查）
伤害UI根据伤害属性类型会使用不同的表现
伤害UI会根据此次伤害的数值进行不同的UI消失动画表现 或 伤害UI会根据此次伤害是否导致某个事件如击杀敌方，而显示不同效果（暂不考虑）
伤害UI会根据此次是否发生元素反应而使用不同的UI
伤害UI的叠加应该怎么做？（第一阶段可以先不实现，提供两种显示方式）

# 人工梳理工程步骤思路
第一阶段：不使用伤害合并UI
+ 定义Message系统（仿Lyra）
+ 新增BattleMessage.h文件,在其中定义伤害Message载荷结构体：FGameplayDamageMessage（包括伤害数值、伤害坐标、伤害类型（GameplayTag）、伤害的发起者、消息频道的GameplayTag等）
+ 定义UGCN_DamageExecuted类，在GCN的OnExecute_Implementation中创建FGameplayDamageMessage实例，并最后通过MessageSubsystem广播
+ 在MalogicDamageExecution类中，根据最终的GameplayTag触发GCN
+ 在UIManager中监听Message系统的对应频道，并绑定回调函数
+ 在回调函数中，根据传入的Message，进行世界空间坐标到屏幕空间坐标的转换，添加随机偏移，并创建Widget添加到指定Widget队列（ActivatableQueue）中
+ 新建ActivatableQueue，该类不像ActivatableStack那样自动控制显隐，它只影响显示层级关系，先进后出。


## 何时触发GCN
### 方案一：在GE中配置GCN自动触发
这是最常用的“声明式”触发方案。

打开你的伤害 GE 蓝图（如 GE_Damage_Basic）。
在 Gameplay Cues 数组中添加一个元素。
Tag: 设置为 GameplayCue.Character.DamageTaken。
Magnitude Attribute: 设置为 LyraHealthSet.Damage。
原理：只要这个 GE 被应用到目标身上，GAS 底层的 AbilitySystemComponent 就会自动扫描这个数组，并在目标坐标处调用关联的 GCN 类。

优点：配置式、符合 GAS 使用方式，便于保持伤害效果和表现的关联。第一阶段优先验证此方案。

需要验证：

- `RawMagnitude` 是否是本次实际伤害，而不是 GE 的基础配置值。
- `Location` 是否稳定携带命中点；没有命中点时使用的回退位置是什么。
- `Instigator`、`EffectCauser` 和原始 Cue Tag 在目标客户端是否可用。
- GCN 是否会在预期的客户端执行，以及本地玩家过滤是否可靠。

### 方案二：在HealthSet的PostGameplayEffectExecute触发GCN
如果你需要根据复杂的逻辑（例如：只有当伤害导致目标被消除 (Elimination) 时才显示特殊颜色）进行触发，可以在 LyraHealthSet.cpp 的 PostGameplayEffectExecute 中手动编写：
// 在 C++ 的属性变更回调中手动执行
if (Data.EvaluatedData.Attribute == GetDamageAttribute())
{
    const float LocalDamage = GetDamage();
    if (LocalDamage > 0.f)
    {
        FGameplayCueParameters Params(Data.EffectSpec.GetContext());
        Params.RawMagnitude = LocalDamage;
        Params.Location = ...; // 从 Context 提取 HitResult 的 ImpactPoint

        // 手动触发
        GetOwningAbilitySystemComponent()->ExecuteGameplayCue(
            FGameplayTag::RequestGameplayTag("GameplayCue.Character.DamageTaken"), 
            Params
        );
    }
}

### 方案三：在Execution中触发GCN（选取此方案）
在MalogicDamageExecution中，新增局部变量FGameplayTag UIGameplayCueTag，在Execution的不同分支中（如触发了某种元素反应）对其进行设置，然后函数尾部通过ExecutionParams获取发起者的ASC，并调用ExecuteGameplayCue传入UIGameplayCueTag以触发指定的GCN。

注意：在ExecuteGameplayCue函数中，如果找不到匹配形参GameplayTag的GCN，它就会寻找匹配形参父级GameplayTag的GCN。例子：新建UGCN_DamageNumber,其对应GameplayTag为GameplayCue.UI.Damage。我在Execution中调用ExecuteGameplayCue的时候传入GameplayTag:GameplayCue.UI.Damage.Light，那么函数内部找不到注册的对应GCN，它就会去查找GameplayCue.UI.Damage匹配的GCN，也就是UGCN_DamageNumber。在UGCN_DamageNumber类中，我可以通过传入的参数获取到Parameters.OriginalTag，而此时它的值仍为GameplayCue.UI.Damage.Light，然后就可以通过Message系统广播这个Tag了。

#### 分析
+ 如何获取伤害的属性以显示不同的UI？


## 伤害事件数据流

当前 Malogic 的伤害计算在 `UMalogicDamageExecution` 中运行，并在服务端计算最终伤害；`UMalogicHealthSet::PostGameplayEffectExecute` 负责将 Damage Meta Attribute 转换为 Health 变化。消息系统不能直接从服务端执行路径把数据变成本地 UI。

目标数据流为：

```text
伤害 Ability / GameplayEffect
    -> Damage Execution 计算最终伤害
    -> GameplayCue 在目标侧按 GAS 规则执行
    -> Damage GameplayCue Notify 识别本地显示资格
    -> 构建 FDamageMessage
    -> UMalogicMessageSubsystem.BroadcastMessage(Message.Combat.Damage, Message)
    -> 当前本地玩家的 UMalogicUIManager 回调
    -> DamageNumberLayer 管理飘字 Widget
```

Combat、GAS 和 GameplayCue 只负责产生并广播事件；它们不直接创建 Widget。UIManager 和 DamageNumberLayer 只负责本地表现，不修改伤害属性。


## 网络边界与本地过滤

### 伤害数字的显示资格

伤害数字要求“只在造成伤害的客户端显示”。GCN 广播到多个客户端时，应在进入 Message 广播前过滤，而不是让所有客户端先广播再由 Widget 过滤。

推荐的过滤顺序：

1. 从 GameplayCue 参数或 Effect Context 取得原始 Instigator。
2. 将 Instigator 解析到其 Pawn、Controller 或所属本地玩家。
3. 判断该来源是否属于当前客户端的本地控制玩家。
4. 只有判断成功且伤害数值大于零时，才构建并广播 `FDamageMessage`。

不能无条件假设 `Parameters.Instigator` 一定是 Pawn，也不能把 `IsLocallyControlled()` 直接调用在任意 EffectCauser 上。投射物、Ability、PlayerState 和服务器生成的 Actor 可能导致这些字段类型不同或为空。过滤辅助函数应覆盖这些情况，并在开发期记录无法判定的事件。

方案的优点是复用 GAS 的 Cue 流程、实现量小、UI 不会显示远端伤害；它不一定消除 Cue 本身的网络开销。若实测伤害频率较高且带宽确实成为问题，再考虑“服务器只向攻击者发送定向 Client RPC，客户端本地执行 UI Cue”的方案。那会引入单独的网络协议，不属于第一阶段 MessageSubsystem 的职责。

## 伤害消息载荷

建议定义一个只携带 UI 所需最终数据的 `USTRUCT`，例如 `FDamageMessage`。第一阶段字段如下：

| 字段 | 类型建议 | 说明 |
| --- | --- | --- |
| `DamageAmount` | `float` | 最终实际造成的伤害，必须为非负值。 |
| `WorldLocation` | `FVector` | 飘字投影到屏幕前使用的世界坐标，优先使用命中点。 |
| `DamageType` | `FGameplayTag` | 元素或伤害属性，例如 `Damage.Light`。 |
| `ReactionTag` | `FGameplayTag` | 元素反应类型；没有反应时为空。 |
| `Instigator` | `TWeakObjectPtr<AActor>` | 造成伤害的原始发起者，用于诊断或后续表现。 |
| `EffectCauser` | `TWeakObjectPtr<AActor>` | 实际造成效果的 Actor，例如投射物。 |
| `Target` | `TWeakObjectPtr<AActor>` | 受击对象；可用于跟随移动目标或后续淘汰判断。 |
| `bCritical` | `bool` | 是否暴击；没有该概念时保持默认值。 |

击杀、免疫、暴击等状态如果第一阶段尚未实现，应保留为可扩展字段或单独消息，不要让 UI 通过再次访问 ASC 推断。消息应尽量是已经完成业务判定的 UI 数据。

`MessageChannel` 不建议作为载荷字段，因为它已经是 `BroadcastMessage(Channel, Payload)` 的参数。只有当 UI 需要把原始 GameplayCue 变体传递到更下游时，才额外保存 `SourceCueTag` 或等价字段。

## 与现有 UI 框架的接入

### 职责边界

当前 Malogic UI 分层为：

```text
LocalPlayer
    -> UMalogicUIManager
    -> AMalogicHUD
    -> UPrimaryGameLayout
    -> 常驻 HUD / 交互层 / 临时表现层
```

伤害数字属于本地、短生命周期的临时表现：

- `UMalogicUIManager` 注册 Message 监听器并负责监听生命周期。
- `UDamageNumberLayer` 或等价的专用层持有活动飘字、创建和移除 Widget。
- 伤害 Widget 接收已经转换好的显示数据，负责动画和绑定表现。
- `UVMLocalPlayerManager` 继续管理长期玩家状态 ViewModel；不应把每一次伤害都当作长期 Player ViewModel 字段。
- Widget 不直接访问 ASC、HealthSet 或其它 Gameplay Component。

消息监听应在 `UMalogicUIManager` 已有本地玩家 UI 生命周期内建立，并在 `Deinitialize()` 中注销句柄。Controller 更换或 Pawn 重生不应导致全局 MessageSubsystem 残留旧 UI 回调；若监听回调需要本地玩家对象，应通过 `TWeakObjectPtr` 或重新绑定逻辑处理。

### 不直接使用现有 Top UI Stack

当前 `UActivatableWidgetStack` 的 Push 行为会折叠旧栈顶 Widget。它适合窗口或单个临时覆盖界面，不适合同时显示多条伤害数字。

因此伤害数字不应每次都 Push 到 `EWidgetLayer::TopUI` 的 Stack。建议在 `UPrimaryGameLayout` 或 HUD 根 Widget 中增加一个专用的 `DamageNumberLayer` Overlay：

```text
PrimaryGameLayout
    HUDLayerStack
    InteractableUILayerStack
    TopUILayerStack
    DamageNumberLayer  // 独立 Overlay，不参与窗口栈和输入模式切换
```

该层应是 `SelfHitTestInvisible`，不改变 PlayerController 的输入模式，也不应盖住交互窗口的鼠标和键盘输入。

### ViewModel 取舍

伤害数字是一次性事件而不是持续状态，不必为了形式统一而把所有消息字段放进全局 ViewModel。推荐：

- 消息到达时构建一次性显示数据；
- DamageNumberLayer 创建短生命周期 Widget；
- 如果 Widget Blueprint 使用 MVVM 绑定，则为该 Popup 创建由 Layer 持有的临时 `UDamageNumberViewModel`，在 Widget 销毁前由 Layer/Popup 清理；
- 不让 Widget 自己长期查找或持有 `UMalogicHealthComponent`、ASC 或 MessageSubsystem。

这样保留 MVVM 的 Model/View 解耦，同时避免用 Player 作用域 Service 管理大量瞬时 Popup 对象。

## 伤害数字叠加策略

第一阶段建议采用“每次有效命中一个飘字 + 专用 Overlay + 局部避让”的策略，先验证消息链路和表现，再加入合并。

### 基础策略：不合并

每条有效 `FDamageMessage` 创建一个 Popup：

1. 使用 `WorldLocation` 调用 `ProjectWorldLocationToScreen`。
2. 如果目标在屏幕外或被视线规则判定不可显示，则丢弃或使用项目确定的回退规则。
3. 在同一目标附近的活动 Popup 中分配一个垂直偏移或短暂横向偏移。
4. Popup 播放自身的出现、停留和消失动画。
5. 动画结束后由 Layer 移除并释放 Widget。

Layer 需要保存活动条目，例如 Widget、生成时间、目标弱引用、世界位置和当前偏移。目标移动时，如果设计要求飘字跟随目标，Layer 在活动条目存在期间更新屏幕位置；如果只表现命中瞬间，则只使用初始投影，不需要持续跟随。

### 后续策略：时间窗口内合并

当高频多段伤害导致屏幕过于拥挤时，可以按以下 Key 合并：

```text
(Target, DamageType, ReactionTag, DisplayOwner)
```

在很短的时间窗口内，将相同 Key 的伤害累加到同一个 Popup，并刷新该 Popup 的动画。合并必须有上限和超时，避免持续伤害永远不消失。不同伤害类型或元素反应不应无条件合并，否则会丢失表现语义。

TMap<TWeakObjectPtr<AActor>, TMap<FName, UDamageWidget*>> ActiveDamageMap;

推荐可配置项：

- 合并时间窗口；
- 同一目标的最大同时 Popup 数；
- 相邻 Popup 的最小屏幕间距；
- 每类伤害的优先级；
- 超过上限时的丢弃、合并或排队规则；
- Popup 的最大生存时间。

### 对象池

对象池不是 Message 系统的必需组成部分。先用正常创建/移除验证功能；当实测高频伤害造成 Widget 分配压力时，再在 DamageNumberLayer 内加入按 Widget Class 分类的池。池的生命周期仍由本地 HUD/UI 层管理，不放进 GameInstance MessageSubsystem。

## 生命周期与错误处理

监听方必须明确以下时机：

```text
UMalogicUIManager::Initialize
    -> 获取 UMalogicMessageSubsystem
    -> RegisterListener<FDamageMessage>

UMalogicUIManager::Deinitialize
    -> UnregisterListener
    -> 清空 DamageNumberLayer
```

还需要处理：

- GameInstance、World、LocalPlayer 或 HUD 尚未准备好时，消息可以被忽略，不能创建无 Owner 的 Widget。
- HUD 被销毁或 Controller 更换时，不能继续向旧 Overlay 添加 Popup。
- 频道无效、载荷类型不匹配、位置不可投影和来源无法判定时，应安全丢弃并在开发期提供足够日志。
- 消息载荷是广播期间的临时只读引用，监听器不得保存该引用；如果异步使用，必须复制需要的数据。
- MessageSubsystem Deinitialize 时清空频道表；任何外部句柄随后都应变为无效。

## 第一阶段实现顺序

1. 在 `Source/Malogic/Message/` 实现 `FMalogicMessageListenerHandle`、类型擦除监听器和 `UMalogicMessageSubsystem`。
2. 添加 `Message.Combat.Damage` 等正式原生 Gameplay Tags，并检查 Tag 配置和命名残留。
3. 定义 `FDamageMessage`，先使用最小字段：最终伤害、世界位置、伤害类型、反应 Tag 和来源/目标弱引用。
4. 添加通用 Damage GameplayCue Notify，验证 Cue 参数、Effect Context、原始 Cue Tag 和客户端执行情况。
5. 在 GCN 中完成本地伤害来源过滤；过滤通过后广播 `FDamageMessage`。
6. 让 `UMalogicUIManager` 注册和注销监听器，并接入专用 DamageNumberLayer。
7. 创建一个最小 Damage Number Widget，先实现一条消息一个 Popup、屏幕投影、动画和清理。
8. 编译 `MalogicEditor Win64 Development`，再使用 PIE/多人 PIE 验证消息生命周期、伤害来源过滤、位置投影和多条飘字叠加。
9. 只有在基础链路稳定后，才加入元素样式、合并策略、对象池、暴击和击杀表现。

## 必须验证的运行时问题

这些问题不能仅通过 C++ 静态阅读确认：

- 伤害 GameplayEffect 的 GameplayCue 在服务器、攻击者客户端和其它客户端分别执行几次。
- `FGameplayCueParameters::RawMagnitude` 是否等于经过 Damage Execution、免疫和衰减后的实际伤害。
- `FGameplayCueParameters::Location` 和 Effect Context 的 HitResult 是否在目标客户端有效。
- `Parameters.Instigator`、EffectCauser、Target 和 `OriginalTag` 在不同伤害来源下的具体类型及有效性。
- `IsLocallyControlled()` 在 Pawn、Controller、PlayerState 和投射物来源下的行为。
- 伤害执行只在服务端时，客户端显示数据通过哪条 GAS Cue 或网络路径到达。
- 同一 GE/GCN 是否可能同时触发健康变化回调和 UI Cue，造成重复数字。
- 角色重生、重新控制、地图切换和 HUD 重建时，旧监听器和 Popup 是否全部清理。


## 潜在问题（即暂不考虑的问题）

### 那一般来说，玩家的伤害UI只在造成伤害的玩家客户端显示，而GCN会将信息广播给所有客户端，这样不是会造成网络带宽的浪费吗？
这是一个非常敏锐且专业的洞察。你所担心的“全员广播导致的带宽浪费”和“屏幕UI混乱”正是大规模多人游戏（如《守望先锋》或《堡垒之夜》）在开发视觉反馈系统时必须解决的核心问题。
在 Lyra 和 GAS 架构中，针对你的疑虑，有以下几种分层优化的解决方案：
1. 为什么 GCN 默认会广播？
在 GAS 的原生逻辑中，GameplayCue 确实默认是广播的。这是因为很多视觉效果（如爆炸火花、受击硬直动画）需要所有玩家都能看到。
但针对“伤害数字”这种私密性 UI，Lyra 推荐以下几种优化策略：

方案 A：在客户端进行“本地玩家”过滤（逻辑层优化）
这是实现成本最低的方法。虽然所有客户端都收到了 GCN 的微小网络包，但我们通过代码逻辑拦截，只允许造成伤害的玩家去执行 UI 逻辑。
在 C++ GCNL_DamageNumbers::OnExecute 中添加过滤：
void UGCNL_DamageNumbers::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
    // 获取造成伤害的那个人的 Pawn
    AActor* DamageInstigator = Parameters.Instigator.Get();
    
    // 关键判断：只有当发起者是本地玩家控制的角色时，才进行后续逻辑
    if (DamageInstigator && DamageInstigator->IsLocallyControlled())
    {
        // 只有“我”造成的伤害，才会在“我”的屏幕上发消息并弹数字
        UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(GetWorld());
        
        FMyDamageMessage Msg;
        Msg.Location = Parameters.Location;
        Msg.DamageAmount = Parameters.RawMagnitude;
        // ... 其他赋值
        
        MessageSubsystem.BroadcastMessage(FGameplayTag::RequestGameplayTag("UI.Event.DamageNumber"), Msg);
    }
}

评价：这种方法虽然没节省网络带宽（包还是发出去了），但完美解决了 UI 混乱问题，且代码最符合 GAS 标准流程。

方案 B：使用本地执行 (Local Execution) 绕过网络广播
如果你追求极致的带宽优化，希望彻底消除网络包的浪费，可以不在 GameplayEffect 的资源面板里配置 Cue，而是改为在代码中手动触发本地 Cue。

在服务器端逻辑中（例如 LyraHealthSet 的伤害处理处）：识别到伤害发生后，不让 GE 自动发 Cue，而是针对攻击者发送一个专有的 RPC 或者利用 Lyra 的 Global Message Subsystem 进行定向通知。
定向通知攻击者：服务器识别到攻击者是玩家 A，则只给玩家 A 发送同步信号。玩家 A 收到信号后，在本地调用 ExecuteGameplayCueLocal。

评价：这能消除网络带宽压力，但开发量较大，因为你需要手动处理服务器到特定客户端的定向通知逻辑。

方案 C：Lyra 的权衡与 GAS 压缩技术
实际上，你可能高估了 GCN 带来的网络负担。GAS 对 Gameplay Cue 做了深度优化：

Tag 压缩：发送的不是字符串，而是经过压缩的 Gameplay Tag 哈希值（几个字节）。
数据量极小：FGameplayCueParameters 只包含必要的数据（位置、数值），且经过了位压缩。
频率限制：Lyra/GAS 内部有逻辑限制 Cue 的触发频率，防止高频伤害导致网络拥塞。

对于大多数项目，方案 A 是最佳平衡点：利用 GAS 成熟的同步机制获取数据，然后在本地客户端通过 IsLocallyControlled() 进行过滤显示。
总结架构建议

触发点：依然放在 GameplayEffect 的 GameplayCues 数组里，让系统自动处理位置和数值的传递。
过滤点：在你的 GameplayCueNotify_Static 类中判断 IsLocallyControlled()。
UI 映射：只有通过过滤的客户端才会调用 Gameplay Message Subsystem，从而触发 HUD 上的 ProjectWorldToScreen 逻辑生成数字。

这样既利用了 GAS 的自动化流程，又确保了每个玩家只看到属于自己的伤害反馈，实现了视觉上的消除重叠和逻辑上的性能平衡。
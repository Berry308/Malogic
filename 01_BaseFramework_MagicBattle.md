# 说明
## 关键设计决策
### 魔法阵的施放方式
背景：魔法阵有不同的施放方式，如点按激活、长按持续激活等

分析：

+ 每个魔法阵所需要的初始数据都有所不同（如发动射线检测，命中点为魔法阵部署位置或魔法射击目标位置）

方案一：

在装备武器时赋予玩家单个 MalogicGA_MagicCircleDeploy，玩家通过激活这一个能力，获取玩家身上装备的魔法阵并施放

缺点：如果不同魔法阵需要不同的数据以及部署方式，单个 GA 无法完成

方案二：

每个魔法阵都具有一个 MalogicGA_MagicNameDeploy（赋予玩家部署该魔法阵的能力） 和 MalogicGA_MagicName（该魔法阵拥有的能力），在装备魔法阵的时候通过 AbilitySet？ 赋予给玩家。

缺点：可能会存在很多重复的代码，有些魔法的部署方式是相同的。

方案三：

通过 MagicCircleDefinition 内置 AbilitySet 定义魔法阵的 MalogicGA_MagicCircleDeploy。装备魔法阵时，将该 Definition 的 CDO 作为 AbilitySpec 的 SourceObject，一并授予部署能力。部署 GA 只使用该 SourceObject 中绑定的 Definition 创建 MagicCircleInstance，不在激活时重新查询 Manager 的当前装备。

优点：MagicCircleDefinition 内部组合，即满足了多样化要求，也提高了代码复用率。

## 魔法施放过程
### 装备法杖
-添加输入映射上下文

+ alt 激活绘制魔法阵界面；
+ 12345 选择快捷魔法阵；

-添加 UI

+ 可选择的快捷魔法阵列表

### 绘制魔法阵（或者使用MagicComponent 持有的快捷魔法阵）
暂时跳过该模块

### 装备当前要持有的魔法阵
选择调用 MagicCircleQuickBarComponent 中某个槽位的魔法阵(MagicCircleDefinition)，间接调用 MagicCircleManagerComponent 设置当前装备魔法阵，自动处理 Definition 中的输入映射和 AbilitySet。授予 AbilitySet 时保存对应的 GrantedHandles，并将该 Definition 的 CDO 作为部署能力的 SourceObject；切换槽位时先移除旧 AbilitySet 和输入映射，再授予新的 AbilitySet。

部署能力激活后使用其 SourceObject 中的 Definition 快照。若施法过程中切换快捷栏，已经激活的部署能力仍然完成原魔法阵的部署；新的快捷栏只影响下一次激活。

### 指定魔法阵部署位置
在装备选择魔法阵后，需要显示该魔法阵的轮廓 Actor（只有本地玩家可见以便调整），代表激活魔法阵时其将会部署的位置。

部分魔法阵不需要魔法阵轮廓，可以在 MagicCircleDefinition 中定义

### 部署当前装备的魔法阵
玩家左键输入，激活先前由 MagicCircleDefinition 的 AbilitySet 赋予的 MalogicGA_MagicDeploy。该能力从自身 AbilitySpec 的 SourceObject 获取对应 Definition，并根据能力逻辑在指定位置生成魔法阵实例，不直接读取 MagicCircleManagerComponent 的 EquippedMagicCircle。

部署类型（在 GA 中自定义不同魔法阵的部署逻辑）：

+ 根据魔法类型选择性进行射线检测并获取到命中目标点作为部署位置。
+ 部分魔法阵不需要部署，通过左键直接指定射击目标点，然后在玩家身前部署，自动调整朝向。
+ 使用先前玩家通过输入指定的魔法阵位置。

### 初始化魔法阵（自动激活或选择激活）
生成 MagicCircleInstance 后，根据 MagicCircleDefinition 中定义的 AbilitySet 对其 ASC 进行初始化，包括初始化血量的 GE、赋予 MagicCircleInstance 本身的魔法能力，以及将每个 GA 的 `ActivationTags` 写入其动态 Spec Tags。

### 激活魔法阵
激活方式：

+ 魔法阵自身自动激活
+ 由玩家指定，手动激活（扩展）

### 魔法阵释放魔法（有可能存在不自动释放的魔法）
魔法阵在部署并完成初始化后，必定先进入 Building 阶段，播放构建动画（目前的想法是控制透明度从 0 到 1）。

构建完成后进入 Ready 阶段，再根据 ActivateStrategy 自动激活、等待玩家激活或等待检测条件。魔法能力激活后才进入 Active 阶段并释放魔法。

## 工程计划
第一阶段：搭建魔法战斗的基础框架

设计第一个简单的魔法进行测试。

第二阶段：元素反应框架的设计

先使用两种元素进行测试，主要在 Execution 中进行设计，使用 UnLua 进行尝试。

第三阶段：复杂魔法的设计，需要突出 3C 的设计

第四阶段：龙卷风魔法的设计

将龙卷风魔法使用 MassEntity 构建。

使用不同的元素魔法攻击龙卷风时，能够转变龙卷风的攻击属性。

---

# MagicSet
## 概述
仿照 MalogicHealthSet 实现，整体结构、属性约束、复制回调和 GameplayEffect 执行流程保持一致，仅将生命值语义替换为魔力。

基类：MalogicAttributeSet

来源：通过 PawnData 的 AbilitySet 赋予给玩家该属性集

职责：

+ MagicValue：当前魔力值，始终限制在 0 到 MaxMagicValue 之间。
+ MaxMagicValue：最大魔力值，可由 GameplayEffect 修改。
+ MagicConsume：临时 Meta Attribute，对应 HealthSet 的 Damage；在 GameplayEffect 结算后从 MagicValue 中扣除，不作为持久状态复制。
+ MagicRecovery：临时 Meta Attribute，对应 HealthSet 的 Healing；在 GameplayEffect 结算后增加 MagicValue，不作为持久状态复制。
+ 在 MagicSet 文件中声明并定义无限魔力 Gameplay Tag。拥有该 Tag 时，魔力消耗结算不扣除 MagicValue；移除 Tag 后恢复正常消耗。

---

# AbilitySet 中的能力标签数组添加
## 概述

`FAbilitySet_GameplayAbility` 同时支持玩家输入激活Tag和其它用途的FGameplayTag。两者都通过 `FGameplayAbilitySpec` 的动态 Spec Tags 保存，但语义必须分开。

## 成员

```cpp
TArray<FGameplayTag> ActivationTags // 目前用于内部按标签查找和激活能力
```

标签命名空间必须区分：

+ `InputTag.*` 只用于 `AbilityInputTagPressed` 和 `ProcessAbilityInput`；
+ `MagicCircle.Ability.*` 用于 MagicCircleInstance 内部的阶段能力调度。

建议使用明确的阶段标签，例如：

+ `MagicCircle.Ability.BuildFinished`：构建完成时调用；
+ `MagicCircle.Ability.Activate`：魔法阵正式激活时调用；
+ `MagicCircle.Ability.TargetDetected`：检测到目标时调用；
+ `MagicCircle.Ability.Destroyed`：魔法阵销毁前调用。

## GiveToAbilitySystem

授予 `FAbilitySet_GameplayAbility` 时：

+ 遍历 `ActivationTags`，将每个有效 Tag 加入同一个动态 Spec Tags 容器；
+ 空 Tag 不参与激活，重复 Tag 由 GameplayTagContainer 自动去重；

同一个 `ActivationTag` 可以配置给多个 GA，表示该阶段需要并行激活多个能力。若某阶段只允许一个 GA，应在数据配置中保持唯一。

---

# MagicComponent
## 概述
仿照 HealthComponent 来写

基类：PawnComponent（BlueprintType, Meta = (BlueprintSpawnableComponent)）

职责：

+ 负责进行 MagicAttribute 的属性值初始化，委托绑定
+ 负责魔法阵的绘制与激活<font style="color:#DF2A3F;">（待定，取消，魔法阵的绘制由专门的组件负责）</font>

## 成员
变量：

函数：

---

# MagicCircleInstance
## 概述
基类：`AActor`（建议类名为 `AMalogicMagicCircleInstance`）

每个魔法阵实例都是一个独立的 GAS Actor，拥有自己的 ASC、HealthSet 和生命周期。实例由服务器生成并复制；客户端不负责决定实例是否生成、何时施法或是否死亡，只负责根据复制状态播放表现。

职责：

+ 保存该次部署的魔法阵运行时状态，以及部署者/Instigator 和来源数据；
+ 持有并初始化自己的 `UMalogicAbilitySystemComponent`；
+ 通过 `AbilitySetForMagicCircle` 获得血量、战斗属性和魔法能力；
+ 处理构建、激活、受击、释放和销毁；
+ 负责把血量归零转换为服务器权威的销毁流程。

### 所有权与网络约定

+ `bReplicates = true`。魔法阵通常是固定位置 Actor，不需要复制移动，除非某种魔法明确要求移动；
+ ASC 的 `OwnerActor` 和 `AvatarActor` 都指向该魔法阵实例。部署者保存为 `Instigator`/来源 Actor，用于效果上下文、阵营判断和伤害归属；
+ `MalogicGA_(MagicName)Deploy` 仅在服务器上执行计算和生成，在生成实例前根据 MagicWeapon 的属性、Definition 的 `BaseBuildingTime` 和其它状态计算 `ActualBuildingTime`；
+ 服务器在生成实例时将 Definition、部署者、部署变换和 `ActualBuildingTime` 一并传入实例，并初始化 ASC、授予 AbilitySet；
+ `ActualBuildingTime` 表示服务器计算出的魔法阵构建时间。服务器将 `MagicCircleState` 切换为 `Building` 后，客户端在收到该复制状态时播放构建动画；
+ 目标位置、朝向、部署者和 Definition 必须在生成时确定并保存，后续不能从玩家当前装备项重新读取；
+ 服务器销毁实例前取消其仍在运行的能力并回收 AbilitySet 的 GrantedHandles。

### 生命周期状态

建议使用可复制的状态枚举，而不是通过多个布尔值推断状态：

+ `Spawned`：Actor 已生成，正在完成 ASC、AbilitySet 和部署参数初始化；
+ `Building`：服务器根据 `ActualBuildingTime` 启用构建计时器，客户端播放魔法阵构建表现；
+ `Ready`：构建完成，等待自动或手动激活；
+ `Active`：已激活魔法能力；
+ `Finished`：本次魔法已经释放，但实例是否销毁由生命周期策略决定；
+ `Destroyed`：血量归零、被取消或生命周期结束，进入销毁流程。

`EMagicCircleState` 使用 `UENUM(BlueprintType)` 定义，并通过 `ReplicatedUsing` 驱动客户端表现。

### 生命周期策略

定义枚举类 `EMagicCircleLifetimeStrategy`。例如：

+ OnceAfterSomeGA //一次性魔法，标志为该策略的魔法阵将会在某个GA释放结束后，推进到生命周期状态到Finish
+ PersistentTilDie //持续存在直到血量归零或Lifetime结束，通常用于可以反复激活的魔法阵。

不能再使用“部分绑定、部分不绑定”的隐式约定；每种魔法都应通过一个明确的生命周期策略配置。

### 魔法阵激活策略

定义枚举类 `EMagicCircleActivateStrategy` 描述魔法阵激活策略。该枚举属于 `MagicCircleInstance` 的默认配置，由具体的魔法阵实例子类在 Blueprint 编辑器中指定；Definition 不负责覆盖该配置。

+ 自动激活
+ 玩家手动激活
+ 检测激活（陷阱等）

## 成员
变量：

protected:

UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
EMagicCircleLifetimeStrategy LifetimeStrategy // 魔法阵的生命周期策略

UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
EMagicCircleActivateStrategy ActivateStrategy // 魔法阵的激活策略

UPROPERTY(Replicated)
float ActualBuildingTime // 服务器MalogicGA_(MagicName)Deploy根据武器、状态和效果计算后的实际构建时间

UPROPERTY(ReplicatedUsing = OnRep_MagicCircleState)
EMagicCircleState MagicCircleState

UPROPERTY(Replicated)
TSubclassOf<UMagicCircleDefinition> MagicCircleDefinitionClass // 本次部署使用的 Definition 类

UPROPERTY()
TObjectPtr<UMalogicAbilitySystemComponent> AbilitySystemComponent

UPROPERTY()
TObjectPtr<UMalogicHealthComponent> HealthComponent

UPROPERTY()
TObjectPtr<UMalogicHealthSet> HealthSet

UPROPERTY()
TObjectPtr<UMalogicCombatSet> CombatSet

UPROPERTY(Replicated)
TObjectPtr<AActor> DeploymentInstigator

// 运行时授予本实例的能力、效果和属性集，用于销毁前回收
FAbilitySet_GrantedHandles GrantedHandles

private:

UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
float LifeTime // Ready 阶段开始计时的存活时间；小于等于 0 时不启用该计时器

UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
TSubclassOf<UMalogicGameplayAbility> FinishAbilityClass // 当该类型能力释放结束后会推进魔法阵生命状态到Finish

FTimerHandle LifeTimeTimerHandle
bool bLifeTimeExpired = false

函数：

public:

AMalogicMagicCircleInstance()
+ 创建 ASC 和 HealthComponent；
+ 设置 Replication；
+ 将 ASC 的 OwnerActor 和 AvatarActor 初始化为实例自身。

void InitializeFromDefinition(const UMagicCircleDefinition* Definition, AActor* InInstigator, float InActualBuildingTime)
+ 在 `BeginPlay` 前保存 Definition、部署者和本次部署参数；
+ 接收并保存由 `MalogicGA_(MagicName)Deploy` 计算出的 `ActualBuildingTime`；
+ 由服务器调用，不能在此处直接激活魔法。

void InitializeLifetime()
+ 仅服务器执行生命周期初始化和委托绑定；
+ 如果 `LifetimeStrategy == OnceAfterSomeGA`，`FinishAbilityClass` 不能为空，否则报告配置错误；配置有效时绑定 `OnAbilityFinished` 到 `ASC->OnAbilityEnded`；
+ 如果 `LifetimeStrategy == PersistentTilDie`，`FinishAbilityClass` 必须为空，否则报告配置错误并跳过能力结束绑定。

void BeginPlay()
+ 初始化 ASC ActorInfo；
+ 服务器根据 Definition 的 AbilitySet 授予属性集、初始血量 GE 和魔法能力，并保存 `GrantedHandles`；
+ 调用InitializeLifetime函数
+ 调用 HealthComponent 的 `InitializeWithAbilitySystem`；
+ 服务器调用 `StartBuilding`，所有魔法阵都必须从 `Building` 阶段开始；
+ 客户端等待 `MagicCircleState` 的初始复制，不在 `BeginPlay` 中自行决定是否开始构建动画。

void StartBuilding()
+ 仅服务器调用，并校验当前状态为 `Spawned`；
+ 将状态切换为 `Building`，调用 `OnMagicCircleStateChanged`；
+ 构建计时器由 `OnMagicCircleStateChanged` 在服务器分支中启动，计时长度为 `ActualBuildingTime`，计时结束后调用 `HandleBuildingFinished`。

void HandleBuildingFinished()
+ 仅服务器调用，并校验当前状态为 `Building`；
+ 清理构建计时器；
+ 将状态切换为 `Ready`，调用 `OnMagicCircleStateChanged`；
+ 是否自动激活以及使用哪个 `ActivationTag`，由 `OnMagicCircleStateChanged` 的 `Ready` 分支统一处理。

void OnRep_MagicCircleState(EMagicCircleState OldState)
+ 客户端收到服务器复制的状态后调用 `OnMagicCircleStateChanged(OldState, MagicCircleState)`；
+ 不负责推进服务器状态，也不负责启动服务器计时器。

void OnMagicCircleStateChanged(EMagicCircleState OldState,EMagicCircleState NewState)
+ 根据 `NewState` 分发状态处理逻辑；该函数可以在服务器状态切换后调用，也可以由客户端的 `OnRep_MagicCircleState` 调用；
+ `Building`：服务器启动构建计时器，客户端根据 `ActualBuildingTime` 播放构建动画；
+ `Ready`：客户端结束构建动画；服务器启动 `LifeTime` 计时器，先按需调用 `ActivateAbilitiesByTag(MagicCircle.Ability.BuildFinished)`，再根据 `ActivateStrategy` 决定是否调用 `ActivateMagic(MagicCircle.Ability.Activate)`；
+ `Active`：客户端播放魔法释放表现；
+ `Finished`：停止生命周期计时器，根据生命周期策略播放结束表现或等待销毁；
+ `Destroyed`：服务器按需调用 `ActivateAbilitiesByTag(MagicCircle.Ability.Destroyed)`，再取消能力、回收 GrantedHandles 并延迟销毁 Actor（通过SetLifeSpan(0.5f)实现 ）；客户端播放销毁表现。

void AMalogicMagicCircleInstance::OnAbilityFinished(const FAbilityEndedData& AbilityEndedData)
+ 仅服务器处理；
+ 先判断 `AbilityEndedData.AbilityThatEnded` 和 `FinishAbilityClass` 是否有效；
+ 通过 `AbilityEndedData.AbilityThatEnded->GetClass() == FinishAbilityClass` 判断结束的能力类型；
+ 如果当前 `MagicCircleState` 不是 `Active`，直接返回；
+ 如果能力被取消，按照魔法阵的取消策略处理，不能默认当作正常释放完成；
+ 如果结束的是指定的魔法能力，调用 `FinishMagicCircle`。

bool ActivateAbilitiesByTag(const FGameplayTag& ActivationTag)
+ 仅服务器调用；
+ 遍历该 ASC 的 `ActivatableAbilities.Items`，查找动态 Spec Tags 中精确匹配 `ActivationTag` 的所有 GA 句柄；
+ 先收集匹配的 `FGameplayAbilitySpecHandle`，再逐个调用 `TryActivateAbility`，避免激活过程中修改能力列表影响遍历；
+ 返回是否至少有一个 GA 成功激活；
+ 不调用 `AbilityInputTagPressed` 或 `ProcessAbilityInput`，不将魔法阵内部调度伪装成玩家输入。

void ActivateMagic(const FGameplayTag& ActivationTag)
+ 仅允许服务器执行，并校验当前状态为 `Ready`；
+ 调用 `ActivateAbilitiesByTag(ActivationTag)` 激活该阶段的一个或多个 GA；
+ 只有能力激活成功后，才将状态切换为 `Active` 并调用 `OnMagicCircleStateChanged`；激活失败时保持 `Ready` 或进入明确的失败状态；
+ 能力结束后的状态推进由 `OnAbilityFinished` 和共用的 `FinishMagicCircle` 处理。

### 魔法阵结束策略

魔法阵结束必须最终通过同一个 `FinishMagicCircle` 函数完成，确保状态切换、计时器清理和 `OnMagicCircleStateChanged` 调用只执行一次。结束条件可以由生命周期时间或指定魔法能力结束触发。

#### 方式一：LifeTime 结束

`LifeTime` 从 `Ready` 阶段开始计时，而不是从 Actor 生成时开始计时。`LifeTime` 小于等于 0 时表示不启用生命周期计时器，必须由其它结束条件销毁实例。

```cpp
UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
float LifeTime
```

函数：

```cpp
void StartLifeTimeTimer()
```

+ 仅服务器调用；
+ 在 `Ready` 状态开始计时，并绑定 `OnMagicCircleLifeTimeEnded`；
+ `Finished` 或 `Destroyed` 状态不再启动计时器。

```cpp
void OnMagicCircleLifeTimeEnded()
```

+ 仅服务器调用；
+ 如果当前状态为 `Finished` 或 `Destroyed`，直接返回；
+ 如果当前状态为 `Active`，记录 `bLifeTimeExpired = true` 并等待指定的魔法能力结束，不能简单丢弃这次回调；
+ 如果当前状态为 `Ready`，调用 `FinishMagicCircle`；
+ 如果没有配置指定的结束能力，则生命周期到期时应直接调用 `FinishMagicCircle`，不能让实例永久停留在 `Active`。

#### 方式二：指定能力结束

实例绑定 ASC 的 `OnAbilityEnded` 数据委托。UE 5.6 中该回调接收 `const FAbilityEndedData&`，其中包含结束能力对象和 `bWasCancelled`。

```cpp
UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
TSubclassOf<UMalogicGameplayAbility> FinishAbilityClass
```

函数：

```cpp
void OnAbilityFinished(const FAbilityEndedData& AbilityEndedData)
```
+ 仅服务器处理；
+ 在 ASC 初始化并授予 AbilitySet 后，仅当服务器上的 `FinishAbilityClass` 不为空时绑定到 `AbilitySystemComponent->OnAbilityEnded`；
+ 判断`AbilityEndedData.AbilityThatEnded->GetClass() == FinishAbilityClass`选择结束
+ 如果结束的是指定的魔法能力，调用 `FinishMagicCircle`。
+ 在 `EndPlay` 或 ASC 反初始化时解除绑定；
+ 如果 `LifeTime` 已经到期并记录了 `bLifeTimeExpired`，指定能力在 `Active` 状态结束后立即调用 `FinishMagicCircle`。


#### 共用结束函数


void FinishMagicCircle()
+ 仅服务器调用，并保证幂等；
+ 只允许从 `Ready` 或 `Active` 进入 `Finished`；
+ 清理 `LifeTime` 计时器；
+ 将 `MagicCircleState` 切换为 `Finished`，调用 `OnMagicCircleStateChanged`；
+ 如果LifetimeStrategy为OnceAfterSomeGA，那么就要推进魔法阵生命状态到Destroyed

void HandleOutOfHealth()
+ 仅服务器处理；
+ 将状态切换为 `Destroyed`，调用 `OnMagicCircleStateChanged`；
+ 由 `OnMagicCircleStateChanged` 的 `Destroyed` 分支按顺序调度销毁前能力、取消其它能力、回收 GrantedHandles 并销毁 Actor；
+ 客户端通过 `OnRep_MagicCircleState` 播放受击结束和销毁表现。

void EndPlay(const EEndPlayReason::Type EndPlayReason)
+ 解除 `OnAbilityEnded` 委托、取消所有计时器并清理实例对外部对象的引用；
+ 确保 ASC 和 AbilitySet 的运行时资源不会在 Actor 销毁后残留。

---

# MagicCircleDefinition
概述：存储一种魔法阵的静态配置。Definition 不保存玩家、快捷栏或施法过程中的运行时状态，也不作为运行时复制对象使用。

基类：UObject
UCLASS(Blueprintable, Const, Abstract, BlueprintType)

设计约定：

+ 每一种魔法阵创建一个 Blueprint 子类，运行时通过 `TSubclassOf<UMagicCircleDefinition>` 持有该类型。
+ 需要读取配置时，通过该类的 CDO 获取 Definition 数据。
+ Definition 内的引用均为静态配置引用；运行时生成的 `MagicCircleInstance`、AbilitySpec 和 GrantedHandles 不保存在 Definition 中。

## 成员
变量：

TSubclassOf<AMalogicMagicCircleInstance> MagicCircleToSpawn //部署时生成的魔法阵实例类型

FInputMappingContextAndPriority DeploymentInputMapping //装备该魔法阵时追加的本地输入映射

bool bIsPreDeploy //是否需要显示本地预部署轮廓

bool bIsLifetimeFollowInstigator //魔法阵是否随玩家死亡而销毁（在部署的时候，该选项为true的MagicCircleInstance指针会被添加到MagicCircleManagerComponent成员数组TArray<TObjectPtr<MagicCircleInstance>> MagicCircleFollowPlayerLifetime中

float BaseBuildingTime //魔法阵构建的基本时间，在MalogicGA_(MagicName)Deploy中会根据武器属性以及一些可能的状态计算魔法阵的实际构建时间

TSubclassOf<AActor> PreviewActor //预部署时显示的本地轮廓 Actor 类型

float BaseMaxDeployDistance //魔法阵最大释放距离

TObjectPtr<const UAbilitySet> AbilitySetForPlayer //赋予玩家部署该魔法阵的能力

TObjectPtr<const UAbilitySet> AbilitySetForMagicCircle //赋予魔法阵实例的 AbilitySet，包括属性集、初始化 GE 和魔法能力

## 运行时与网络

+ Definition 类引用可以作为 `UPROPERTY(Replicated)` 成员复制；客户端必须能够加载对应的 Blueprint 类。
+ 快捷栏和管理器复制的是 Definition 类引用及当前索引，不复制 Definition CDO 的运行时副本。
+ 如果只使用硬引用，引用链必须能够被 Unreal Cooker 收集；不能依赖服务器运行时临时拼接的类路径。

函数：

---

# MagicCircleManagerComponent
## 概述
仿照 EquipmentManagerComponent 的思路。虽然只允许装备一个当前魔法阵，不保留 EquipmentList，但当前装备项仍需要像一条 EquipmentEntry 一样，完整保存 Definition、服务器侧授予的 AbilitySet 句柄和本地侧已安装的输入映射。

装备与卸载必须成对处理：装备新魔法阵前先完整卸载旧魔法阵，不能只覆盖 EquippedMagicCircle 指针。

基类：PawnComponent

职责：

+ 负责持有当前装备的魔法阵
+ 在服务器上装备/卸载魔法阵，并管理 AbilitySet 的授予与回收
+ 在本地受控玩家上安装/移除 Definition 的输入映射
+ 在复制、重生、重新 Possess 后，将当前装备状态与本地输入、预部署表现重新同步

## 设计原则

+ EquippedMagicCircle 是服务器权威状态。快捷栏选择请求需要由拥有该 Pawn 的客户端发送到服务器，服务器确认可选择后才更新当前装备；客户端不能自行授予 AbilitySet 或生成可造成伤害的魔法阵。
+ 每次将 AbilitySet 赋予 OwnerASC 时，必须保存对应的 FAbilitySet_GrantedHandles。卸载时仅通过这组句柄回收本次授予的 GA、GE 和 AttributeSet，不能按 Ability 类或 InputTag 做全局清理。
+ 授予部署 GA 时，将本次装备的 MagicCircleDefinition CDO 作为 AbilitySpec 的 SourceObject。部署 GA 激活后读取该快照，而不是再次读取 EquippedMagicCircle；这样在施法过程中切换快捷栏也不会改变本次施法的定义。
+ Input Mapping 不是网络状态，只能由本地受控玩家的 Enhanced Input Local Player Subsystem 安装和移除。服务器复制 EquippedMagicCircle，客户端在 OnRep_EquippedMagicCircle 中对齐本地输入状态。
+ MagicCircleDefinition 中的 DeploymentInputMapping 仅用于该魔法阵特有的附加输入；
+ 本组件卸载的是“玩家持有的部署能力”和预部署表现，不默认销毁已生成的 MagicCircleInstance。已生成实例是否在施法者切换魔法阵、卸下法杖或死亡时销毁，由实例自身的生命周期策略明确决定。
+ MagicCircleManagerComponent 不直接实现预览位置和目标计算。管理器提供非复制的 `OnMagicCircleDefinitionChanged` 装备变化通知，参数为当前 Definition 类，卸载时传入空值；服务器装备/卸载以及客户端 `OnRep_EquippedMagicCircle` 对齐状态后都触发该通知。
+ MagicCircleDeployComponent 在初始化时监听该通知，并根据 Definition 的 `bIsPreDeploy` 和 `PreviewActor` 创建或清理预部署预览。管理器只负责发布装备状态，预览生命周期仍由 DeployComponent 负责。

## 成员
变量：

UPROPERTY(ReplicatedUsing=OnRep_EquippedMagicCircle)
TSubclassOf<UMagicCircleDefinition> EquippedMagicCircle //当前装备的魔法阵类型；服务器权威，客户端用于本地表现同步

FAbilitySet_GrantedHandles EquippedAbilitySetHandles //仅服务器保存；记录当前 Definition 赋予 OwnerASC 的所有内容

FInputMappingContextAndPriority AppliedInputMapping //仅本地保存；记录当前实际加入 Local Player Subsystem 的映射，供卸载时精确移除

DECLARE_MULTICAST_DELEGATE_OneParam(FMagicCircleDefinitionChanged, TSubclassOf<UMagicCircleDefinition>);
FMagicCircleDefinitionChanged OnMagicCircleDefinitionChanged //非复制；当前 Definition 变化时通知本 Pawn 上的 DeployComponent

函数：

public:

UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
bool EquipMagicCircle(TSubclassOf<UMagicCircleDefinition> NewMagicCircle);

UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
void UnequipMagicCircle();

protected:

void ApplyAbilitySetOnServer(TSubclassOf<UMagicCircleDefinition> MagicCircle);

void RemoveAbilitySetOnServer();

void ApplyInputMappingForLocalPlayer(TSubclassOf<UMagicCircleDefinition> MagicCircle);

void RemoveInputMappingForLocalPlayer();

UFUNCTION()
void OnRep_EquippedMagicCircle(TSubclassOf<UMagicCircleDefinition> PreviousMagicCircle);

DeployComponent 在 BeginPlay 绑定 `OnMagicCircleDefinitionChanged`，在 EndPlay 解除绑定。委托传入当前 Definition 类，卸载时传入空值；DeployComponent 以实际本地状态为准，不依赖 PreviousMagicCircle。

## 装备流程

1. MagicCircleQuickBarComponent 选中槽位后，向服务器请求装备该 MagicCircleDefinition。
2. 服务器验证该 Definition 属于该玩家的可用槽位、玩家状态允许切换，且与当前装备不同；验证失败时不改变状态。
3. 若已有当前装备，先调用 UnequipMagicCircle：取消仍在进行的玩家侧部署/瞄准能力，调用 EquippedAbilitySetHandles.TakeFromAbilitySystem(OwnerASC) 回收旧 AbilitySet，然后清空服务器侧句柄和 EquippedMagicCircle。
4. 服务器从新 Definition 类获取 CDO，调用其 AbilitySet.GiveToAbilitySystem(OwnerASC, &EquippedAbilitySetHandles, SourceObject)，其中 SourceObject 为该 Definition CDO；随后设置 EquippedMagicCircle 并复制该状态。
5. 本地受控玩家在状态变更后安装所需 DeploymentInputMapping，并通过 `OnMagicCircleDefinitionChanged` 通知 DeployComponent 创建或更新本地预部署轮廓。若 Pawn、Controller 或 Local Player Subsystem 尚未就绪，则延迟到它们就绪后执行，不在远端 Pawn 上安装映射或预览。

## 卸载流程

1. 本地受控玩家移除 AppliedInputMapping，停止并销毁本地预部署轮廓，清除待处理的输入状态。
2. 服务器取消旧 Definition 授予且仍在运行的部署/瞄准能力；不影响已经独立运行的 MagicCircleInstance，除非其生命周期策略明确要求随装备卸载而结束。
3. 服务器调用 EquippedAbilitySetHandles.TakeFromAbilitySystem(OwnerASC)，回收旧 Definition 授予的 GA、GE 和 AttributeSet，并重置句柄。
4. 清空 EquippedMagicCircle 并复制；客户端 OnRep 进行一次幂等的本地输入状态对齐，以应对重生、重新 Possess 和网络延迟。

## 异常与边界情况

+ 选择同一个槽位不重复授予 AbilitySet 或重复安装输入映射。
+ 切换法杖、角色死亡、Pawn 被销毁、UninitializeComponent 和重新 Possess 都必须走同一套卸载或重同步路径。
+ 若 OwnerASC 不存在或尚未完成初始化，服务器不授予 AbilitySet；保留待装备状态，在 ASC 就绪后重试。不得将能力授予临时或错误的 ASC。
+ OnRep_EquippedMagicCircle 必须先移除实际已安装的旧映射，再安装新映射；不要假设 PreviousMagicCircle 一定与本地实际状态一致。

# MagicCircleDeployComponent
## 概述

负责本地玩家的魔法阵预部署预览、部署目标计算以及部署距离调整。该组件只负责客户端表现和目标数据准备，不负责在服务器生成 MagicCircleInstance，也不负责最终的部署合法性判断。

基类：UPawnComponent（BlueprintType, Meta = (BlueprintSpawnableComponent)）

## 设计边界

+ 只有本地控制的 Pawn 创建和更新 PreDeployMagicCircle；
+ MagicCircleManagerComponent 在装备状态变化时通过 `OnMagicCircleDefinitionChanged` 传递当前 MagicCircleDefinition；DeployComponent 根据 Definition 的 `bIsPreDeploy`、`PreviewActor`、`BaseMaxDeployDistance` 和 `DeployStrategy` 创建或清理预览。切换、卸载、死亡、失去控制权或 Pawn 销毁时清理旧预览。
+ 预览位置的距离基于玩家相机或玩家角色，这由。默认位置可以按“起源位置 + 起源瞄准方向 * DistanceFromCamera”计算，再由部署策略进行射线检测、贴地、表面法线对齐或其它修正。
+ 部署位置计算规则由成员DeployStrategy决定，MagicCirclePreview 只负责显示。
+ 预览更新主要由组件在计算好部署位置后驱动更新（修改MagicCirclePreview的Transform）。
+ 玩家激活 MalogicGA_MagicDeploy 时，GA 从该组件获取当前MagicCirclePreview的Transform（判断bCanBeDeployed），立即复制为 TargetData 快照并提交给服务器。之后预览 Actor 的继续移动、装备切换或输入变化不得修改已经提交的部署请求。

## 输入约定

部署距离的增加、减少以及其它预部署操作使用初始 InputConfig 和 HeroComponent 完成 NativeAction 到组件回调的绑定。MagicCircleDefinition 中的 DeploymentInputMapping 只负责在装备该魔法阵时动态调整按键到既有 InputAction 的映射。

如果某个 MagicCircleDefinition 使用了新的 InputAction，则该 InputAction 必须在初始 InputConfig 中完成回调绑定，或者由输入系统提供成对的动态绑定和解绑；仅添加 DeploymentInputMapping 不会自动创建回调。切换或卸载魔法阵时必须移除旧的映射，避免多个 Definition 同时响应同一输入。

## 部署策略

当前阶段使用枚举 `EMagicCircleDeployStrategy` 描述部署方式。该枚举由 MagicCircleDefinition 保存，DeployComponent 在更新预览时根据枚举选择对应的目标计算分支。

```cpp
UENUM(BlueprintType)
enum class EMagicCircleDeployStrategy : uint8
{
	CameraRaycast,  // 从相机发出射线，使用命中点和表面法线计算部署位置与旋转。
	CameraForward,  // 以相机位置和瞄准方向计算部署位置。
	PawnForward     // 以玩家角色位置和朝向计算部署位置，不以相机为中心。
};
```
UpdatePreDeployMagicCircle函数中会根据DeployStrategy成员，进行不同的计算逻辑以更新MagicCirclePreview的位置。

当前需求下不单独创建 `UMagicCircleDeployStrategy` UObject，避免为了尚未明确的策略差异引入额外运行时对象和初始化接口。后续如果策略参数或目标过滤逻辑显著复杂，再将枚举分支迁移为独立策略类。

## 生命周期

预部署状态建议遵循以下规则：

+ 装备需要预部署的 Definition：创建并显示预览。
+ 装备不需要预部署的 Definition：销毁预览，部署由对应策略或 GA 直接处理。
+ GA 激活：捕获 MagicCirclePreview 位置并提交 TargetData。
+ 服务器确认部署成功：清理预览。
+ 部署失败或服务器拒绝：保留预览，并允许玩家重新调整距离或目标后再次提交。

## 成员

变量：

TObjectPtr<AActor> MagicCirclePreview //本地预部署预览 Actor

float DistanceFromSource //相对于基准点的当前部署距离

//float MinDeployDistance = 1.0f //部署距离下限；没有部署距离下限，大于等于0即可。

float BaseMaxDeployDistance //基础部署距离上限，在Deployment根据魔法阵定义，以及玩家状态计算出的当前实际部署距离

float MaxDeployDistanceRatio = 1.0f //部署距离上限的系数，如果有武器或者其它buff修改则直接修改这个值

float DeployDistanceStep = 10.f //每次调整的距离步长，这是硬编码的固定值，不会随游戏逻辑发生变化

EMagicCircleDeployStrategy DeployStrategy //当前魔法阵的部署目标计算策略

bool bCanBeDeployed //当前Preview位置是否能部署魔法阵

## 函数

public：

void HandleMagicCirclePreDeploy(TSubclassOf<UMagicCircleDefinition> MagicCircleDefinition)
+ 仅本地控制端执行；销毁旧预览并创建新的 PreDeployMagicCircle。
+ 从 Definition CDO 读取 `bIsPreDeploy`、`PreviewActor`、`BaseMaxDeployDistance` 和 `DeployStrategy`；不需要预部署或没有 PreviewActor 时直接清理并返回。
+ 将成员BaseMaxDeployDistance 初始化为 Definition 的 BaseMaxDeployDistance（或许该项还会根据别的状态也做出覆写），并根据当前状态更新MaxDeployDistanceRatio
+ 保存当前 DeployStrategy。
+ 创建后立即计算一次预览位置，避免等待下一帧时出现空目标。

void UpdatePreDeployMagicCircle(float DeltaTime)
+ 根据相机位置、瞄准方向、DistanceFromCamera 和 DeployStrategy 更新预览；其中 `CameraRaycast`、`CameraForward` 和 `PawnForward` 分别执行对应的目标计算规则。
+ 同步更新 CurrentDeployTarget；无效目标不能作为部署请求提交。

void IncreaseDeployDistance()
+ 根据DeployDistanceStep增加一次DistanceFromSource

void DecreaseDeployDistance()
+ 根据DeployDistanceStep减少一次DistanceFromSource

void ClampDeployDistance(float NewDistance)

+ 将距离限制在 MinDeployDistance 和 MaxDeployDistance 范围内。

void ClearPreDeployMagicCircle()

+ 在切换、卸载、死亡、失去控制权或服务器确认部署成功后销毁预览并清理当前目标。
+ 部署失败或服务器拒绝时不调用该函数，保留预览供玩家修正后重试。

## 注意事项
部署位置是否有效合理非常重要，可以成功部署的魔法阵一定不会被当帧中静态物体所阻碍施法


# MagicCircleQuickBarComponent
## 概述
仿照 QuickBarComponent 的思路，保存可快捷选择的魔法阵 Definition 类型。由于 Definition 是无状态的静态 CDO，快捷栏不创建或复制 Definition 实例。

基类：ControllerComponent

职责：

+ 持有可快捷选择的魔法阵类型数组（通常为 3~5 个）。
+ 复制槽位和当前槽位索引，使服务器和拥有者客户端使用同一选择结果。
+ 通过 Server RPC 请求切换当前槽位。
+ 切换槽位时调用 Pawn 上的 MagicCircleManagerComponent，由管理器负责 AbilitySet 和 InputMapping 的装备/回收。
+ 只在本地客户端更新快捷栏 UI 和预部署轮廓，不在客户端直接生成权威魔法阵。

## 成员

```cpp
UPROPERTY(EditDefaultsOnly)
int32 NumSlots = 3;

UPROPERTY(ReplicatedUsing = OnRep_Slots)
TArray<TSubclassOf<UMalogicMagicCircleDefinition>> Slots;

UPROPERTY(ReplicatedUsing = OnRep_ActiveSlotIndex)
int32 ActiveSlotIndex = INDEX_NONE;

UPROPERTY(BlueprintAssignable)
FMagicCircleQuickBarSlotsChanged OnSlotsChanged;

UPROPERTY(BlueprintAssignable)
FMagicCircleQuickBarActiveSlotChanged OnActiveSlotIndexChanged;
```

`Slots` 与 `ActiveSlotIndex` 使用普通组件复制，不限制为 OwnerOnly，为其它客户端观察选择状态和后续扩展保留数据。快捷栏只保存和复制 `TSubclassOf<UMalogicMagicCircleDefinition>`；需要读取配置时使用对应类的 CDO。

`OnSlotsChanged` 向 UI 提供完整槽位数组，`OnActiveSlotIndexChanged` 提供当前索引。服务器的显式修改和客户端的 `OnRep` 都会触发对应委托，UI 可直接绑定。

函数：

```cpp
UFUNCTION(Server, Reliable, BlueprintCallable)
void SetActiveSlotIndex(int32 NewIndex);

UFUNCTION(BlueprintCallable)
void CycleActiveSlotForward();

UFUNCTION(BlueprintCallable)
void CycleActiveSlotBackward();

UFUNCTION(BlueprintPure)
TSubclassOf<UMalogicMagicCircleDefinition> GetActiveSlotMagicCircle() const;

UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
void AddToSlot(int32 SlotIndex, TSubclassOf<UMalogicMagicCircleDefinition> MagicCircleDefinition);

UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
TSubclassOf<UMalogicMagicCircleDefinition> RemoveFromSlot(int32 SlotIndex);
```

私有流程函数：

```cpp
void EquipMagicCircleInSlot();
void UnequipMagicCircleInSlot();
UMagicCircleManagerComponent* FindMagicCircleManager() const;
```

快捷栏负责当前槽位的装备编排：`EquipMagicCircleInSlot` 从当前槽位读取 Definition，再调用 Pawn 上 Manager 的 `EquipMagicCircle`；`UnequipMagicCircleInSlot` 调用 Manager 的 `UnequipMagicCircle`。Manager 仍是 AbilitySet、输入映射和实际装备状态的唯一所有者。

`AddToSlot` 与 `UMalogicQuickBarComponent::AddItemToSlot` 语义一致：只允许向有效的空槽写入非空 Definition，已有槽位不会被覆盖。替换槽位必须先调用 `RemoveFromSlot`。

`RemoveFromSlot` 与 `UMalogicQuickBarComponent::RemoveItemFromSlot` 语义一致：返回被移除的 Definition；无效或空槽返回 `nullptr`。清除当前激活槽位时，先卸载魔法阵，将 `ActiveSlotIndex` 重置为 `INDEX_NONE`，再清空槽位。

```cpp
UFUNCTION()
void OnRep_Slots();

UFUNCTION()
void OnRep_ActiveSlotIndex();
```

## 切换流程

1. 本地输入请求 `SetActiveSlotIndex`。
2. 服务器校验索引和槽位内容。
3. 服务器调用 `UnequipMagicCircleInSlot`，更新 `ActiveSlotIndex`，再调用 `EquipMagicCircleInSlot`；两个辅助函数通过 Pawn 上的 `MagicCircleManagerComponent` 卸载旧 Definition 并装备新槽位的 Definition。
4. 复制索引和槽位变化；客户端在 `OnRep` 中刷新 UI、预部署轮廓和本地输入表现。

## 设计边界

+ `Slots` 只保存 `TSubclassOf<UMagicCircleDefinition>`，不保存 `TObjectPtr<UMagicCircleDefinition>` 运行时对象。
+ Definition 不保存充能、升级、符文、耐久等实例状态；如果未来出现这些需求，再单独引入 MagicCircleItemInstance，而不是修改 Definition 的职责。

# MagicWeaponInstance
## 概述
基类：WeaponInstance

职责：

提供施放魔法相关的一些属性，如施法距离。

在装备武器的时候，注入指定的InputMapping。（此处需要对InputMapping的获取方式做设计）

在卸载武器的时候，卸载对应的 InputMapping。

## 武器实例的输入映射获取及注入
首先自定义InventoryFragment_InputMapping。其中定义了一个成员TSoftPtr<UInputMappingContext> InputMappingContext(这里需要用软指针么)
在EquipmentInstance中，有成员UObject* Instigator，它通常在QuickBarComponent中装备Equipment时，被设置为InventoryItemInstance* SlotItem。可以通过InventoryItemInstance类中定义的FindFragmentByClass函数找到对应的Frgament。
所以在MagicWeaponInstance中，重写OnEquipped函数，调用父类逻辑Super，然后调用Instigator的FindFragmentByClass函数找到InventoryFragment_InputMapping，调用GetPawn获取到持有该武器实例的Pawn并且添加InputMapping。

## 成员
变量：

float DeployDistanceRatio = 1.0f //部署魔法距离上限加成系数

函数：

OnEquipped()

OnUnEquipped()

# MalogicGA_(DeployMethod)Deploy
## 概述
基类：UMalogicGameplayAbility

职责：

+ 从自身 AbilitySpec 的 `SourceObject` 获取本次施法绑定的 MagicCircleDefinition CDO，不重新查询 MagicCircleManagerComponent 的当前装备；
+ 根据部署策略进行射线检测、目标过滤、位置约束和朝向计算；
+ 读取 Definition 的 `BaseBuildingTime`，结合 MagicWeapon 属性、GameplayEffect、GameplayTag 和其它状态计算 `ActualBuildingTime`；
+ 对 `ActualBuildingTime` 进行最小值和最大值限制，不能通过非法值绕过 `Building` 阶段；
+ 在服务器上使用 Deferred Spawn 创建 `AMalogicMagicCircleInstance`，并在 `FinishSpawning` 前传入 Definition、部署者、部署变换和 `ActualBuildingTime`；
+ 验证部署请求的权限、距离、资源和目标数据，验证通过后才生成实例。

函数：

`CalculateActualBuildingTime(const UMagicCircleDefinition* Definition)`
+ 仅服务器调用；
+ 以 Definition 的 `BaseBuildingTime` 为基础值；
+ 结合当前 MagicWeapon、来源 ASC 上的属性和标签，以及其它影响施法时间的状态计算最终值；
+ 返回经过限制后的 `ActualBuildingTime`。

`SpawnMagicCircleInstance(const UMagicCircleDefinition* Definition, const FTransform& DeployTransform, float ActualBuildingTime)`
+ 仅服务器调用；
+ 使用 Definition 的 `MagicCircleToSpawn` 创建实例；
+ 调用 `InitializeFromDefinition(Definition, Instigator, ActualBuildingTime)`；
+ 完成实例生成后由实例自身进入 `Building`，部署 GA 不直接激活实例魔法能力。

问题：

对于不同的魔法，存在不同的部署方式，如有的魔法需要发出射线检测获取命中点。部署 GA 可以通过部署策略或子类复用，魔法阵实例类型和构建时间均从 Definition 和运行时参数获取，不能硬编码在 GA 中。

# MalogicGA_(MagicName)
## 概述
基类：UMalogicGameplayAbility_FromEquipment

职责：由 MagicCircle 持有的 GA，由 MagicCircle 选择时机激活

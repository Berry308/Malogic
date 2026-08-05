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
生成 MagicCircleInstance 后，根据 MagicCircleDefinition 中定义的 AbilitySet 对其 ASC 进行初始化，包括初始化血量的 GE 和赋予 MagicCircleInstance 本身的魔法能力。

### 激活魔法阵
激活方式：

+ 魔法阵自身自动激活
+ 由玩家指定，手动激活（扩展）

### 魔法阵释放魔法（有可能存在不自动释放的魔法）
在激活后，所有魔法阵都有构建的动画（目前的想法是控制透明度从 0 到 1）

在播放完魔法阵构建的动画后，魔法阵就会自动施放魔法

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
基类：Actor

职责：

+ 作为魔法阵的实例，由玩家部署与创建
+ 持有 AbilitySystemComponent

<font style="color:#DF2A3F;">注意：</font>

部分魔法不和魔法阵绑定生命周期，有些则绑定

## 成员
变量：

bool bIsInstantActivate //是否是即时施放魔法

float BaseActivateTime //从魔法阵显形到魔法释放的基础时间

TObjectPtr<UMalogicAbilitySystemComponent> AbilitySystemComponent

TObjectPtr<UHealthComponent> HealthComponent

TObjectPtr<UMalogicHealthSet> HealthSet

TObjectPtr<UMalogicCombatSet> CombatSet 

函数：

public:

UMalogicMagicCircleInstance()

+ 创建AbilitySystemComponent
+ 创建 HealthSet
+ 设置 Replication

void K2_BeginPlay() （蓝图内实现）

+ 蓝图本地播放魔法阵启动动画
+ 在动画播放完毕后，回调 ActivateMagic。

void ActivateMagic()

+ 激活当前 ASC 中的 ActivatableAbilities.Items（在初始化的时候 MagicCircleDefinition 会将 AbilitySet 赋予 MagicCircleInstance 的 ASC。

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

TSubclassOf<UMagicCircleInstance> MagicCircleToSpawn //部署时生成的魔法阵实例类型

FInputMappingContextAndPriority InputMapping //装备该魔法阵时追加的本地输入映射

bool bIsPreDeploy //是否需要显示本地预部署轮廓

TSubclassOf<AActor> PreviewActor //预部署时显示的本地轮廓 Actor 类型

float DefaultDistance //不需要预部署输入时的默认生成距离（相对于玩家角色）

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
+ 授予部署 GA 时，将本次装备的 MagicCircleDefinition CDO 作为 AbilitySpec 的 SourceObject，或在能力激活时将 Definition 类复制到本次施法的数据中。部署 GA 激活后读取该快照，而不是再次读取 EquippedMagicCircle；这样在施法过程中切换快捷栏也不会改变本次施法的定义。
+ Input Mapping 不是网络状态，只能由本地受控玩家的 Enhanced Input Local Player Subsystem 安装和移除。服务器复制 EquippedMagicCircle，客户端在 OnRep_EquippedMagicCircle 中对齐本地输入状态。
+ 法杖提供通用输入映射，例如左键部署、取消部署和快捷栏选择。MagicCircleDefinition 中的 InputMapping 仅用于该魔法阵特有的附加输入；若复用同一输入，必须定义优先级和互斥关系，避免多个部署 GA 同时响应左键。
+ 本组件卸载的是“玩家持有的部署能力”和预部署表现，不默认销毁已生成的 MagicCircleInstance。已生成实例是否在施法者切换魔法阵、卸下法杖或死亡时销毁，由实例自身的生命周期策略明确决定。

## 成员
变量：

UPROPERTY(ReplicatedUsing=OnRep_EquippedMagicCircle)
TSubclassOf<UMagicCircleDefinition> EquippedMagicCircle //当前装备的魔法阵类型；服务器权威，客户端用于本地表现同步

FAbilitySet_GrantedHandles EquippedAbilitySetHandles //仅服务器保存；记录当前 Definition 赋予 OwnerASC 的所有内容

FInputMappingContextAndPriority AppliedInputMapping //仅本地保存；记录当前实际加入 Local Player Subsystem 的映射，供卸载时精确移除

TSubclassOf<UMagicCircleDefinition> PendingInputMappingDefinition //Pawn 尚未具备本地输入子系统时暂存，初始化完成后补装

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

## 装备流程

1. MagicCircleQuickBarComponent 选中槽位后，向服务器请求装备该 MagicCircleDefinition。
2. 服务器验证该 Definition 属于该玩家的可用槽位、玩家状态允许切换，且与当前装备不同；验证失败时不改变状态。
3. 若已有当前装备，先调用 UnequipMagicCircle：取消仍在进行的玩家侧部署/瞄准能力，调用 EquippedAbilitySetHandles.TakeFromAbilitySystem(OwnerASC) 回收旧 AbilitySet，然后清空服务器侧句柄和 EquippedMagicCircle。
4. 服务器从新 Definition 类获取 CDO，调用其 AbilitySet.GiveToAbilitySystem(OwnerASC, &EquippedAbilitySetHandles, SourceObject)，其中 SourceObject 为该 Definition CDO；随后设置 EquippedMagicCircle 并复制该状态。
5. 本地受控玩家在状态变更后安装所需输入映射，并创建或更新本地预部署轮廓。若 Pawn、Controller 或 Local Player Subsystem 尚未就绪，则延迟到它们就绪后执行，不在远端 Pawn 上安装映射或预览。

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

private:

UPROPERTY(ReplicatedUsing=OnRep_Slots)
TArray<TSubclassOf<UMagicCircleDefinition>> Slots;

UPROPERTY(ReplicatedUsing=OnRep_ActiveSlotIndex)
int32 ActiveSlotIndex = -1;

函数：

UFUNCTION(Server, Reliable, BlueprintCallable)
void SetActiveSlotIndex(int32 NewIndex);

UFUNCTION(BlueprintCallable)
void CycleActiveSlotForward();

UFUNCTION(BlueprintCallable)
void CycleActiveSlotBackward();

UFUNCTION(BlueprintPure)
TSubclassOf<UMagicCircleDefinition> GetActiveSlotMagicCircle() const;

UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
void SetSlot(int32 SlotIndex, TSubclassOf<UMagicCircleDefinition> MagicCircleDefinition);

UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
void ClearSlot(int32 SlotIndex);

UFUNCTION()
void OnRep_Slots();

UFUNCTION()
void OnRep_ActiveSlotIndex();

## 切换流程

1. 本地输入请求 `SetActiveSlotIndex`。
2. 服务器校验索引和槽位内容。
3. 服务器更新 `ActiveSlotIndex`，先让管理器卸载旧魔法阵，再装备新槽位的 Definition。
4. 复制索引和槽位变化；客户端在 `OnRep` 中刷新 UI、预部署轮廓和本地输入表现。

## 设计边界

+ `Slots` 只保存 `TSubclassOf<UMagicCircleDefinition>`，不保存 `TObjectPtr<UMagicCircleDefinition>` 运行时对象。
+ Definition 不保存充能、升级、符文、耐久等实例状态；如果未来出现这些需求，再单独引入 MagicCircleItemInstance，而不是修改 Definition 的职责。

# MagicWeapon
## 概述
基类：WeaponInstance

职责：

提供施放魔法相关的一些属性，如施法距离。

在装备武器的时候，注入 EquipmentInstance 的 InputMapping。（此前需要对 EquipmentInstance 做一些修改）

在卸载武器的时候，卸载对应的 InputMapping。

## 成员
变量：

TObjectPtr<UActivatableWidget> QuickMagicBar //快捷魔法栏

float DeployDistanceRatio //部署魔法距离上限加成系数

函数：

OnEquipped()

OnUnEquipped()

# MalogicGA_(DeployMethod)Deploy
## 概述
基类：UMalogicGameplayAbility

职责：

+ 从 MalogicMagicCircleManagerComponent 中获取当前装备的 MagicCircleDefinition
+ 

问题：

对于不同的魔法，存在不同的部署方式，如有的魔法需要发出射线检测获取命中点

或许 DeployGA 可以复用，所以命名方式需要更改，魔法阵实例子类需要从别的地方获取而不是硬编码在 GA 中。

# MalogicGA_(MagicName)
## 概述
基类：UMalogicGameplayAbility_FromEquipment

职责：由 MagicCircle 持有的 GA，由 MagicCircle 选择时机激活

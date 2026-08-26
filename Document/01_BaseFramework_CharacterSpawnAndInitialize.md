# 01 - Base Framework Character Spawn And Initialize
## 任务目标
完成角色初始化系统
-角色初始数据定义
-角色初始数据挂载
-角色初始数据注入
-角色生成点定义
-角色生成系统构建

## 设计背景
基于`E:\Unreal Projects\MazeRunner`项目中的初始化系统，但是剔除了Experience和GameFeature系统。
希望能够构建相比起MazeRunner更简洁的数据流动和来源的初始化系统。

主要相关类：MRPlayerStart、MRPlayerSpawningManagerComponent、MRGameMode、MRPawnData

## 设计思路

以下内容记录 MazeRunner 当前的实现事实，不代表 Malogic 的设计方案。

### PawnData 定义

`UMRPawnData` 继承自 `UPrimaryDataAsset`，被声明为不可变数据资产。当前有效字段包括：

- `PawnClass`：生成 Pawn 时使用的类。
- `AbilitySets`：授予 PlayerState 上 Ability System Component 的能力集。
- `TagRelationshipMapping`：Pawn 成为 ASC Avatar 时设置到 ASC 的能力标签关系映射。
- `InputConfig`：本地受控 Pawn 初始化 Enhanced Input 时读取的输入配置。

### MazeRunner 中的来源与选择顺序

`AMRGameMode::GetPawnDataForController` 按以下顺序返回 PawnData：

1. Controller 的 `AMRPlayerState` 已持有的 PawnData。
2. 当前已加载 `UMRExperienceDefinition` 的 `DefaultPawnData`。
3. `UMRAssetManager` 配置的 `DefaultPawnData`；该软引用在访问时同步加载。

第 2 项是 MazeRunner 对 Experience 的直接依赖。Experience 尚未加载且 PlayerState 未持有 PawnData 时，该函数返回空值。

### PawnData 数据流

1. `AMRPlayerState` 在服务器的 `PostInitializeComponents` 中订阅 Experience 加载完成事件；事件触发后通过 `AMRGameMode::GetPawnDataForController` 取得 PawnData，并调用 `SetPawnData`。
2. `AMRPlayerState::SetPawnData` 仅允许 Authority 执行且拒绝重复设置。它保存 PawnData，将其中每个 `AbilitySet` 授予 PlayerState 持有的 ASC，发送 `NAME_MRAbilityReady` 扩展事件，并复制 PawnData。
3. GameMode 在 Experience 已加载后才启动新玩家或为已有无 Pawn 的玩家调用 `RestartPlayer`。生成 Pawn 类时读取 `PawnData->PawnClass`。
4. `AMRGameMode::SpawnDefaultPawnAtTransform` 采用延迟生成；在 `FinishSpawning` 前将同一份 PawnData 写入新 Pawn 的 `MRPawnExtensionComponent`。
5. `MRPawnExtensionComponent` 的 PawnData 同样只由服务器设置并复制。客户端收到复制后会触发 `OnRep_PawnData`，继续检查初始化状态链。
6. PawnExtensionComponent 进入 `DataAvailable` 的前提是已存在 PawnData；对于 Authority 或本地控制的 Pawn，还必须已经拥有 Controller。所有特性达到 `DataAvailable` 后，才能进入 `DataInitialized`。
7. `MRHeroComponent` 在 `DataAvailable -> DataInitialized` 阶段从 PawnExtensionComponent 读取 PawnData：使用 PlayerState 上的 ASC 初始化 Pawn 为 Avatar，并在本地输入初始化时读取 `InputConfig`。ASC 初始化期间由 PawnExtensionComponent 读取 `TagRelationshipMapping` 并设置到 ASC。

### 数据归属与生成位置

- `PlayerState` 保存跨死亡和 Pawn 切换持续存在的数据；MazeRunner 的 ASC 和属性集位于 PlayerState。
- `PawnExtensionComponent` 保存当前 Pawn 对应的数据，并在该 Pawn 结束时解除 ASC Avatar 关系。
- PlayerState 与 PawnExtensionComponent 均复制 PawnData；PlayerState 的 `OnRep_PawnData` 当前没有额外处理，PawnExtensionComponent 的 `OnRep_PawnData` 会推进初始化检查。
- `MRPlayerStart` 和 `MRPlayerSpawningManagerComponent` 负责确定生成位置。GameMode 的 `ChoosePlayerStart` 将位置选择委托给生成管理组件；该路径不读取或写入 PawnData。


## 任务步骤
先决定PawnData的数据挂载处，以及数据流向与注入
然后构建角色生成系统

## 待确认问题

- Malogic 的 PawnData 来源、优先级和缺失时行为是否需要与 MazeRunner 的三层来源保持一致；MazeRunner 的第 2 层来源依赖 Experience。
- Malogic 是否需要同时在跨 Pawn 生命周期的持有者与当前 Pawn 上保存并复制同一份 PawnData。
- 删除 Experience 初始化链后，PawnData 何时可用，以及角色生成应等待的前置条件是什么。
- PawnData 中能力集、标签关系映射和输入配置分别由哪些现有 Malogic 组件消费。
- 角色重生、重新控制 Pawn 和客户端延迟复制期间，PawnData 与 ASC Avatar 关系需要满足哪些验证条件。
- MalogicHeroComponent的InputConfig来源（从成员DefaultInputConfig，或从PawnData中获取）

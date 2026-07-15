# 01 - Base Framework

## 任务目标

将 `E:\Unreal Projects\MazeRunner` 中已验证的 Gameplay Ability System (GAS) 基础能力迁移并适配到 Malogic。

MazeRunner 是基于 Lyra 搭建的旧项目。本任务只复用其中与 GAS、角色能力、属性、输入标签和网络同步直接相关的实现；迁移结果必须符合 Malogic 当前的模块边界、命名和初始化方式，不能将 Lyra 的框架依赖一并带入。

## 已有基础

Malogic 当前已具备下列对应骨架，后续工作应以补全、校验和适配为主，避免重复创建平行系统：

| Malogic 模块 | MazeRunner 对应来源 | 说明 |
| --- | --- | --- |
| `AbilitySystem/MalogicAbilitySystemComponent` | `AbilitySystem/MRAbilitySystemComponent` | 能力输入、激活组、标签关系、失败通知和动态标签效果。 |
| `AbilitySystem/Abilities` | `AbilitySystem/Abilities` | Gameplay Ability 基类、能力消耗及具体能力。 |
| `AbilitySystem/Attributes` | `AbilitySystem/Attributes` | 属性基类、生命和战斗属性集。 |
| `AbilitySystem/AbilitySet` | `AbilitySystem/MRAbilitySet` | 授予/回收能力、属性集和 Gameplay Effect。 |
| `AbilitySystem/MalogicGameplayEffectContext` | `AbilitySystem/MRGameplayEffectContext` | 自定义效果上下文及网络序列化。 |
| `AbilitySystem/Executions` | `AbilitySystem/Executions` | 伤害等 Gameplay Effect Execution Calculation。 |
| `Character`、`Player` | `Character`、`Player` | PlayerState 持有 ASC，角色作为 Avatar 的初始化与输入转发。 |
| `Input`、`MalogicGameplayTags` | `Input`、`MRGameplayTags` | 基于 `InputTag.*` 的输入绑定和原生 Gameplay Tag 定义。 |
| `System/GameplayTagStack` | `System/GameplayTagStack` | 可复制的统计标签栈。 |

## 迁移范围

### 需要迁移或补全

1. GAS 核心：Ability System Component、Gameplay Ability 基类、Ability Set、Ability Cost、Ability Tag Relationship Mapping、Ability Source Interface、Gameplay Effect Context 和 Target Data。
2. 属性与战斗：属性基类、生命/战斗属性集，以及 `MRDamageExecution` 中可独立于 Lyra 的伤害计算逻辑。
3. 角色接入：由 `AMalogicPlayerState` 持有并复制 ASC；在角色被控制、重生或切换 Avatar 时正确调用 `InitAbilityActorInfo`。
4. 能力输入：通过 Enhanced Input 将输入动作转换为 `InputTag.*`，调用 ASC 的按下、释放和逐帧处理接口；保留客户端预测和服务端确认路径。
5. 原生标签与配置：将实际使用的 MazeRunner 标签迁移为 `MalogicGameplayTags` 中的原生标签；仅在需要时修改 Malogic 的 `.ini` 配置。
6. 验证资产：为 Ability Set、Gameplay Effect、输入配置等数据资产提供明确的 Malogic 内容路径或可配置引用，不依赖 MazeRunner 的资产路径。

### 明确不迁移

- Lyra `GameFeatures` / `GameFeatureAction_*` 系统，以及 `GameFeatures` 模块依赖。
- Lyra `Experience`、`ExperienceDefinition`、`ExperienceActionSet`、`ExperienceManager` 和相关加载流程。
- 基于 `UGameFrameworkComponentManager` 的 InitState / feature 注册初始化链，包括 Pawn Extension Component 的 Lyra 初始化职责。
- 仅为 Lyra 样例玩法、UI、武器、AI、关卡或插件服务的代码与资产。

Malogic 已启用 `ModularGameplay` 和 `ModularGameplayActors`。如现有角色组件确有使用组件管理器的局部需求，可以保留其本项目实现，但不得恢复 MazeRunner/Lyra 的全局 InitState 驱动模型。

## 实施步骤

1. 对比两项目的同名 GAS 类，优先完成 API、命名空间、模块引用和 `MR`/`MazeRunner` 残留的清理。
2. 迁移 Malogic 缺失的 GAS 类型，例如伤害执行计算、单目标命中 Target Data、全局 Ability System Globals；每个类型先确认其是否引用 Experience、GameFeature 或 InitState。
3. 适配 PlayerState、Character 和 HeroComponent 的生命周期，使 ASC 的 Owner 始终为 PlayerState、Avatar 为当前 Pawn，并在客户端和服务端均正确初始化。
4. 接通 Enhanced Input 到 `UMalogicAbilitySystemComponent` 的输入标签接口，并在角色 Tick 或等价位置调用 `ProcessAbilityInput`。
5. 补齐原生 Gameplay Tags、数据资产引用和必要配置；禁止硬编码 MazeRunner 内容路径或 tag 字符串。
6. 编译 `MalogicEditor Win64 Development`，随后在编辑器中验证授予能力、输入激活、属性变化、伤害结算、死亡/重生和多人复制。

## 实施约束

- 统一使用 `Malogic` 命名、`MALOGIC_API` 和现有日志分类；清除迁移代码中的 `MR`、`MazeRunner`、`Lyra` 命名及旧分类文本。
- GAS 权威状态只在服务端确认。能力输入、Gameplay Effect、属性和标签变更必须保留 GAS 的预测与复制语义。
- 优先扩展 `UMalogicAbilitySystemComponent`、`UMalogicGameplayAbility` 和现有属性集；不使用平行的自定义战斗状态替代 GAS。
- 不修改 `Plugins/UnLua/**`，不迁移 MazeRunner 的生成目录、临时目录或 Lyra 资产引用。
- 每次迁移以可编译的最小批次提交；若涉及 Blueprint 资产、序列化字段或网络协议变化，先记录兼容性影响。

## 完成标准

- Malogic 不依赖 `GameFeatures` 模块，也不存在 Experience 或 Lyra GameFrameworkManager 初始化链。
- 编辑器目标可成功编译，且不引用 `E:\Unreal Projects\MazeRunner` 中的源文件或内容路径。
- 本地联机测试中，PlayerState 上的 ASC 能在 Pawn 重生/重新控制后保持正确的 Owner/Avatar 关系。
- 输入标签可激活和结束对应能力，能力阻塞、取消、激活组和标签关系按预期工作。
- 属性、伤害、Gameplay Effect 和动态标签在服务端权威下正确复制到客户端。

## 待确认事项

- Damage Execution 是否需要在第一阶段落地，还是仅先完成基础能力与属性框架。
- 当前 Malogic 的角色初始化是否继续使用已有 `UMalogicHeroComponent`，或由 Character/PlayerController 直接承担 ASC Avatar 绑定。
- 首批需要迁移的 Ability Set、Gameplay Effect、Input Config 和相关 Blueprint 数据资产清单。

# UI 基础框架

## 设计目标

使用 Model-View-ViewModel（MVVM）解耦游戏数据、UI 业务逻辑和 Widget 表现层。

```text
C++ Model（Component / Ability System）
    -> Lua Service
    -> C++ ViewModel（FieldNotify）
    -> Widget（MVVM Binding）
```

- Model：由现有 C++ Gameplay 系统维护，例如 `UMalogicHealthComponent` 和 `UMalogicMagicComponent`。
- Service：按业务领域监听 Model、转换数据并维护 ViewModel。当前 UI Service 的业务逻辑使用 Lua 实现。
- ViewModel：使用 C++ 定义反射字段、FieldNotify 和 UI 数据接口。
- Widget：只绑定和显示 ViewModel，不创建长期 ViewModel，也不直接访问 Model。

## 分层 Manager

ViewModel 按数据生命周期由三个 Unreal Subsystem 承载：

| Manager | Subsystem | 生命周期 | 数据范围 |
| --- | --- | --- | --- |
| `UVMGameManager` | `UGameInstanceSubsystem` | GameInstance 创建至销毁 | 全局和跨世界数据 |
| `UVMLocalPlayerManager` | `ULocalPlayerSubsystem` | LocalPlayer 创建至销毁 | 玩家状态、背包和玩家 UI 数据 |
| `UVMWorldManager` | `UWorldSubsystem` | World 创建至销毁 | 关卡任务和世界 UI 数据 |

每个 Manager 只管理本作用域的 Service，不维护具体业务 Service 成员。例如，`UVMLocalPlayerManager` 不包含 `UPlayerVMService` 成员，也不提供 Player 专用访问接口。

## Manager 实现

### C++ 职责

三个 Manager 的 C++ 类负责：

- 实现 Subsystem 原生生命周期；
- 实现 `IUnLuaInterface`，提供对应 Lua 模块名；
- 使用 `TMap<FName, TObjectPtr<UViewModelService>>` 持有 Service，保证 UObject 生命周期；
- 提供通用的 `FindService`、`RegisterService` 和 `UnregisterService` 接口；
- 在正确的原生时机调用 Lua 回调；
- Manager 销毁时调用 Service 的清理函数并释放 Registry。

Manager 不直接创建 `UPlayerVMService` 或任何具体 ViewModel。

`USubsystem::Initialize()` 和 `Deinitialize()` 是 C++ 原生虚函数，不能由 Lua 直接覆写。C++ 使用以下反射回调把生命周期转发给 Lua：

```text
ReceiveManagerInitialized()
ReceivePlayerControllerChanged(NewPlayerController)  // 仅 LocalPlayer Manager
ReceiveManagerDeinitialized()
```

### Lua 职责

Manager Lua 模块位于：

```text
Content/Script/Malogic/UI/ViewModel/
```

| C++ Manager | Lua 模块 |
| --- | --- |
| `UVMGameManager` | `VMGameManager.lua` |
| `UVMLocalPlayerManager` | `VMLocalPlayerManager.lua` |
| `UVMWorldManager` | `VMWorldManager.lua` |

Lua Manager 负责：

- 决定创建和注册哪些 Service；
- 为 Service 分配 Registry 名称；
- 编排 Controller 或其他作用域事件；
- 不替代 C++ Registry 的 UObject 持有职责。

LocalPlayer Manager 的实际流程是：

```lua
local Service = UE.NewObject(UE.UPlayerVMService, self)
self:RegisterService("Player", Service)
```

Controller 变化时，Lua Manager 通过 `FindService("Player")` 找到 Service，再调用 `SetPlayerController()`。

## ViewModelService

`UViewModelService` 是所有业务 Service 的 C++ 基类，只提供通用生命周期接口：

```text
InitializeService()
DeinitializeService()
```

Service 必须使用所属 Manager 作为 Outer，并由 Manager Registry 持有。ViewModel 使用 Service 作为 Outer，并由 Service 的 `UPROPERTY` 成员持有，不能只依赖 Lua 表引用。

Service 的具体业务逻辑、ViewModel 创建、Model 委托绑定、数据转换和解绑逻辑放在 Lua。

## UPlayerVMService

`UPlayerVMService` 负责本地玩家状态 ViewModel 的 C++ 生命周期桥接：

- C++ 保存当前 `APlayerController`；
- C++ 保存 Lua 创建的 `UVMPlayerState`；
- C++ 在 Controller 变化时调用 Lua 的 `ReceivePlayerControllerChanged()`；
- C++ 在 Service 销毁时调用 Lua 的 `ReceiveServiceDeinitialized()`。

Lua Service 初始化时创建 ViewModel：

```lua
local ViewModel = UE.NewObject(UE.UVMPlayerState, self)
self:SetPlayerViewModel(ViewModel)
```

Service Lua 中的 `Initialize()` 是 UnLua 对象初始化回调，不是 UE Subsystem 的生命周期函数。

收到 Controller 后，Lua 执行以下操作：

1. 解除旧 Controller 和旧 Pawn 的委托绑定。
2. 保存新的 Controller。
3. 获取当前 Pawn。
4. 查找 `UMalogicHealthComponent` 和 `UMalogicMagicComponent`。
5. 组件获取成功后绑定对应委托并同步初始数据。
6. Pawn 或 Controller 变化时重复上述解绑和绑定流程。

`MagicComponent` 已由蓝图角色类挂载，Service 只判断组件是否获取成功，不负责挂载组件。

`HealthNormalized` 和 `MagicNormalized` 是 `[0, 1]` 范围的 UI 进度值，计算方式为：

```text
Normalized = Clamp(CurrentValue / MaxValue, 0, 1)
```

当最大值小于等于零时返回 `0`。

## 生命周期时序

```text
C++ Manager::Initialize
    -> Lua ReceiveManagerInitialized
        -> Lua NewObject Service
        -> Lua RegisterService
            -> C++ InitializeService
    -> C++ 获取当前 PlayerController
    -> Lua Manager ReceivePlayerControllerChanged
        -> Lua FindService("Player")
        -> C++ UPlayerVMService::SetPlayerController
            -> Lua Service ReceivePlayerControllerChanged
                -> 解绑旧委托
                -> 查找 Pawn 和组件
                -> 绑定委托并同步 ViewModel

C++ Manager::Deinitialize
    -> Lua ReceiveManagerDeinitialized
    -> C++ DeinitializeService
        -> Lua ReceiveServiceDeinitialized
            -> 解除所有委托
            -> 清理 Lua 引用
    -> C++ 清空 Registry
```

## Widget 管理框架

### MazeRunner 的框架思路

MazeRunner 采用以玩家 HUD 为入口的轻量 Widget 管理方案：

```text
MaruUISubsystem（全局入口）
    -> MaruHUD（单个玩家）
    -> PrimaryGameLayout（主布局）
    -> ActivatableWidgetStack（分层 Widget 栈）
    -> ActivatableWidget（具体界面）
```

- `MaruHUD` 在 `BeginPlay` 中根据可配置的 `PrimaryGameLayoutClass` 创建主布局，并将其加入 Viewport。
- `PrimaryGameLayout` 通过 `EWidgetLayer` 维护 `HUD`、`InteractableUI`、`TopUI` 三个 Widget 栈。
- `ActivatableWidgetStack` 基于 `UOverlay` 实现 Push/Pop。新 Widget 入栈时隐藏旧栈顶，出栈时移除当前 Widget 并恢复上一个 Widget。
- `ActivatableWidget` 保存自身的输入模式、鼠标锁定和鼠标显示配置。
- `PrimaryGameLayout` 根据当前栈顶 Widget 的配置统一切换 `GameOnly`、`GameAndUI` 或 `UIOnly` 输入模式，并设置鼠标状态。
- `MaruUISubsystem` 提供跨玩家分发入口，通过各玩家的 HUD 将 Widget 送入指定层级。

### 可迁移的设计原则

- 每个本地玩家拥有自己的 HUD、主布局和 Widget 栈，UI 状态不直接挂在全局对象上。
- 用主布局统一管理层级和输入路由，业务代码只指定目标层级并执行 Push/Pop。
- 常驻 HUD、可交互窗口和临时提示使用不同层级，避免临时提示改变主要窗口的输入状态。
- Widget 的输入需求由 Widget 自身配置，布局负责根据栈顶状态统一应用到 PlayerController。
- 主布局和栈持有 Widget 的 UObject 引用，避免 Widget 仅由临时 Lua 或业务变量引用而被提前回收。

### 迁移时需要重新评估的部分

- MazeRunner 使用自定义 `UUserWidget`、Widget 栈和 `AHUD`，没有依赖 CommonUI 的 Activatable Widget 生命周期；Malogic 是否引入 CommonUI 或继续采用轻量实现，需要在后续 Widget 阶段决定。
- MazeRunner 的 `TopUI` 栈不参与输入模式选择。若提示界面需要接收焦点或输入，应重新定义其输入优先级和关闭策略。
- `MaruUISubsystem::DeliverWidgetToAllPlayer` 分发时复用同一个 Widget 实例；迁移时应确认每个 PlayerController 是否需要独立创建 Widget，避免同一个 Widget 被重复挂载。
- Push/Pop 和输入模式切换需要补充空指针、重复入栈、非法出栈以及 PlayerController 尚未初始化等边界处理。

### 当前 Malogic 约定

Widget 只声明并绑定所需 ViewModel：

- 不在 Widget Construct 中创建长期 ViewModel；
- 不直接查找或监听 Gameplay Component；
- 不承担 Service 生命周期；
- 由外部 Lua 逻辑或后续 Widget 管理框架将 ViewModel 注入 Widget。

Widget 的具体创建入口、层级命名、激活/关闭接口、输入管理和复用策略仍待后续确定。本节记录 MazeRunner 的参考结构，不表示现有实现应无修改地迁移。

## Lua 在 UI 框架中的使用

MazeRunner 已启用 UnLua 和 MVVM 插件，但当前 UI 核心代码主要由 `MaruUISubsystem`、`MaruHUD`、`PrimaryGameLayout` 和 Widget 栈组成，未形成独立的 UI Lua Service 实现。因此，Lua Service 是 Malogic 在迁移 MVVM 方案时的架构扩展，不是 MazeRunner UI 代码的直接复制。

在 Malogic 中，C++ Manager 负责 Subsystem 作用域、Service Registry 和 UObject 生命周期；Lua Manager/Service 负责业务编排、Model 事件响应、数据转换和 ViewModel 更新。Widget 只接收并绑定 ViewModel，不直接从 Lua 查找游戏数据。

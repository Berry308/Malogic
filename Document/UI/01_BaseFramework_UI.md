# 使用Lua构建MVVM框架下ViewModel的管理与更新

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

# Widget 管理框架

## MazeRunner 参考

MazeRunner 采用以玩家 HUD 为入口的轻量 Widget 管理方案：

```text
MaruHUD（单个玩家）
    -> PrimaryGameLayout（主布局）
    -> ActivatableWidgetStack（分层 Widget 栈）
    -> ActivatableWidget（具体界面）
```

- `MaruHUD` 在 `BeginPlay` 中根据可配置的 `PrimaryGameLayoutClass` 创建主布局，并将其加入 Viewport。
- `PrimaryGameLayout` 通过 `EWidgetLayer` 维护 `HUD`、`InteractableUI`、`TopUI` 三个 Widget 栈。
- `ActivatableWidgetStack` 基于 `UOverlay` 实现 Push/Pop。新 Widget 入栈时隐藏旧栈顶，出栈时移除当前 Widget 并恢复上一个 Widget。
- `ActivatableWidget` 保存自身的输入模式、鼠标锁定和鼠标显示配置。
- `PrimaryGameLayout` 根据当前栈顶 Widget 的配置统一切换 `GameOnly`、`GameAndUI` 或 `UIOnly` 输入模式，并设置鼠标状态。

## Malogic UI Manager

`UMalogicUIManager` 继承 `ULocalPlayerSubsystem`，是单个本地玩家的客户端 UI 管理入口。

```text
本地 LocalPlayer
    -> UMalogicUIManager
    -> AMalogicHUD / UPrimaryGameLayout
    -> HUD 根 ActivatableWidget
    -> EquipmentQuickBar 等 HUD 子 Widget
```

`UMalogicUIManager` 负责：

- 等待本地 `PlayerController`、`AMalogicHUD` 和 `UPrimaryGameLayout` 就绪。
- 创建、添加和移除本地玩家的 Widget。
- 在 Widget 加入布局前注入所需 ViewModel。
- 响应本地 UI 请求与复制状态变化，协调常驻 HUD、窗口和提示 UI。

`UVMLocalPlayerManager` 只负责 ViewModel Service、数据绑定和 ViewModel 生命周期；`UMalogicUIManager` 只负责 Widget 生命周期和布局。两者通过已注册的 ViewModel 协作，不互相持有业务数据。

## Widget 容器

`UPrimaryGameLayout` 按用途划分容器：

| 容器 | 规则 | 示例 |
| --- | --- | --- |
| HUD Layer | 通常只保留一个 HUD 根 `ActivatableWidget` | 状态栏、`EquipmentQuickBar`、准星、任务追踪 |
| Interactable UI Stack | 仅显示栈顶，用于需要焦点的窗口 | 背包、设置、商店、暂停菜单 |
| Top UI Layer | 不改变主输入状态的临时覆盖 UI | Toast、提示、飘字 |

`EquipmentQuickBar` 是 HUD 根 Widget 的子 Widget，不单独进入 `HUDLayerStack`。这样常驻 HUD 内的多个模块可同时显示，而不会因 Stack 的 Push 行为相互隐藏。

## 创建与绑定时机

常驻 HUD 在以下条件都满足后，由 `UMalogicUIManager` 为每个本地玩家创建一次：

1. 本地 `PlayerController` 已创建。
2. `AMalogicHUD` 已创建 `UPrimaryGameLayout`。
3. 对应 ViewModel Service 已注册，或能够提供目标 ViewModel。

创建顺序为：

```text
获取 ViewModel
    -> CreateWidget（以本地 PlayerController 为 Owning Player）
    -> 注入 Manual ViewModel
    -> Push 到指定容器
```

Widget 不直接读取 `UMalogicQuickBarComponent` 等 Gameplay Component。以 `EquipmentQuickBar` 为例，后续应由本地玩家作用域的快捷栏 ViewModel Service 监听槽位和当前选中槽位变化，并更新快捷栏 ViewModel；Widget 仅绑定该 ViewModel。Controller 更换、重生或 Pawn 更换时，Service 重新绑定数据源，常驻 HUD 不需要重建。

## 网络边界

服务器不创建、不持有、也不向远端客户端分发 Widget。服务器通过复制权威游戏状态或发送 Client RPC 表达游戏事件；每个客户端的 `UMalogicUIManager` 监听本地可见状态后，各自创建、更新或关闭 UI。

例如，快捷栏数据通过所属玩家的复制状态更新，Boss 战通过 `GameState` 状态同步，物品获得提示通过 Owner-only 数据或 Client RPC 触发。Widget 实例只能附加到其所属本地玩家的布局，不能在多个 PlayerController 间复用。

## 设计原则

- 每个本地玩家拥有自己的 HUD、主布局和 Widget 栈，UI 状态不直接挂在全局对象上。
- 用主布局统一管理层级和输入路由，业务代码只指定目标层级并执行 Push/Pop。
- 常驻 HUD、可交互窗口和临时提示使用不同容器，避免临时提示改变主要窗口的输入状态。
- Widget 的输入需求由 Widget 自身配置，布局负责根据栈顶状态统一应用到 PlayerController。
- 主布局和栈持有 Widget 的 UObject 引用，避免 Widget 仅由临时 Lua 或业务变量引用而被提前回收。

# Lua 在 UI 框架中的使用

在 Malogic 中，C++ Manager 负责 Subsystem 作用域、Service Registry 和 UObject 生命周期；Lua Manager/Service 负责业务编排、Model 事件响应、数据转换和 ViewModel 更新。Widget 只接收并绑定 ViewModel，不直接从 Lua 查找游戏数据。

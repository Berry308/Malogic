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

## Widget 约定

Widget 只声明并绑定所需 ViewModel：

- 不在 Widget Construct 中创建长期 ViewModel；
- 不直接查找或监听 Gameplay Component；
- 不承担 Service 生命周期；
- 由后续 Widget 管理框架或外部 Lua 逻辑将 ViewModel 注入 Widget。

Widget 创建、层级、激活、输入管理和复用策略暂不属于本阶段范围。

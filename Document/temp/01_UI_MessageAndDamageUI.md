# Malogic Message System

## 文档定位

本文是后续实现 Gameplay Message 系统和伤害数字 UI 的设计基线，不是代码实现记录。

本文使用的两个参考文档，其性质不同：

- `01_BaseFramework_Message_AIQ&A.md` 是对 Lyra Gameplay Message 系统的技术分析。它提供设计思想和简化实现方向，不等同于必须原样复制的代码指令。
- `UI/01_UI_CauseDamage.md` 是伤害数字 UI 的需求和候选流程。伤害数字只应显示在造成伤害的本地玩家客户端，伤害类型、数值和元素反应应影响表现；其中三种 GCN 触发方案是待选择的实现方案。
- 本文将两份材料整理为 Malogic 的项目方案，并结合当前已有的 GAS、Gameplay Tags、UMalogicUIManager 和 MVVM 框架进行约束和修正。

本阶段只整理架构，不引入代码、不引入 Lyra 的完整 Message 插件，也不修改配置。


## 验收标准

### 伤害数字

- 只有造成伤害的本地玩家客户端显示数字，其它客户端不显示该数字。
- 显示值是实际有效伤害，不显示免疫、零伤害或被过滤的事件。
- 不同 `DamageType` 和 `ReactionTag` 可映射到不同 UI 表现。
- 世界坐标可以正确投影到屏幕，Popup 能在动画结束后清理。
- 多条同时出现的数字互不完全遮挡，不改变交互 UI 的输入模式。
- 攻击、受击、重生、切换 Pawn 和地图切换后没有重复监听、野回调或残留 Widget。

## 待确认决策

在开始代码实现前，默认采用以下决策：

- 伤害触发：优先验证 GameplayEffect GameplayCue 方案。
- 显示过滤：在 GCN 进入 Message 广播前按本地伤害来源过滤。
- UI 接入：`UMalogicUIManager` + 独立 DamageNumberLayer，不使用会折叠旧 Widget 的 TopUI Stack 逐条 Push。
- 叠加方式：第一阶段一击一条 Popup，后续再加入短时间窗口合并和对象池。
- 击杀特殊效果：暂不实现，保留扩展字段或独立消息频道。

若运行时验证证明 GameplayCue 无法稳定携带本地显示所需的来源或最终伤害数据，应暂停继续扩展 UI 表现，先改为明确的服务器定向通知方案，并重新定义网络数据流。

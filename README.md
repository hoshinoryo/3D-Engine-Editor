# 自制引擎编辑器（Custom Engine Editor）



## 项目概述 | プロジェクト概要 | Project Overview

本项目是基于DirectX 11从零构建的3D引擎编辑器原型。
核心目标在于验证“编辑器驱动型引擎结构”的可行性，而非完成具体游戏产品。

本项目强调：

- 引擎结构可解释性
- 工具链与运行逻辑分离
- 数据结构清晰边界
- Pass化的职责划分

------

本プロジェクトは DirectX 11を用いてゼロから構築した3Dエンジンエディタの試作です。
完成したゲームの制作ではなく、「エディタ主導型エンジン構造」の設計と検証を目的としています。

重視している点：

- 構造の明確性
- ツール処理とゲーム処理の分離
- データ責務の明確化
- Pass単位の処理分割

------

This is a custom 3D engine editor prototype built from scratch using DirectX 11.

The goal is not game completion, but validating an editor-driven engine architecture with clear structural separation and extensibility.

Key principles:

- Structural clarity
- Tool/runtime separation
- Explicit data boundaries
- Pass-based responsibility design



# 架构核心思想 | 設計思想 | Architectural Philosophy

## 1. Editor-First设计

引擎结构从“编辑流程”反向推导，而非从游戏逻辑出发。

编辑完整链路：

```
可视化 → 可选中 → 可编辑 → 可验证
```

内部对应关系：

```
Render → Picking → Transform Update → Debug Validation
```

------

エディタ操作の流れから逆算してエンジン構造を設計しています。

編集フロー：

```
表示 → 選択 → 編集 → 検証
```

------

The engine is designed from the editor workflow backward:

```
Visualize → Select → Modify → Validate
```



# 核心技术总结 | 技術構成 | Technical Summary

------

## Pass-Based Architecture

系统将不同职责拆分为独立Pass：

```
+------------------+
|   Render Pass    |
+------------------+
|   Picking Pass   |
+------------------+
|   Gizmo Pass     |
+------------------+
| Debug Draw Pass  |
+------------------+
```

每个Pass：

- 拥有独立更新入口
- 自行判断AppMode
- 不直接依赖其他Pass内部逻辑

原理说明：

Pass化结构的本质是**按职责分层执行流程**，而非单一大循环内混合处理。
这降低耦合度，并为后续插入新功能提供明确位置。

------

処理をPass単位で分離し、それぞれが独立した責務を持つ構造です。
責務ごとに更新・描画を分離することで、保守性と拡張性を確保しています。

------

Each responsibility is separated into independent passes.
This reduces coupling and creates explicit extension points.

------

## Transform (TRS) 数据模型

统一数据结构：

```
struct Transform
{
    float3 position;
    float3 rotation;
    float3 scale;
};
```

矩阵转换流程：

```
WorldMatrix = T * R * S
```

渲染与判定阶段统一使用矩阵结果，而编辑阶段使用原始TRS数值。

优势：

- 编辑友好
- 数据可序列化
- 与导入导出逻辑兼容

------

TRSを統一データとして保持し、描画時に行列へ変換します。
編集時は数値、描画時は行列という明確な分離を行っています。

------

TRS values are edited directly, but converted into matrices for rendering and collision.

------

## Gizmo 世界空间拖拽原理

拖拽核心逻辑：

1. 鼠标屏幕坐标 → 射线生成
2. 射线与Gizmo轴投影平面计算交点
3. 计算轴向位移量
4. 更新Transform.position

示意：

```
Mouse → Ray → Plane Intersection
                   ↓
              Axis Projection
                   ↓
            Delta Movement
```

该系统仅在Editor模式生效，运行时完全隔离。

------

スクリーン座標をレイに変換し、軸方向へ投影して移動量を算出します。

------

Screen-space ray projection is used to compute axis-aligned movement.

------

## Scene / Asset 分离设计

```
ModelAsset  (资源数据)
     ↑
     |
MeshObject (场景实例)
```

ModelAsset：

- 网格
- 材质
- 静态资源信息

MeshObject：

- Transform
- 选择状态
- 实例数据

意义：

- 避免重复加载
- 资源共享
- 序列化边界清晰

------

リソースとインスタンスを明確に分離しています。

------

Assets and instances are clearly separated for reuse and serialization safety.

------

## Editor / Game Mode 隔离

```
AppMode = Editor | Game
```

所有工具功能：

- Gizmo
- DebugDraw
- Picking

均检测当前模式。

作用：

- 防止工具逻辑污染运行时
- 为未来构建纯运行版本打基础

------

エディタ専用処理はGameモードでは動作しません。

------

Tool logic is strictly disabled in runtime mode.



# 数据流程概览 | データフロー | Data Flow

```
Input
  ↓
AppMode Check
  ↓
Pass Execution
  ↓
Scene Update
  ↓
Render
```

所有编辑操作最终修改Scene数据结构。
Render始终读取同一份数据。

------

編集処理も描画処理も同じSceneデータを参照します。

------

Both editor and render operate on the same scene data.



# 当前技术完成度 | 実装状況 | Current Status

已完成：

- SceneManager
- MeshObject管理
- TRS编辑系统
- Translate Gizmo
- Picking Pass
- Debug Draw分类系统
- AppMode管理
- ImGui编辑界面
- Pass化结构

------

Implemented core editor pipeline and structural separation.



# 后续扩展方向 | 今後の拡張 | Future Extensions

- JSON场景序列化
- Material编辑扩展
- Hierarchy结构支持
- Undo / Redo系统
- 更完整资源管理系统



# 项目定位 | 位置づけ | Positioning

本项目定位为：

> 面向工具开发理解的结构型引擎实验

重点在于：

- 架构可解释性
- 数据职责划分
- 编辑器驱动思路

------

This project focuses on structural understanding rather than feature scale.

It is an architectural experiment in editor-oriented engine design.

# 自制引擎编辑器（Custom Engine Editor）

## 项目简介 | プロジェクト概要 | Project Overview

这是一个基于 DirectX 的自制 3D 引擎编辑器原型，目标是在有限时间内，从零实现一个具备明确“编辑器感”的最小可用工具。

项目重点并非完成一款游戏，而是围绕 **引擎结构设计、编辑器交互、数据组织方式以及可扩展性** 进行实践，尝试构建一条能够被持续扩展的编辑工作流。

目前，该编辑器已经具备场景对象管理、Transform 编辑、Gizmo 操作、调试可视化、编辑器 / 运行模式区分等核心功能，并通过 Pass 化的处理流程，将「编辑器行为」与「渲染、选择、调试逻辑」明确拆分，逐步向真正可维护、可扩展的工具型引擎演进。

------

本プロジェクトは、DirectX を用いてゼロから制作している 3D 自作エンジン編集エディターです。
完成したゲームを作ることを目的とせず、「編集ツールとして成立する最小構成」を実装することを重視しています。

シーン管理、Transform 編集、Gizmo 操作、デバッグ描画、Editor / Game モードの切り替えなど、編集作業に直結する機能を中心に実装しており、処理は Pass 単位で整理されています。
これにより、編集器特有の処理と描画・入力・判定ロジックを明確に分離しています。

------

This project is a custom-built 3D engine editor prototype based on DirectX, developed entirely from scratch.

Rather than building a complete game, the focus is on creating a tool-oriented engine that supports editor-driven workflows.
Core features such as scene management, transform editing, gizmo manipulation, debug visualization, and editor/game mode separation are already implemented.

A key design concept of this project is the **pass-based structure**, which separates rendering, picking, editor interaction, and debug visualization into independent processing stages, enabling clearer responsibilities and future extensibility.

------

## 项目背景与设计目标 | 企画背景・設計方針 | Background & Design Goals

以理解引擎与工具开发流程为核心目标，从「编辑器如何驱动引擎」的角度反向设计系统结构。

所有功能均以 **可扩展性、可维护性、以及可解释性** 为前提进行实现，在时间受限的条件下，优先完成一条从“可视 → 可选 → 可编辑 → 可验证”的完整编辑链路。

------

エンジン・ツール開発の流れを、設計から実装まで一貫して理解すること
「エディタから使われるエンジン」という視点での構造設計
拡張性・保守性・説明可能性を重視
最小構成であっても、編集フローとして成立することを優先

------

Focus on learning engine and tool development through hands-on implementation
Design the engine from an editor-first perspective
Emphasize extensibility, maintainability, and clarity
Prioritize a minimal but complete editing workflow under time constraints

------

## 当前已实现功能 | 実装済み機能 | Implemented Features

### 场景与对象管理 | シーン・オブジェクト管理 | Scene & Object Management

- 统一管理场景中的 MeshObject
- 对象唯一 ID 分配与选择状态管理
- 基于 SceneManager 的集中式管理结构
- 编辑器与运行时共享同一场景数据结构

------

### Transform 系统（TRS） | Transform（TRS）編集システム | Transform (TRS) System

- 统一的 Transform 数据结构（Translation / Rotation / Scale）
- 编辑器中可直接数值编辑 TRS 参数
- 所有空间变换最终统一转换为矩阵参与渲染与判定
- 为后续导入 / 导出与序列化提供清晰的数据边界

------

### Gizmo 系统（Translate） | Gizmo 操作 | Transform Gizmo (Translate)

- 实现基于世界空间的 Translate Gizmo
- 支持 X / Y / Z 轴独立选择与拖拽
- Gizmo 操作与对象 Transform 数据直接联动
- 通过屏幕空间与世界空间映射完成精确拖拽计算
- 编辑器模式下专用，不影响运行时逻辑

------

### 编辑器 UI（ImGui） | エディタ UI（ImGui） | Editor UI (ImGui)

- 基于 ImGui 的编辑器窗口构成
- 实时反馈对象状态与参数变化
- 编辑用途 UI 与运行逻辑严格区分
- 为后续属性扩展提供统一入口

------

### Editor / Game 模式区分 | Editor / Game モード切り替え | Editor / Game Mode Separation

- 统一的 AppMode 管理
- 编辑器专用功能仅在 Editor 模式下生效
- 为 Play / Edit 切换与工具隔离提供基础结构

------

### Pass 化处理结构 | パス分割設計 | Pass-based Architecture

- 将编辑器中的处理流程拆分为多个 Pass
- 不同 Pass 负责不同职责，例如：
  - 渲染 Pass
  - Picking Pass（对象选择判定）
  - Gizmo 操作 Pass
  - Debug Draw Pass
- 每个 Pass 独立判断当前模式与启用条件
- 有效避免编辑逻辑与渲染逻辑相互耦合
- 为后续功能扩展提供清晰的插入点

------

### Debug Draw 系统 | デバッグ描画システム | Debug Draw System

- 基于 DebugDrawCategory 的分类管理
- 可开关的调试绘制设置
- 根据当前 AppMode 与 Category 判定是否绘制
- 用于 Collision、辅助信息等可视化验证

------

### ModelAsset / MeshObject 结构设计 | データ構造設計 | Data Structure Design

- **ModelAsset**
  - 代表模型资源数据
  - 负责网格、材质等共享信息
  - 可被多个场景对象引用
- **MeshObject**
  - 场景中的实例对象
  - 持有 Transform、选择状态等实例级信息
  - 通过指针或引用关联对应的 ModelAsset

该分离结构使资源与实例职责清晰，避免重复加载，并为后续序列化、导入导出与资源管理奠定基础。

------

## 当前设计重点 | 現在の設計上の重点 | Current Design Focus

- 结构清晰优先于功能数量
- 所有系统均以可保存、可复原为设计前提
- 编辑器逻辑与游戏逻辑严格区分
- 明确 Asset / Object / Scene / Editor 各层职责
- 通过 Pass 化结构提升可读性与可维护性

------

## 计划中的功能 | 今後の予定 | Planned Features

- 场景数据导入 / 导出（JSON 等）
- Transform / Material 等最小序列化单元
- 编辑器操作流程的进一步完善
- 工具型引擎定位的持续强化

------

## 备注 | 備考 | Notes

本项目用于学习与作品展示，重点在于设计思路、结构划分与实现过程，而非追求完整的商业级功能。

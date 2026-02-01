🛠️ 自制引擎编辑器（Custom Engine Editor）
📌 项目简介 | プロジェクト概要 | Project Overview
🇨🇳 中文

这是一个基于 DirectX 的自制 3D 引擎编辑器原型，目的是在有限时间内，从零实现一个具备“编辑器感”的最小可用工具（Minimum Viable Editor）。
项目重点不在于完成一个完整游戏，而是围绕引擎结构设计、编辑器交互、数据组织与可扩展性进行实践。

目前该编辑器已经可以完成场景中对象的管理、Transform 编辑、调试可视化、编辑器 / 运行模式区分等基础功能，并正在逐步向真正可保存、可复用的编辑工具方向推进。

🇯🇵 日本語

本プロジェクトは、DirectX を用いてゼロから制作している 3D 自作エンジン編集ツール（Editor）です。
完成したゲームを作ることではなく、「編集ツールとして成立する最小構成（Minimum Viable Editor）」を実装することを目的としています。

現在は、シーン管理、Transform 編集、デバッグ描画、Editor / Game モードの切り替えなど、
「ツールらしさ」を感じられる基本機能を中心に実装しており、今後はデータの保存・再利用を前提とした設計へ発展させていく予定です。

🇺🇸 English

This project is a custom-built 3D engine editor prototype based on DirectX, developed from scratch.
Rather than building a complete game, the goal is to create a Minimum Viable Editor that demonstrates core concepts of engine architecture, editor interaction, and data-driven design.

At its current stage, the editor supports scene object management, transform editing, debug visualization, and editor/game mode separation, and is being extended step by step toward a practical, reusable editing tool.

🎯 项目背景与设计目标
企画背景・設計方針
Background & Design Goals
🇨🇳 中文

以学习和理解引擎/工具开发流程为核心目标

从“程序如何被编辑器使用”的角度反向设计引擎结构

所有功能均以可扩展、可维护、可解释为前提实现

在时间受限的情况下，优先实现最小但完整的一条编辑链路

🇯🇵 日本語

エンジン・ツール開発の流れを実装を通して理解すること

「エディタから使われるエンジン」という視点で設計

拡張性・保守性・説明可能性を重視

制限された期間の中で、最小だが一貫した編集フローを優先

🇺🇸 English

Focus on learning engine & tool development through implementation

Design the engine from the perspective of editor-driven usage

Emphasis on extensibility, maintainability, and clarity

Prioritize a minimal but complete editing workflow under time constraints

🧩 当前已实现功能
実装済み機能
Implemented Features
🔹 场景与对象管理 | シーン・オブジェクト管理 | Scene & Object Management

统一管理场景中的 MeshObject

对象唯一 ID 分配与选择状态管理

基于 SceneManager 的集中式管理结构

🔹 Transform 系统（TRS）
Transform（TRS）編集システム
Transform (TRS) System

统一的 Transform 数据结构（Translation / Rotation / Scale）

编辑器中可直接修改对象的 TRS 参数

Transform → Matrix 的集中计算逻辑

为后续导入 / 导出与序列化做结构准备

🔹 编辑器 UI（ImGui）
エディタ UI（ImGui）
Editor UI (ImGui)

基于 ImGui 的属性编辑窗口（Attribute Editor）

可视化编辑对象 Transform

编辑器友好的即时反馈设计

明确区分“编辑用途 UI”与“运行时逻辑”

🔹 Editor / Game 模式区分
Editor / Game モード切り替え
Editor / Game Mode Separation

应用模式（AppMode）管理

编辑器专用功能仅在 Editor 模式下启用

为后续 Play / Edit 切换打下基础

🔹 Debug Draw 系统
デバッグ描画システム
Debug Draw System

分类管理的 DebugDrawCategory

可开关的调试绘制设置

根据当前模式与类别判断是否绘制

用于 Collision / 辅助可视化的基础设施

🔹 ModelAsset / MeshObject 分离设计
ModelAsset / MeshObject 分離設計
ModelAsset / MeshObject Separation

ModelAsset：资源层（可被多个对象引用）

MeshObject：场景实例层

资源唯一缓存（Asset Cache）

为减少重复加载与后续序列化做准备

🧠 当前设计重点
現在の設計上の重点
Current Design Focus
🇨🇳 中文

结构清晰优先于功能数量

所有系统都以“将来能被保存 / 复原”为前提

编辑器 ≠ 游戏逻辑，二者严格区分

明确每一层（Asset / Object / Scene / Editor）的职责

🇯🇵 日本語

機能数よりも構造の明確さを優先

将来的なシリアライズを前提とした設計

エディタ機能とゲームロジックの分離

各レイヤーの責務を明確に定義

🇺🇸 English

Prioritize clear structure over feature count

Design with future serialization in mind

Strict separation between editor tools and game logic

Clearly defined responsibilities for each system layer

🚧 计划中的功能（简要）
今後の予定（概要）
Planned Features (Brief)

场景数据的导入 / 导出（JSON 等）

Transform / Material 等最小序列化单元

编辑器操作流程的进一步完善

更明确的“工具型引擎”定位强化

📝 备注 | 備考 | Notes

本项目为学习与作品展示用途，重点在于设计思路与实现过程，而非追求完整商业级功能。

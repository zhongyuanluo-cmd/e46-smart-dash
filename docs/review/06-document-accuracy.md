# 审查报告：文档准确性

> 审查日期: 2026-05-31 | 审查人: Copilot | 范围: 项目全部 .md 文档 vs 代码/硬件现状

---

## 整体评价：⭐⭐⭐ (3/5)

文档体系完整、分类清晰，但存在两类典型问题：(1) Phase 2 推进后多个文档未同步更新（"骨架就绪"→"已实现"），(2) 个别硬伤（SPI0→SPI1、UART 分配表错误）。

---

## 1. docs/reference-index.md — 资料索引 ⭐⭐⭐

### 🔴 DOC-1 — 源码状态描述严重过时

| 条目 | 当前描述 | 实际情况 | 应改为 |
|------|---------|---------|--------|
| `src/common/` | "骨架就绪，待填充" | logging/config/types/ring_buffer 全部实现 | "✅ Phase 2 完成" |
| `src/can-gateway/` | "骨架就绪，Phase 2 实现" | 7 个源文件 + spec 全部实现 | "✅ Phase 2 完成" |
| 顶层 CMake | "仅 common 启用，其余注释" | can-gateway 也已启用 | "common + can-gateway 启用" |
| Phase 2 变更 | "🟡 提案完成，待实施" | 代码已实现，只剩硬件上车部分 | "🟡 实施中 (代码完成，待上车)" |

### 🔴 DOC-5 — 缺少 devlog 06 条目

`docs/devlog/06-can-bringup.md` 已存在但索引中只列到 05。需新增。

### 🟡 DOC-9 — Phase 1 归档路径错误

索引写 `openspec/changes/refine-phase1-basic-platform/`，实际路径是 `openspec/changes/archive/refine-phase1-basic-platform/`。

---

## 2. docs/pin-mapping.md — 引脚映射 ⭐⭐⭐

### 🔴 DOC-3 — SPI1 MCP2515 标注 "硬件未安装" 已过时

```
> ⚠️ 当前硬件未安装，驱动 probe 因 vdd-supply 缺失和硬件不在而失败
> 📝 我们只有 1 个 MCP2515 模块，DTBO 是双 CAN 版本，Phase 2 时需改为单 CAN
```

实际情况：
- MCP2515 已物理安装（tasks.md 9.1-9.5 全部 ✅）
- DTBO 已改为单 CAN (`mcp2515_can0.dtbo`)
- can0 已上线，ERROR-ACTIVE 状态

### 🔴 DOC-4 — UART 分配表完全错误

| 当前文档 | 正确分配 |
|---------|---------|
| UART2 → K-Bus | **UART4 (ttyS4) → K-Bus** (Pin 32/33) |
| UART3 → I-Bus | UART2 (ttyS2) → CH343 串口终端 |
| UART4 → BT HCI | — |

实际验证：K-Bus 通过 ttyS4 (Pin 32/33) 通信，台上测试已通过 (tasks.md 10.4)。

### 🟡 DOC-11 — Phase 2 待办列表已全部完成

文档末尾 "Phase 2 待办" 列了 4 项，全部已完成。应删除或改为 "✅ 已完成" 汇总。

---

## 3. 硬件 - 外部实体接线一览.md ⭐⭐⭐

### 🔴 DOC-2 — CAN 接口写的是 SPI0

```
动力 CAN | Pin 9 (CAN_H) / Pin 10 (CAN_L) | → MCP2515-E → RK3566 SPI0
```

正确是 **SPI1**。pin-mapping.md 已明确说明 SPI0 引脚不在 40-pin 排针上。

### 🟡 DOC-12 — K-Bus UART 未指定具体端口

```
K-Bus | Pin 14 | → TH3122.4 → RK3566 UART
```

应更新为 `RK3566 UART4 (ttyS4, Pin 32/33)`，与 pin-mapping.md 和实际接线一致。

---

## 4. docs/devlog/ — 开发日志 ⭐⭐⭐⭐

✅ 01-05 准确反映 Phase 1 历史。  
✅ 06-can-bringup.md 记录 Phase 2 离线开发阶段，内容准确。

### 🟡 DOC-10 — 缺少 Phase 2 硬件安装日志 (devlog 07)

Phase 2 硬件安装（MCP2515 接线、TH3122.4 台上测试、can0 上线）已完成 (tasks.md 9-10 大部分 ✅)，但没有对应的 devlog。

**建议**: 创建 `docs/devlog/07-phase2-hardware.md` 记录硬件安装过程。

---

## 5. BOM.md — 物料清单 ⭐⭐⭐⭐

### 🟡 DOC-7 — ADS1115 采购状态未更新

BOM 显示 ADS1115 为 "📦 待采购"，但 tasks.md 8.5 说 "BOM.md 已更新: ADS1115 已加入待采购"。实际应该是 "✅ 已采购" 或至少 "📦" 状态一致。

> 注：tasks.md 8.5 写的是 "ADS1115 已加入待采购" 而非 "已采购"，所以 BOM 的 📦 与 tasks 一致。但这与 worker-progress 中 "ADS1115 模块已就位" 的说法不同。需用户确认实际状态。

### 🟡 DOC-8 — BOM 序号重复

GPS 部分两个条目都是 "#3"（ATGM336H 和 NEO-M9N），应改为 #1, #2, #3。

---

## 6. 0 开发流程.md — 开发路线图 ⭐⭐⭐⭐

### 🟡 DOC-6 — Phase 1 SPI 状态描述过时

```
├── ✅ GPIO/UART/I2C 基础驱动验证        # SPI 待 Phase 2 MCP2515 硬件
```

SPI1 loopback 已于 Phase 1 验证通过（PIN19↔PIN21, 100/100）。应改为：
```
├── ✅ GPIO/SPI/I2C/UART 全部验证通过     # SPI1 loopback 100/100
```

---

## 7. 软件与SDK - 中间件设计.md / 总体框架.md

✅ 这些设计文档描述的是架构规划而非当前状态，不需要逐 Phase 更新。内容与 Phase 2 实现方向一致。

---

## 8. README.md ⭐⭐⭐⭐

### 🟡 DOC-13 — 屏幕描述可能误导

```
- **屏幕**: 6.9" IPS 280×1424 MIPI DSI (ER-TFT069-1)
```

这是 Phase 6 目标屏。当前实际使用的是 7" DSI LCD (800×480)。建议加注：
```
- **屏幕 (Phase 6)**: 6.9" IPS 280×1424 MIPI DSI (ER-TFT069-1)
- **屏幕 (当前)**: 7" DSI LCD 800×480 (Waveshare, Phase 1-5 开发用)
```

---

## 📊 汇总

| ID | 级别 | 文件 | 问题 |
|----|------|------|------|
| DOC-1 | 🔴 | reference-index.md | 源码状态 4 处过时（common/can-gw/CMake/Phase2） |
| DOC-2 | 🔴 | 外部实体接线一览.md | CAN 写 SPI0，实际 SPI1 |
| DOC-3 | 🔴 | pin-mapping.md | "硬件未安装/双CAN" 已过时 |
| DOC-4 | 🔴 | pin-mapping.md | UART 分配表错误 |
| DOC-5 | 🔴 | reference-index.md | 缺少 devlog 06 |
| DOC-6 | 🟡 | 0 开发流程.md | SPI 状态描述过时 |
| DOC-7 | 🟡 | BOM.md | ADS1115 采购状态待确认 |
| DOC-8 | 🟡 | BOM.md | GPS 序号重复 #3 |
| DOC-9 | 🟡 | reference-index.md | Phase1 归档路径错误 |
| DOC-10 | 🟡 | devlog/ | 缺少 Phase2 硬件安装日志 |
| DOC-11 | 🟡 | pin-mapping.md | Phase2 待办已全部完成 |
| DOC-12 | 🟡 | 外部实体接线一览.md | K-Bus UART 未指定具体端口 |
| DOC-13 | 🟡 | README.md | 屏幕描述是 Phase6 目标而非当前 |

### 修复优先级

1. **立即修复 (🔴)**: DOC-1~5 — 5 个明确错误，直接更正
2. **本 Phase (🟡)**: DOC-6, DOC-11, DOC-12 — 简单更新
3. **待确认 (🟡)**: DOC-7 — 需用户确认 ADS1115 实际状态
4. **可选 (🟡)**: DOC-8/9/10/13 — 不影响开发

---

## ✅ 做得好的地方

1. `reference-index.md` 分类清晰，快速查阅指南实用
2. `pin-mapping.md` 的 40-pin 完整表格质量极高，每 pin 都有 ALT0-3
3. 硬件接线文档包含电源策略、终端电阻等实际细节
4. devlog 序列完整记录开发历史

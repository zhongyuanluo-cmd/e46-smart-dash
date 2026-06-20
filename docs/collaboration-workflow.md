# 混合协作模式 — 工作流指南

## 架构总览

```
            你（人 = 总调度 / Manager）
            ├── 裁决 Worker ↔ Monitor 分歧
            ├── 确认硬件操作
            └── 推进 Phase 节点
                   │
    ┌──────────────┼──────────────┐
    │              │              │
  Chat 1        共享文件        Chat 2
  Worker        协调层          Monitor
  (执行者)                     (监控者)
```

## 角色分工

| | Worker（执行者） | Monitor（监控者） |
|---|---|---|
| **职责** | 写代码、跑测试、实现任务 | 审查代码、检查进度、发现问题 |
| **读取** | tasks.md, specs, monitor-corrections.md | worker-progress.md, 源码文件, specs |
| **写入** | 源码, tasks.md checkbox, worker-progress.md | monitor-corrections.md |
| **Prompt** | `.github/prompts/worker.prompt.md` | `.github/prompts/monitor.prompt.md` |
| **Chat 启动** | 新 Chat → `@worker` 或粘贴 Worker prompt | 新 Chat → `@monitor` 或粘贴 Monitor prompt |

## 共享文件层

| 文件 | 写入者 | 读取者 | 用途 |
|---|---|---|---|
| `/memories/session/worker-progress.md` | Worker | Monitor + 人 | Worker 的进度流水账 |
| `/memories/session/monitor-corrections.md` | Monitor | Worker + 人 | Monitor 的审查反馈 |
| `tasks.md` (openspec 内) | Worker | 双方 | 任务 checkbox 状态 |
| openspec specs | 设计时写入 | 双方 | 设计规格，双方对齐的依据 |
| `/memories/repo/current-status.md` | Worker | 双方 | 项目全局状态 |

## 操作流程

### 1. 启动工作会话

```
1. 开两个 VS Code Copilot Chat 窗口
2. Chat 1: 粘贴 Worker prompt 或输入 @worker
3. Chat 2: 粘贴 Monitor prompt 或输入 @monitor
4. 人: 说明本次会话目标（如"继续 Phase 2 Task 2.1"）
```

### 2. Worker 工作循环

```
while 有未完成任务:
    1. 读取 tasks.md → 认领下一个 [ ] 任务
    2. 读取对应 openspec spec → 理解设计意图
    3. 实现 → 编译 → 测试
    4. 更新 tasks.md checkbox
    5. 写入 worker-progress.md
    6. 如果需要审查 → 标注"需要 Monitor 检查"
```

### 3. Monitor 审查循环

```
while worker-progress.md 有新条目:
    1. 读取 worker-progress.md 最新条目
    2. 读取被修改的源码文件
    3. 对照 openspec spec 审查
    4. 运行审查清单（见 Monitor prompt）
    5. 写入 monitor-corrections.md
```

### 4. 纠正处理循环

```
Worker 读取 monitor-corrections.md:
    - 🔴 严重问题 → 立即修复，更新进度
    - 🟡 建议改进 → 评估后决定是否采纳
    - 🟢 通过 → 继续下一个任务
    
    如果不同意纠正:
    → 在 worker-progress.md 写明理由
    → 由人（总调度）裁决
```

### 5. 结束会话

```
1. Worker: 确保 worker-progress.md 已更新
2. 人: 检查 tasks.md 整体进度
3. 人: 更新 /memories/repo/current-status.md（如有重大变化）
4. 会话文件自动保留在 /memories/session/，下次可续接
```

## 与 OpenSpec 的衔接

```
OpenSpec propose → design → specs → tasks → [混合协作执行] → archive
                                       ↑                    ↓
                                  Worker 读取 tasks      Worker 更新 tasks.md
                                  双方对齐 specs         Monitor 验证 specs 落地
```

- **OpenSpec 产出 specs 和 tasks** → Worker 消费它们
- **Worker 实现 + Monitor 审查** → 确保代码符合 specs
- **全部 tasks 完成** → 人执行 `/opsx:archive` 归档

## 人（总调度）的决策清单

| 情况 | 决策 |
|---|---|
| Worker ↔ Monitor 意见分歧 | 听取双方理由，做最终裁决 |
| 硬件操作（接线/上电） | 确认硬件就绪后批准 |
| Phase 节点推进 | 验收当前 Phase，批准进入下一 Phase |
| 优先级调整 | 指定 Worker 下一个该做的任务 |
| 资源约束（编译慢/板子卡） | 决定是否跳过非关键测试 |

## 实际操作提示

### VS Code Chat 窗口管理
- **两个 Chat 可以在同一窗口不同标签**，不必开两个 VS Code
- 用 `Ctrl+Shift+P → New Chat` 开新 Chat
- 每个 Chat 粘贴对应 prompt 后，后续对话无需重复
- Chat 上下文有限，长会话可能需要重新粘贴 prompt

### .prompt.md 的使用
- VS Code Copilot Chat 输入框输入 `@` 可看到可用 prompt
- 选择 `worker` 或 `monitor` 即自动注入对应 prompt
- 也可直接复制 `.github/prompts/worker.prompt.md` 内容粘贴到 Chat

### 频率建议
- **Worker 连续工作**：一次完成 1-3 个小任务再报告
- **Monitor 定期审查**：Worker 报告后触发审查，或每 5 个任务审查一次
- **人定期检查**：每 30 分钟或每个 Phase 里程碑

## 局限与注意

1. **两个 Chat 不共享上下文** — 只通过文件协调
2. **Monitor 不能改代码** — 只写审查意见，由 Worker 或人修改
3. **会话文件不持久** — `/memories/session/` 在会话结束后清除，重要结论应写入 `/memories/repo/`
4. **不是全自动** — 人的参与是核心，AI 是辅助

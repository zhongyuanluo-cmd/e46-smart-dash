---
name: dual-agent-gate
description: 双 Agent 门控协作框架 — 一个 Agent 构建(Worker)，一个 Agent 审查(Monitor)，通过 Git 分支隔离 + 门控合并实现安全迭代
argument-hint: "init | review | close | status"
allowed-tools: Read, Write, Bash, Glob, Grep, AskUserQuestion
---

<objective>
建立双 Agent（Worker + Monitor）协作框架，实现"构建-审查-合并"门控流程。

核心理念：
- Worker 在隔离分支上自由构建，永远无法触碰主分支
- Monitor 审查分支产出，决定是否合并到主分支
- 审查留痕、问题分级、闭环确认——全过程可追溯

适用场景：
- 两个不同 LLM Agent 协作完成项目（如 DeepSeek 构建 + GLM 审查）
- 同一项目中需要"写"和"审"两种角色严格分离
- 对代码/文档质量有较高要求，不能容忍错误直接合入主干

命令：
- `init` — 初始化协作框架（创建 reviews/ 目录 + 文件模板 + Git branch protection）
- `review` — Monitor 执行一轮审查，产出 monitor-review.md 新章节
- `close` — Monitor 闭环确认 Worker 修复，更新 INDEX.md
- `status` — 查看当前审查状态和进度
</objective>

---

# 双 Agent 门控协作框架（Dual-Agent Gate）

## 一、架构总览

```
┌─────────────────────────────────────────────────────┐
│                    Git 仓库                          │
│                                                      │
│  master ◄──────────────────── Monitor 签名合并       │
│    │                                 ▲               │
│    │  git merge --squash             │               │
│    │  (只有 Monitor 能 push)         │               │
│    │                                 │               │
│    └── fix/R-00X ───────── Worker 作业分支           │
│           ▲                           │               │
│           │  git push                 │               │
│           │                           ▼               │
│     Worker 改代码              Worker 自由构建        │
│                                                      │
└─────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────┐
│              reviews/ 审查工作区                      │
│                                                      │
│  INDEX.md              ← 审查状态索引               │
│  monitor-review.md     ← Monitor 审查记录（追加式） │
│  worker-progress.md    ← Worker 修复记录（追加式）  │
│  MERGE-NOTES.md        ← Monitor 合并调整记录（追加式）│
│  COLLABORATION.md      ← 角色定义与协作规范          │
│  REVIEW-PROTOCOL.md    ← 审查流程与评分标准          │
│  REVIEW-PLAN.md        ← 审查计划（首次审查前制定）  │
│  onboarding-for-worker.md ← Worker 开场白模板       │
│                                                      │
└─────────────────────────────────────────────────────┘
```

## 二、角色定义

### Worker（构建者）

| 属性 | 说明 |
|------|------|
| **职责** | 撰写、修改、生成所有项目内容（文档 + 代码） |
| **权限** | 只能推送到 `fix/R-*` 或 `feat/*` 分支，**禁止直接推送 master/main** |
| **产出** | 代码、文档、规格说明 + `worker-progress.md` 修复记录 |
| **标记** | 文件头部标注 `Author: Worker (<Agent名称>)` |
| **约束** | 必须处理 P0/P1 问题，P2/P3 可选但需在 progress 中说明决策 |

### Monitor（审查者）

| 属性 | 说明 |
|------|------|
| **职责** | 审查、验证、质询 Worker 的产出 |
| **权限** | 唯一能合并到 master/main 的角色 |
| **产出** | `monitor-review.md` 审查报告 + `INDEX.md` 状态更新 |
| **标记** | 文件头部标注 `Reviewer: Monitor (<Agent名称>)` |
| **约束** | 不直接修改 Worker 的代码逻辑，但合并时可做微调（必须记录在 MERGE-NOTES.md） |

## 三、核心原则

1. **权限隔离**：Worker 永远无法 push 到 master，只有 Monitor 能合并
2. **审查留痕**：所有审查意见和修改决策必须记录在 `reviews/` 目录，不可口头协商不留记录
3. **问题分级**：P0（严重）→ P1（重要）→ P2（改进）→ P3（微小），Worker 必须处理 P0/P1
4. **闭环确认**：Worker 修复后，Monitor 必须二次确认，形成闭环
5. **分支生命周期短**：每个 `fix/R-*` 分支应在 1-2 天内合并或关闭，避免长期偏离
6. **信息回流**：Monitor 合并时如修改了 Worker 的代码，必须在 MERGE-NOTES.md 记录，Worker 开工前必须阅读

## 四、迭代流程

### 一轮完整审查（R-NNN）的 7 个步骤

```
Step 1: Worker 完成产出，通知 Monitor
   │
   ▼
Step 2: Monitor 审查 → 写 monitor-review.md#R-NNN
   │    （问题清单 + 一致性检查 + 六维度评分 + 改进建议）
   │
   ▼
Step 3: Worker 读 review → 在 fix/R-NNN 分支上修复
   │    git checkout -b fix/R-NNN
   │    # ... 修改代码 ...
   │    git commit -m "fix(R-NNN): 描述"
   │    git push -u origin fix/R-NNN
   │
   ▼
Step 4: Worker 写 worker-progress.md#R-NNN 修复记录
   │
   ▼
Step 5: Monitor 闭环确认
   │    git diff master..origin/fix/R-NNN  # 审查差异
   │    # 逐项验证修复 → 写闭环确认到 monitor-review.md
   │
   ▼
Step 6: Monitor 合并（★ 可调整代码）
   │    git checkout master
   │    git merge --squash origin/fix/R-NNN
   │    # ★ 如需微调（参数、结构、补漏），此时编辑暂存区
   │    git commit -m "fix(R-NNN): approved by Monitor [+ 调整说明]"
   │    git push origin master
   │
   ▼
Step 7: 信息回流（★ 如有 Step 6 的调整）
        → 写 MERGE-NOTES.md#R-NNN（修改了什么/为什么/下游影响）
        → 更新 INDEX.md 状态为 ✅ 已闭环
        → 通知 Worker
```

### 特殊流程：文档审查 vs 代码审查

| 类型 | 审查对象 | Worker 产出位置 | Monitor 审查方式 |
|------|------|------|------|
| **文档审查** | 设计文档、规格说明 | 直接在项目目录修改 | `read_file` 逐文件验证 |
| **代码审查** | 源代码实现 | Git 分支 `fix/R-*` | `git diff` 审查差异 + 本地构建验证 |

> 文档审查阶段（如 R-001~R-003）可以在 master 上直接操作，因为文档修改零风险。
> 代码审查阶段（R-004+）**必须**使用分支隔离，代码错误可能破坏构建。

## 五、文件体系

### 目录结构

```
项目根目录/
├── reviews/                     ← 审查工作区
│   ├── INDEX.md                 ← 审查状态索引（Monitor 维护）
│   ├── monitor-review.md        ← Monitor 审查记录（追加式，Monitor 维护）
│   ├── worker-progress.md       ← Worker 修复记录（追加式，Worker 维护）
│   ├── MERGE-NOTES.md           ← Monitor 合并调整记录（追加式，Monitor 维护）
│   ├── COLLABORATION.md         ← 角色定义与协作规范
│   ├── REVIEW-PROTOCOL.md       ← 审查流程与评分标准
│   ├── REVIEW-PLAN.md           ← 首次审查计划
│   └── onboarding-for-worker.md ← Worker 开场白模板
├── src/                         ← 项目源码（Monitor 合并入口）
├── docs/                        ← 项目文档
└── .github/                     ← CI/CD + branch protection
```

### INDEX.md 格式

```markdown
# 审查索引

| 编号 | 审查对象 | 日期 | 状态 | Monitor 审查 | Worker 进度 |
|:---:|------|------|:---:|------|------|
| R-001 | [对象简述] | YYYY-MM-DD | ✅ 已闭环 / 🟡 待修复 | monitor-review.md#R-001 | worker-progress.md#R-001 |
```

### monitor-review.md 格式

每轮审查追加一个 `## R-NNN` 章节，包含：

```
## R-NNN: [审查对象简述]

- **审查日期**: YYYY-MM-DD
- **审查人**: Monitor (<Agent名称>)
- **审查对象**: [文件列表]
- **审查范围**: [审查范围描述]
- **审查结论**: ✅ / ⚠️ / ❌

### 总体评价          — 优势 + 主要问题 + 风险
### 问题清单          — P0/P1/P2/P3 分级表格
### 一致性检查        — 跨文档/跨模块一致性验证
### 六维度评分        — 一致性/完整性/可行性/准确性/安全性/清晰性 (1-5)
### 改进建议汇总      — 按优先级排列
### 闭环确认          — Monitor 二次验证结果（修复后追加）
```

### worker-progress.md 格式

```markdown
## R-NNN: [审查对象简述] - Fix Records

- **Review Source**: monitor-review.md#R-NNN
- **Fix Date**: YYYY-MM-DD

### P0 Fixes
| # | Status | Fix | Files |
|:---:|:---:|------|------|
| P0-1 | ✅ | 修复描述 | file1, file2 |

### P1 Fixes
[同上格式]

### P2/P3 Decisions
| # | Decision | Reason |
|:---:|------|------|
| P2-1 | ✅ Accepted | 原因 |

### Fix Summary
- **Files modified**: N files
- **Compilation verified**: [是/否]
```

### MERGE-NOTES.md 格式

Monitor 合并时如修改了 Worker 的代码，必须追加一个章节：

```markdown
## R-NNN: [合并调整标题]

- **合并日期**: YYYY-MM-DD
- **原分支**: fix/R-NNN
- **Monitor**: <Agent名称>

### 修改内容

| 文件 | 修改位置 | 原值 | 新值 | 原因 |
|------|------|------|------|------|
| types.h | TrackSegment.difficulty | `int` | `int (1-5)` | 添加范围约束注释 |

### 下游影响

- [ ] Worker 需在后续 Phase 中使用新参数范围
- [ ] Python schemas.py 需同步更新

### Worker 注意事项

- 具体注意事项
```

> **为什么需要 MERGE-NOTES.md？** LLM Agent 的上下文是每次对话从零开始的，不会自动感知 master 上的变化。MERGE-NOTES.md 是 Monitor → Worker 的单向信息回流通道，确保 Worker 在开工前知道 Monitor 的修改。

## 六、问题分级标准

| 级别 | 含义 | Worker 必须处理 | 示例 |
|:---:|------|:---:|------|
| **P0** | 严重 — 阻塞后续工作，架构/安全/一致性根本问题 | ✅ 必须 | 数据模型缺失关键字段、技术选型矛盾、API 内存泄漏 |
| **P1** | 重要 — 影响功能正确性或可维护性 | ✅ 必须 | 接口签名偏差、缺少必要模块、测试不可运行 |
| **P2** | 改进 — 不影响正确性但影响质量 | ⚠️ 可选 | 依赖管理冗余、CI 配置不完整、文档不完整 |
| **P3** | 微小 — 风格/命名/注释级别 | ⬜ 可选 | 注释不清、命名不一致、微小格式问题 |

## 七、六维度评分标准

| 维度 | 5分 | 3分 | 1分 |
|------|------|------|------|
| **一致性** | 术语/接口/数据流跨模块完全统一 | 有少数不一致但不影响理解 | 严重矛盾，无法判断以哪个为准 |
| **完整性** | 所有功能点有数据模型和实现路径 | 核心功能覆盖，边缘功能缺失 | 关键功能无落地路径 |
| **可行性** | 技术方案经过验证，无已知阻碍 | 方案合理但部分假设待验证 | 技术矛盾或方案不可行 |
| **准确性** | 字段/算法/数值完全正确 | 有小偏差但不影响方向 | 计算错误或事实错误 |
| **安全性** | 无安全风险，有防御性设计 | 低风险，有缓解方案 | 内存泄漏/注入/未授权访问 |
| **清晰性** | 文档自洽可读，新成员可直接上手 | 大体清晰，部分需口头解释 | 术语混乱，需反复确认含义 |

## 八、Git 分支策略

### 分支命名

```
fix/R-NNN           ← Worker 修复分支（审查驱动）
feat/<feature-name> ← 新功能分支（路线图驱动）
```

### 合并方式

```bash
# Monitor 合并（推荐 squash merge 保持历史干净）
git checkout master
git merge --squash origin/fix/R-004
git commit -m "fix(R-004): <简述> — approved by Monitor"
git push origin master
```

### Branch Protection Rule（GitHub 配置）

```
Settings → Branches → Add rule → master
  ☑ Require a pull request before merging
  ☑ Require approvals (1)
  ☑ Restrict who can push to matching branches
      → 只加 Monitor 的 GitHub 账号
```

### Worker 常用命令速查

```bash
# 创建修复分支
git checkout -b fix/R-004

# 在分支上工作
# ... 修改文件 ...
git add -A
git commit -m "fix(R-004): P0-1 TrackSegment NaN semantics"

# 推送分支
git push -u origin fix/R-004

# 同步 master 最新变化到分支
git fetch origin
git merge origin/master
```

### Monitor 常用命令速查

```bash
# 审查分支差异
git fetch origin
git diff master..origin/fix/R-004

# 审查通过，合并
git checkout master
git merge --squash origin/fix/R-004
# ★ 如需微调代码，此时编辑暂存区文件
git commit -m "fix(R-004): approved by Monitor [+ 调整说明]"
git push origin master

# ★ 如有合并调整，写 MERGE-NOTES.md#R-004

# 清理分支
git branch -d fix/R-004
git push origin --delete fix/R-004
```

## 九、初始化检查清单（`init` 命令）

执行 `dual-agent-gate init` 时，按以下清单逐项检查：

- [ ] 1. 创建 `reviews/` 目录
- [ ] 2. 生成 `reviews/COLLABORATION.md`（角色定义）
- [ ] 3. 生成 `reviews/REVIEW-PROTOCOL.md`（审查流程）
- [ ] 4. 生成 `reviews/REVIEW-PLAN.md`（审查计划，基于项目现状）
- [ ] 5. 生成 `reviews/monitor-review.md`（空壳，待追加）
- [ ] 6. 生成 `reviews/worker-progress.md`（空壳，待追加）
- [ ] 7. 生成 `reviews/INDEX.md`（空索引）
- [ ] 7.5. 生成 `reviews/MERGE-NOTES.md`（空壳，待追加）
- [ ] 8. 生成 `reviews/onboarding-for-worker.md`（Worker 开场白模板）
- [ ] 9. 检查 Git 仓库是否存在，提示创建（如不存在）
- [ ] 10. 检查 GitHub remote 是否配置，提示设置 branch protection
- [ ] 11. 询问用户两个 Agent 的名称（如 "DeepSeek" 和 "GLM"）
- [ ] 12. 询问当前项目处于文档审查阶段还是代码审查阶段

## 十、审查执行指南（`review` 命令）

执行 `dual-agent-gate review` 时：

1. **确定审查范围**：读取 REVIEW-PLAN.md 或询问用户本次审查的对象
2. **读取被审查文件**：逐文件 `read_file`，理解完整内容
3. **交叉比对**：检查文件间一致性（术语、接口、数据模型、流程）
4. **分级归类**：按 P0/P1/P2/P3 标准归类问题
5. **六维度评分**：对每个维度给出 1-5 分 + 理由
6. **写入 monitor-review.md**：追加新章节 `## R-NNN`
7. **更新 INDEX.md**：新增行，状态 `🟡 待修复`

## 十一、闭环确认指南（`close` 命令）

执行 `dual-agent-gate close` 时：

1. **读取 worker-progress.md**：获取 Worker 声称的修复内容
2. **逐项验证**：读取实际文件，确认修复到位
3. **分类闭环**：
   - ✅ 完全闭环：修复到位
   - ⚠️ 部分闭环：修复了但留有遗留
   - ❌ 未闭环：修复缺失或方向错误
4. **写入闭环确认**：追加到 monitor-review.md 对应章节
5. **更新 INDEX.md**：状态改为 `✅ 已闭环` 或 `⚠️ 部分闭环`

## 十二、经验教训

> 以下来自 CoDriver 项目 R-001~R-004 四轮审查的实战经验

1. **文档审查和代码审查适用不同流程**：文档修改零风险，可在 master 上直接操作；代码修改有破坏性，必须用分支隔离
2. **Worker 标签容易标错**：Worker 会把 R-002 的修复写入 R-003 段落，Monitor 闭环时需核对实际内容而非仅看标签
3. **P1-6 类问题跨轮次传递**：R-001 提出的 TrackSegment reference_* 可空问题，到 R-004 代码审查时才真正暴露（C++ 实现未用 NaN）。问题可能在文档阶段看似已修复，但代码实现时才会露出真实偏差
4. **六维度评分的"一致性"最难拿高分**：项目越大、文档越多，跨文档一致性越难维护。建议首次审查就建立术语表
5. **审查效率：先粗后细**：先做一致性检查（术语、接口对齐），再做逐文件详细审查。不一致的文件优先详审
6. **闭环确认必须读源文件**：不能只看 worker-progress.md 的描述，必须 `read_file` 验证实际修改
7. **C API FFI 设计是高风险区**：C++ ↔ C ↔ Dart/Python 跨语言接口容易出现内存安全、所有权、类型不匹配问题，需重点审查
8. **空目录无 .gitkeep 会导致 git 丢失目录结构**：Worker 建了空目录但没提交，Monitor 审查时看到的目录可能和仓库实际不一致
9. **Monitor 合并时的修改必须回流给 Worker**：LLM Agent 不会自动 `git pull` 感知变化，MERGE-NOTES.md 是唯一的信息回流通道。Monitor 每次合并调整都必须写 MERGE-NOTES，Worker 每次开工前必须读 MERGE-NOTES

## 十三、与其他 Skill 的关系

本 Skill 是独立框架，可与以下 Skill 配合使用：

| 配合 Skill | 配合方式 |
|------|------|
| GSD (get-shit-done) | GSD 管理 Phase 规划和执行，dual-agent-gate 管理质量门控 |
| gsd-code-review | gsd-code-review 做 Phase 内部代码审查，dual-agent-gate 做跨 Agent 宏观审查 |
| gsd-pr-branch | 共享分支管理理念，dual-agent-gate 增加了 Monitor-Worker 权限分离 |

## 十四、适用与不适用

### 适用场景 ✅

- 两个不同 LLM Agent 协作（如 DeepSeek 构建 + GLM 审查）
- 对产出质量有高要求（生产级代码、正式设计文档）
- 需要审查留痕和可追溯性（团队协作、合规要求）
- 长期迭代项目（多轮审查 + 累积改进）

### 不适用场景 ❌

- 单 Agent 独立工作（无协作需求）
- 一次性/探索性任务（审查成本 > 收益）
- 两个 Agent 能力差距过大（审查无意义或修复无意义）
- 项目极小（< 5 个文件，直接审查即可）

## 十五、信息回流机制

### 问题：Monitor 修改了 Worker 的代码，Worker 怎么知道？

LLM Agent 和人类开发者不同——人类会习惯性 `git pull` + 看差异，但 LLM Agent 的上下文是每次对话从零开始的，不会自动感知 master 上的变化。

### 解决方案：MERGE-NOTES.md + Worker 开工检查清单

**MERGE-NOTES.md** 是 Monitor → Worker 的单向信息回流通道。Monitor 每次合并时如果做了调整，必须在此记录。

**Worker 开工检查清单**（每次开始新一轮工作前执行）：

```
1. git pull origin master             ← 同步最新代码
2. 阅读 MERGE-NOTES.md 最新章节       ← 了解 Monitor 的修改
3. 阅读 INDEX.md                      ← 了解当前审查状态
4. 阅读最新 monitor-review.md 章节    ← 了解待处理问题
5. 阅读 worker-progress.md 上次修复   ← 了解自己的修复记录（避免重复）
6. 开始工作
```

### 信息流总览

```
Worker 产出 ──→ Monitor 审查 ──→ Worker 修复 ──→ Monitor 闭环
     │                                              │
     │              ┌────────────────┐              │
     │              │  MERGE-NOTES   │◄─────────────┘
     │              │  (Monitor 写)   │   合并时如有调整
     │              └───────┬────────┘
     │                      │
     │  ◄───────────────────┘
     │  Worker 开工时阅读
     │
     ▼
  下一轮工作
```

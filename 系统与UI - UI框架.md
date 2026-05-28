
| | LVGL v9 | Qt 6 + QML | Flutter Embedded |
|------|:---:|:---:|:---:|
| 语言 | C | C++ / QML | Dart |
| GPU 加速 | ✅ Mali-G52 | ✅ Mali-G52 | ✅ Mali-G52 |
| RAM 占用 | ~30MB | ~80-150MB | ~80-120MB |
| 启动速度 | 极快 | 快 | 中等 |
| 车载 UI 组件 | 需自建 | 需自建 | 需自建 |
| 动画/特效 | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| 生态/社区 | 嵌入式中等 | 非常成熟 | 快速增长 |
| 开发效率 | 低（C手写UI） | 高（QML声明式） | 高（Widget） |

### 推荐：Qt 6 + QML

```
选择理由（针对你的项目）：

1. 仪表盘 UI 是核心卖点
   QML 声明式做圆盘仪表、数字滚动、指针动画极为方便
   LVGL 做同样效果，代码量 3-5 倍

2. 280×1424 虽然像素少，但 UI 复杂度不低
   赛道仪表需要实时数据刷新 + 图表（圈速、G 值）
   QML Canvas / ChartView 天然支持

3. Mali-G52 GPU 加速
   Qt eglfs 后端直接走 DRM/KMS，无 X11 开销
   60fps 渲染 CPU 占用 <15%

4. 学习曲线
   QML 声明式语法上手快
   C++ 后处理传感器数据和业务逻辑
```

```
Qt 渲染管线：

  QML Scene Graph
       │
       ▼
  Qt Quick Renderer
       │
       ▼
  OpenGL ES 3.2 (Mali-G52 via Panfrost)
       │
       ▼
  DRM/KMS (直接帧缓冲，无 X11/Wayland)
       │
       ▼
  MIPI DSI → 6.9" 280×1424
```

> **如果团队不熟悉 C++/QML，LVGL 也可以**。但对于车载仪表这种重 UI 场景，Qt 的开发效率优势太明显了。
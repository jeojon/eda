# EDA 实验一课程骨架

这是一个按实验要求拆分的 C++17 课程骨架，使用 wxWidgets、CMake 和 MSVC。

当前目标不是直接交一个完成品，而是把：

- GUI
- 数据模型
- 接口
- 待实现逻辑
- 元件库目录

分开，方便课程组自己补全逻辑。

## 目录

```text
include/eda/        数据结构和接口
src/gui/            已经拆出的 wxWidgets 界面
src/todo/           学生需要实现的逻辑
docs/               架构、接口和实现说明
footprints/         元件库目录
tests/              测试目录
```

## 已完成的 GUI

- wxWidgets 主窗口。
- 菜单和状态栏。
- 左侧绘制工具。
- 左侧封装库列表。
- 中央画布。
- 右侧封装属性和焊盘属性面板。
- 缩放、平移、网格和基础绘制。

## 需要学生实现

1. `src/todo/FootprintEditorService.cpp`
2. `src/todo/GeometryService.cpp`
3. `src/todo/FootprintCodec.cpp`
4. `src/todo/FilesystemFootprintLibrary.cpp`

这些文件目前只有函数骨架和 `TODO(student)` 注释。

## 构建

默认使用你电脑中的：

```text
D:\wxWeigets\wxWidgets-3.3.3
```

执行：

```powershell
.\build_msvc.ps1
```

输出：

```text
build-msvc\EdaLab1Skeleton.exe
```

## 先读

1. `docs/01_架构说明.md`
2. `docs/02_必须实现的逻辑.md`
3. `docs/03_接口说明.md`
4. `docs/04_实验要求对照.md`
5. `docs/05_从TODO到可运行_实现指南.md`
6. `docs/06_代码量统计.md`
7. `docs/07_每个文件要做什么.md`

其中 `05` 按四个 TODO 文件说明实现步骤和验收条件，`06` 统计完整实现与当前课程骨架的代码量，`07` 逐个说明每个文件的职责和修改边界。


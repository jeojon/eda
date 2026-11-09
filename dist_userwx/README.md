# EDA 实验一：PCB 元件封装编辑器（wxWidgets / MSVC）

这是一个可运行的 wxWidgets/C++17 课程实验原型，目标是完成“电路板元件封装的设计与管理”。

## 已实现功能

- 新建、打开、保存和另存为封装文件
- 文件系统元件库，启动时自动创建示例封装
- SMD 焊盘：位置、宽高、旋转、阻焊扩展
- THT 引脚：焊盘尺寸、钻孔、编号
- 丝印线段、矩形、圆弧、多边形和文本
- 栅格吸附、鼠标滚轮缩放、中键平移
- 选择、拖动、删除图元
- 焊盘编号、名称、类型和几何属性编辑
- 示例封装：`R_0805`、`HEADER2_1`、`DIP14`、`SO14`

## 启动

编译后的程序第一次运行会自己在可执行文件旁边创建：

```text
footprints/
```

示例封装会自动写入这个目录。程序保存的是自定义 ASCII 封装格式，单位统一为毫米。

## 编译（MSVC x64）

本项目按 CMake + MSVC x64 + Ninja 构建。

使用：

```powershell
.\build_msvc.ps1
```

默认直接使用你电脑已有的 `D:\wxWeigets\wxWidgets-3.3.3` 和其中的 `vc_x64_lib` 静态库。

Windows 下可执行文件位于：

```text
build-msvc-userwx/EdaLab1.exe
```

## 封装文件格式

每行是一条记录，文件头为：

```text
EDALAB1_FOOTPRINT 1
NAME R_0805
CATEGORY Passive
DESC 0805 SMD resistor/capacitor land pattern, mm
ORIGIN 0 0
PAD SMD ROUNDED 1 1 -0.9 0 1.3 1.5 0 0 0.1 0.2
PAD SMD ROUNDED 2 2 0.9 0 1.3 1.5 0 0 0.1 0.2
LINE -0.5 -0.8 0.5 -0.8 0.12
TEXT 0 1.1 0.8 R?
END
```

`PAD` 字段顺序：

```text
PAD 类型 形状 编号 名称 X Y 宽 高 旋转 钻孔 阻焊扩展 安全间距
```

## 与实验二的接口

实验二只需要读取 `.fp` 文件并创建元件实例。板上实例应保存：

```text
footprintId   -> 引用的封装名称
refdes        -> R1、U1、J1
value         -> 10k、AMS1117-3.3
x, y          -> 板上位置
rotation      -> 旋转角度
pinNet        -> 引脚编号到网络名的映射
```

不要把焊盘坐标重新复制到板文件里。实验二应引用实验一的封装原型，再对原型应用平移、旋转和镜像。

## 当前范围

这是实验一的可运行最小版本。圆弧工具当前生成半圆，多边形通过右键结束。后续可继续加入圆弧起止角编辑、封装预览缩略图、库搜索、撤销重做和 DRC 间距检查。

详细模块说明见 [docs/架构与实现说明.md](docs/架构与实现说明.md)。

建议阅读顺序：

1. `docs/00_零基础先看_封装到底是什么.md`
2. `docs/名词解释与元件清单.md`
3. `docs/01_实验一功能与验收.md`
4. `docs/02_软件架构与修改入口.md`
5. `docs/架构与实现说明.md`
6. `docs/05_完整版与课程骨架代码量统计.md`


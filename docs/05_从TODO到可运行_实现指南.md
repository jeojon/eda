# 从 TODO 到可运行：实验一实现指南

这份文档说明课程骨架中哪些代码已经完成、哪些地方需要自己写，以及每个 TODO 应该如何拆成可验证的小步骤。

## 1. 先明确边界

这个工程把界面和核心逻辑分开：

```text
src/gui/       已经完成的 wxWidgets 界面
include/eda/   数据结构、接口和实现声明
src/todo/      需要补全的四个实现文件
footprints/    运行时生成的 .fp 封装库
tests/         可以自己增加的测试
```

需要补全的四个文件是：

1. `src/todo/FootprintEditorService.cpp`
2. `src/todo/GeometryService.cpp`
3. `src/todo/FootprintCodec.cpp`
4. `src/todo/FilesystemFootprintLibrary.cpp`

除非发现界面调用接口有错误，否则不要先改 GUI。先把接口实现好，才能保证实验二可以复用这套封装库。

所有长度单位统一为毫米（mm）。封装内部使用局部坐标，原点可以放在元件中心，也可以放在某个标准参考点。实验二放置元件时，只对封装原型做平移、旋转或镜像，不把焊盘坐标重新抄到板文件里。

## 2. 建议实现顺序

### 第一步：编辑逻辑

实现 `FootprintEditorService.cpp`，让点击工具按钮后能够真正修改 `Footprint`。

目标是先做到：

```text
点击 SMD 焊盘
-> pads 增加一个焊盘
-> number 自动变成 1、2、3...
```

### 第二步：几何逻辑

实现 `GeometryService.cpp`，让画布能够适配视图、选择对象和进行栅格吸附。

目标是：

```text
点击已有焊盘
-> HitTest 返回 Pad 和 index
-> 画布进入选中状态

鼠标移动
-> SnapToGrid 把坐标吸附到 0.1、0.5 或 1.0 mm
```

### 第三步：文本编解码

实现 `FootprintCodec.cpp`，先只做内存对象和字符串之间的转换。

目标是：

```text
Footprint -> Serialize -> std::string
std::string -> Deserialize -> Footprint
```

不要在这个文件里打开文件对话框，也不要直接写 `wxFile`。它只负责字符串。

### 第四步：文件系统元件库

实现 `FilesystemFootprintLibrary.cpp`，把编解码器接到真实目录。

目标是：

```text
扫描 footprints/*.fp
-> 读取每个 Footprint
-> 生成 LibraryEntry
-> 在左侧元件库中显示名称和分类
```

### 第五步：整体验收

最后再检查 GUI 到逻辑的完整链路：

```text
新建 -> 画焊盘 -> 选中 -> 修改属性 -> 保存
-> 刷新元件库 -> 重新打开 -> 再次保存
```

## 3. FootprintEditorService

文件：

```text
src/todo/FootprintEditorService.cpp
```

这个类只修改内存中的 `eda::Footprint`，不处理鼠标事件，也不处理文件。

### 3.1 焊盘默认值

建议先定义两个明确的默认值，后续 GUI 中再允许修改：

| 属性 | SMD 焊盘 | THT 引脚 |
|---|---:|---:|
| 类型 | `PadKind::Smd` | `PadKind::ThroughHole` |
| 形状 | `PadShape::RoundedRect` | `PadShape::Circle` |
| 宽 | 1.3 mm | 1.524 mm |
| 高 | 1.5 mm | 1.524 mm |
| 钻孔 | 0 | 0.965 mm |
| 阻焊扩展 | 0.1 mm | 0.1 mm |
| 安全间距 | 0.2 mm | 0.2 mm |

这些是课程示例值，不代表所有真实标准封装。真实器件要以 datasheet、IPC 标准或课程给定尺寸为准。

### 3.2 自动编号

接口没有提供 `NextNumber()`，可以在实现文件内部写一个小函数：

```text
number = 1
while 已有焊盘使用该编号:
    number = number + 1
return number
```

不要只使用 `pads.size() + 1`。如果删除了中间的焊盘，直接使用数组长度可能生成重复编号。

### 3.3 各方法的责任

#### `AddSmdPad`

输入是已经经过栅格吸附的模型坐标 `x/y`。

伪代码：

```text
pad.kind = Smd
pad.shape = RoundedRect
pad.number = 下一个未使用编号
pad.name = pad.number
pad.x = x
pad.y = y
pad.width = 1.3
pad.height = 1.5
pad.drill = 0
pad.maskExpansion = 0.1
pad.clearance = 0.2
pads.push_back(pad)
```

#### `AddThroughHolePin`

和 SMD 类似，但是：

```text
kind = ThroughHole
shape = Circle
width = height = 1.524
drill = 0.965
```

钻孔必须小于焊盘尺寸，否则在制造上没有形成可靠环形铜箔。

#### `AddLine`

创建 `SilkLine`：

```text
x1/y1 = 起点
x2/y2 = 终点
width = 0.15
```

线段属于丝印，不是铜箔，也不是板框。

#### `AddRect`

创建 `SilkRect`，保存两个对角点和线宽 0.15 mm。绘制时用 `min/max` 归一化，因此用户从左下或右上拖动都应该得到同一个矩形。

#### `AddArc`

当前 GUI 传入圆心和两个半径。可以先按下面的规则保存：

```text
cx/cy = 圆心
rx/ry = 半径
startDegrees = 0
sweepDegrees = 180
width = 0.15
```

后续如果界面增加起始角和扫过角，只需要修改结构体字段和绘制逻辑，不要让接口重新依赖鼠标坐标。

#### `AddPolygon`

先检查顶点数量：

```text
少于 3 个点：不创建，直接返回
至少 3 个点：复制所有 (x, y)，width = 0.15
```

多边形是闭合丝印轮廓，不是填充铜皮。若以后要做铺铜，应新增明确的铜层对象，不要偷偷把丝印多边形当成铜。

#### `AddText`

创建 `SilkText`：

```text
x/y = 文本左下角
size = 0.9
text = 用户输入的 UTF-8 字符串
```

界面通过 `wxString::FromUTF8` 显示文本，所以不要在 C++ 源码里直接把中文 `std::string` 传给 `%s`。

#### `UpdatePad`

必须做以下检查：

```text
index 在 [0, pads.size()) 范围内
updated.width > 0
updated.height > 0
updated.drill >= 0
如果 kind == THT，drill 应小于 min(width, height)
number 不能为空
```

检查通过后，直接替换 `pads[index]`，并保留列表中其他焊盘不变。失败时返回 `false`，不要修改原数据。

#### `MovePad`

只修改一个焊盘的位置：

```text
pads[index].x += dx
pads[index].y += dy
```

索引非法时返回 `false`。移动逻辑不要改变旋转、尺寸或钻孔。

#### `RemoveItem`

根据 `ItemType` 和索引删除对应容器中的元素：

| `ItemType` | 容器 |
|---|---|
| `Pad` | `pads` |
| `Line` | `lines` |
| `Rect` | `rects` |
| `Arc` | `arcs` |
| `Polygon` | `polygons` |
| `Text` | `texts` |
| `None` | 不处理，返回 `false` |

删除前检查索引。删除后画布会清空当前选择，因此不需要在服务里保存选择状态。

### 3.4 这一部分的验收

- 连续添加三个 SMD 焊盘，编号为 `1/2/3`。
- 删除编号 2 的焊盘，再添加新焊盘，编号不能与现有编号重复。
- 添加 THT 引脚，`kind` 为 `ThroughHole` 且 `drill > 0`。
- 添加线、矩形、圆弧、多边形和文本后，容器大小分别增加 1。
- 修改焊盘属性后，原对象被替换，其他对象不变。

## 4. GeometryService

文件：

```text
src/todo/GeometryService.cpp
```

这个类只做纯数学计算，不创建前景色、不弹窗、不访问文件。

### 4.1 `ComputeBounds`

边界框要覆盖所有可见图元。初学时可以按下面的规则实现：

#### 焊盘

没有旋转时：

```text
minX = x - width / 2
maxX = x + width / 2
minY = y - height / 2
maxY = y + height / 2
```

如果焊盘有旋转，不能只取未旋转的矩形边界。应计算四个角：

```text
dx = ±width / 2
dy = ±height / 2
cornerX = x + dx*cos(theta) - dy*sin(theta)
cornerY = y + dx*sin(theta) + dy*cos(theta)
```

其中 `theta = rotation * pi / 180`。然后对四个角取最小值和最大值。

#### 线段

把两个端点加入边界。

#### 矩形

把 `(x1, y1)` 和 `(x2, y2)` 两个对角点加入边界。

#### 圆弧

课程要求可以先使用完整椭圆的保守边界：

```text
minX = cx - rx
maxX = cx + rx
minY = cy - ry
maxY = cy + ry
```

这比实际圆弧略大，但不会导致视图裁掉图形。若要做精确边界，再根据 `startDegrees` 和 `sweepDegrees` 判断圆弧经过的最大、最小角度点。

#### 多边形

把每个顶点加入边界。

#### 文本

初版可以估算：

```text
宽约 = size * 0.6 * 字符数
高约 = size
```

这里只用于视图定位和选择辅助，不需要追求字体的像素级精确。

如果封装没有任何图元，返回一个有效的默认范围，例如 `(-5, -5)` 到 `(5, 5)`，避免 `FitView` 除以 0。

### 4.2 `SnapToGrid`

公式：

```text
round(value / gridSize) * gridSize
```

必须先检查 `gridSize > 0`。如果传入 0 或负数，可以返回原值，避免除零。

### 4.3 `HitTest`

`HitTest` 接收模型坐标和模型空间容差。界面通常传入 `8 / zoom`，所以服务内部不要把容差固定成 8 mm。

建议顺序：

1. 焊盘
2. 文本
3. 线段
4. 矩形
5. 圆弧
6. 多边形

每一类都从后往前找，因为后添加的图元显示在上层。找到第一个对象后返回对应的 `ItemType` 和索引。

#### 焊盘命中

未旋转时可以先使用扩展后的矩形：

```text
abs(px - x) <= width / 2 + tolerance
且
abs(py - y) <= height / 2 + tolerance
```

有旋转时，把鼠标点反向旋转到焊盘局部坐标，再做上面的判断。

#### 线段命中

计算鼠标点到线段的最短距离，距离不超过 `tolerance` 就算命中。

二维点到线段距离的步骤：

```text
向量 ab = (x2 - x1, y2 - y1)
向量 ap = (px - x1, py - y1)
t = dot(ap, ab) / dot(ab, ab)
t 限制在 [0, 1]
最近点 = a + t * ab
距离 = length(p - 最近点)
```

线段长度为 0 时直接计算点到端点的距离。

#### 矩形命中

矩形图元是轮廓，不是填充面。更符合直觉的规则是：

```text
点在矩形外扩 tolerance 的范围内
并且点到四条边之一的最短距离 <= tolerance
```

如果课程要求“点内部也能选中”，可以在此规则上增加内部判断，但要在文档中明确。

#### 圆弧命中

1. 计算鼠标相对圆心的角度。
2. 把角度转换到 `[0, 360)`。
3. 判断角度是否落在 `startDegrees` 到 `startDegrees + sweepDegrees` 之间。
4. 计算鼠标到圆心的距离，和平均半径 `(rx + ry) / 2` 比较，误差不超过 `tolerance`。

如果 `rx` 和 `ry` 不相等，严格算法还应使用椭圆参数方程；课程初版可以先按平均半径或归一化坐标处理。

#### 多边形命中

不要只判断包围盒。至少遍历每条边，使用线段距离判断；如果要做“点击内部选中”，再增加射线法或奇偶规则。

### 4.4 这一部分的验收

- 空封装调用 `ComputeBounds` 仍然返回有效范围。
- 添加多个图元后，边界覆盖所有图元。
- 鼠标点在焊盘外接区域内时，`HitTest` 返回正确的焊盘索引。
- 点在丝印线附近命中线，点在线段中间但远离线段时不命中。
- 点击上层对象时优先返回最后添加的对象。
- `SnapToGrid(1.26, 0.5)` 返回 `1.5`，`SnapToGrid(1.24, 0.5)` 返回 `1.0`。

## 5. FootprintCodec

文件：

```text
src/todo/FootprintCodec.cpp
```

这个文件定义封装文件格式。它是实验二读取封装原型的入口，所以格式一旦确定，要尽量保持兼容。

### 5.1 建议格式

使用纯文本、UTF-8 和毫米单位：

```text
EDALAB1_FOOTPRINT 1
NAME R_0805
CATEGORY Passive
DESC 0805 SMD resistor/capacitor land pattern
SILKCOLOR #1A6984
ORIGIN 0 0
PAD SMD ROUNDED 1 1 -0.9 0 1.3 1.5 0 0 0.1 0.2
LINE -0.5 -0.8 0.5 -0.8 0.12
RECT -0.5 -0.8 0.5 0.8 0.12
ARC 0 0 1.0 1.0 0 180 0.15
POLY 3 0 0 1 0 0 1 0.15
TEXT 0 1.1 0.8 "R?"
END
```

字段含义：

```text
PAD 类型 形状 编号 名称 X Y 宽 高 旋转 钻孔 阻焊扩展 安全间距
```

其中：

- `NAME`、`CATEGORY`、`DESC` 读取整行剩余内容，可以包含空格。
- `PAD` 的编号和名称建议不允许空格；如果必须允许，改用 `std::quoted`。
- `TEXT` 使用带引号的字符串，避免将空格误拆成多个字段。
- `#` 开头可以作为注释行。
- `END` 表示文件结束。

### 5.2 `Serialize`

建议用 `std::ostringstream`，不要手工拼接 `char*`。

流程：

```text
输出版本头
输出封装基本属性
遍历 pads
遍历 lines
遍历 rects
遍历 arcs
遍历 polygons
遍历 texts
输出 END
```

数值输出建议：

```cpp
output << std::setprecision(10);
```

这样可以保留课程尺寸所需的小数精度。不要把本地化设置带进数字格式，避免某些机器输出逗号小数点。

### 5.3 `Deserialize`

建议分三步：

1. 按行读取，忽略空行和注释。
2. 第一行必须是 `EDALAB1_FOOTPRINT 1`。
3. 根据关键字解析后面的字段。

伪代码：

```text
清空 footprint
sawHeader = false
逐行读取:
    keyword = 行的第一个词
    if keyword == EDALAB1_FOOTPRINT:
        读取 version，只有 version == 1 才继续
    else if keyword == NAME/CATEGORY/DESC/SILKCOLOR:
        getline 读取整行剩余内容
    else if keyword == PAD:
        读取完整字段，构造 PadItem，加入 pads
    ...
    else if keyword == END:
        结束
```

合法性检查建议：

- 没有版本头：返回错误。
- 版本不是 1：返回错误。
- 坐标、尺寸或线宽无法读取：返回错误。
- 宽、高小于等于 0：返回错误。
- THT 的钻孔大于焊盘短边：返回错误或修正并报错。
- 文件没有 `END`：可以接受，但要明确这是兼容策略。
- 名称为空：使用 `Untitled`。
- 分类为空：使用 `Uncategorized`。
- 颜色为空：使用 `#1A6984`。

错误信息写到 `std::string* error`，不要弹窗。界面层会负责显示。

### 5.4 这一部分的验收

- `Serialize` 后再 `Deserialize`，封装名、分类、描述、坐标、焊盘、丝印和文本数量一致。
- 浮点数经过保存和读取后误差小于容差。
- 缺版本头、版本错误和字段损坏时返回 `false`，并给出错误文本。
- 空名称、空分类、空颜色恢复到默认值。

## 6. FilesystemFootprintLibrary

文件：

```text
src/todo/FilesystemFootprintLibrary.cpp
```

这个类负责“文件系统中的元件库”，不能把目录扫描写进 GUI，也不能把文件读写写进 Codec。

### 6.1 `DefaultDirectory`

推荐返回可执行文件同目录下的 `footprints`，不要依赖当前工作目录。因为从 Visual Studio、PowerShell 或桌面快捷方式启动时，当前工作目录可能不同。

可以用 Windows API `GetModuleFileNameW` 或 C++17 `std::filesystem` 来实现：

```text
exePath = 当前可执行文件路径
return exePath.parent / "footprints"
```

如果在 VS 中调试，也可以额外允许环境变量或命令行覆盖目录，但课程版本不要求。

### 6.2 `Reload`

流程：

1. 如果目录不存在，创建目录。
2. 清空旧的 `entries_`。
3. 遍历 `*.fp` 文件。
4. 对每个文件调用 `Read`。
5. 把路径、`name`、`category` 加入 `LibraryEntry`。
6. 按分类、名称排序。
7. 返回成功；如果单个文件坏了，记录错误并跳过，避免一个坏文件让整个库打不开。

伪代码：

```text
create_directories(root)
entries.clear()
for path in directory_iterator(root):
    if extension == ".fp":
        if Read(path, footprint, &error):
            entries.push_back({path, footprint.name, footprint.category})
sort(entries, category then name)
return true
```

### 6.3 `Read`

流程：

```text
打开文件
读取全部字节
调用 codec_->Deserialize(bytes, footprint, &error)
转换错误文本为 std::wstring
失败时返回 false
成功时返回 true
```

需要检查：

- 文件打开失败。
- 文件长度异常。
- 读取字节数不等于文件长度。
- UTF-8 解码后的名称可以为空，但结构本身必须有效。

### 6.4 `Write`

流程：

```text
调用 codec_->Serialize(footprint)
确保父目录存在
先写临时文件
写入成功后替换目标文件
失败时返回 false
```

课程项目可以第一版直接覆盖写入，但推荐使用临时文件再重命名，避免程序崩溃时把原封装写坏。

### 6.5 `Delete`

至少做这些安全判断：

```text
路径扩展名必须是 .fp
路径必须在元件库根目录下
文件存在才执行删除
删除失败返回错误
```

不要直接删除用户传入的任意路径。实验二是长期使用这套库的，安全边界要在实验一就确定。

### 6.6 这一部分的验收

- 程序启动后能创建 `footprints` 目录。
- 目录中的 `.fp` 文件能出现在左侧列表。
- 修改名称或分类并刷新后，列表顺序随之变化。
- 保存后重新启动，封装仍能打开。
- 删除一个封装后，刷新列表不再显示它。
- 不在库目录下的文件不能被 `Delete` 误删。

## 7. GUI 到逻辑的调用链

### 放置焊盘

```text
MainFrame 工具按钮
-> FootprintCanvas::OnLeftDown
-> ScreenToModel
-> IGeometryService::SnapToGrid
-> IFootprintEditor::AddSmdPad
-> pads.push_back
-> NotifyChanged
-> MainFrame::OnCanvasChanged
-> Refresh
```

### 点击选择

```text
OnLeftDown
-> IGeometryService::HitTest
-> selection = HitResult
-> MainFrame::OnSelectionChanged
-> 右侧属性面板刷新
```

### 保存

```text
MainFrame::SaveCurrent
-> SyncMetadata
-> IFootprintLibrary::Write
-> IFootprintCodec::Serialize
-> 写入 .fp
```

### 打开

```text
MainFrame::LoadSelectedLibraryEntry
-> IFootprintLibrary::Read
-> IFootprintCodec::Deserialize
-> currentFootprint_
-> canvas_->SetFootprint
-> RefreshProperties
```

## 8. 建议增加的测试

第一版不需要做复杂测试框架，可以先写一个普通控制台程序，返回 0 表示成功。

建议至少覆盖：

| 测试 | 输入 | 期望 |
|---|---|---|
| 添加 SMD | 空 Footprint | `pads.size() == 1` |
| 自动编号 | 添加两次 | 编号为 `1, 2` |
| 添加 THT | 空 Footprint | `kind == ThroughHole` 且 `drill > 0` |
| 编辑焊盘 | 合法 index | 属性改变，数量不变 |
| 非法编辑 | index = -1 | 返回 `false`，数据不变 |
| 边界 | 空封装 | `valid == true` |
| 边界 | 两个位于 ±5 的焊盘 | 包含 -5 到 +5 |
| 吸附 | `1.26, 0.5` | `1.5` |
| 命中 | 焊盘中心 | 返回 Pad/index |
| 编解码 | 一个含全部图元的封装 | 数量与关键字段一致 |
| 库扫描 | 临时目录两个 `.fp` | 返回两个条目 |

可以先把测试放在：

```text
tests/
```

不要把测试文件放进 `src/gui/`，否则界面和核心逻辑又会混在一起。

## 9. 实验二如何接上

实验一保存的是“封装原型”，实验二保存的是“板上实例”，两者不要混淆。

### 实验一 `.fp` 中保存

```text
封装名称
分类
描述
丝印颜色
参考点
焊盘、引脚、钻孔、阻焊扩展、安全间距
丝印线、矩形、圆弧、多边形、文本
```

### 实验二板文件中保存

```text
footprintId   引用的封装名，例如 R_0805
refdes        位号，例如 R1、U1、J1
value         值，例如 10k、AMS1117-3.3
x, y          板上位置
rotation      旋转角度
pinNet        引脚编号到网络名的映射
```

实验二不要重复保存每个焊盘的局部坐标。正确做法是：

```text
读取 .fp
-> 得到封装原型
-> 对原型做平移/旋转/镜像
-> 再叠加到板图上
```

这样修改封装库后，所有引用该封装的实例都会使用同一套焊盘定义。

## 10. 哪些部分适合让 AI 辅助

AI 适合帮助你做这些“外围且可验证”的工作：

- 解释编译错误和 wxWidgets 事件绑定。
- 生成 `CMakeLists.txt`、构建脚本和文档目录。
- 生成测试用例的输入数据。
- 帮你检查边界条件、格式版本和错误处理。
- 根据你写好的伪代码做代码审查。

以下部分建议由你自己写并解释清楚：

- `ComputeBounds` 和 `HitTest` 的数学逻辑。
- `.fp` 格式的字段选择与兼容策略。
- 自动编号、钻孔合法性和删除索引判断。
- 文件库路径安全边界。

即使让 AI 生成代码，也要能够说明每个字段的含义、为什么使用这个公式，以及失败时数据为什么不能改变。实验的难点不是把代码敲出来，而是能把 PCB 封装的数据模型和操作链路讲明白。

## 11. 最终提交检查表

- [ ] 四个 TODO 文件都有实现，没有留下空函数。
- [ ] SMD、THT、丝印线和矩形可以创建和选择。
- [ ] 至少能保存并重新读取一个封装。
- [ ] 元件库能够扫描并显示至少两个 `.fp` 文件。
- [ ] 中文名称、描述和文本不会乱码。
- [ ] 空文件、坏版本和损坏字段不会崩溃。
- [ ] 实验二只依赖封装 ID 和实例属性，不复制焊盘坐标。
- [ ] 文档说明了单位、坐标、封装与实例的区别。


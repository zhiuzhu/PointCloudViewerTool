# PointCloudViewerTool（VS2026 / C++，非 CMake 方案）

这是一个 **Visual Studio 解决方案（.sln/.vcxproj）** 的点云查看小工具：
- 选择指定文件夹后，自动扫描 `*.pcd` / `*.ply`。
- 文件列表支持**多选**，多选文件会在**同一个 3D 窗口**中同时显示。
- 可以实时调整点云显示颜色（RGB）和点大小。
- 通过文件系统监听 + 定时刷新，实现“目录内容变化后的实时更新”。

## 1) 你需要安装什么

在 Windows 上，至少安装：

1. **Visual Studio 2026**
   - 勾选工作负载：`使用 C++ 的桌面开发`
2. **vcpkg**（推荐）
3. 依赖库（通过 vcpkg 安装）：
   - `qtbase`
   - `vtk[qt]`
   - `pcl`

> 工程根目录已提供 `vcpkg.json`（manifest 模式），并且 `.vcxproj` 已开启 `VcpkgEnableManifest=true`。

## 2) 安装依赖（一次性）

```powershell
# 进入项目目录后
vcpkg install --triplet x64-windows
```

如果你的 VS 未做 vcpkg 集成，执行：

```powershell
vcpkg integrate install
```

## 3) 打开与运行（不用 CMake）

1. 用 VS2026 打开 `PointCloudViewerTool.sln`
2. 选择 `x64` + `Debug` 或 `Release`
3. 直接编译运行（`F5`）

生成的可执行文件默认在：

- `bin/Debug/PointCloudViewerTool.exe`
- `bin/Release/PointCloudViewerTool.exe`

## 4) 功能使用

1. 点击“选择点云文件夹”
2. 在左侧列表中单选/多选点云文件
3. 用 RGB 滑块调整颜色
4. 用“点大小”调整渲染点尺寸
5. 多选时会在同一个窗口里显示多个点云（程序会沿 X 方向稍微错开，避免完全重叠）

## 5) 支持格式

- `.pcd`
- `.ply`

## 6) 常见问题

1. **编译时报找不到 Qt/VTK/PCL 头文件**
   - 确认已执行 `vcpkg install --triplet x64-windows`
   - 确认 VS 工程使用 `x64` 配置
2. **运行时报 DLL 缺失**
   - 对 Qt 可使用 `windeployqt` 复制运行时
   - 或把 vcpkg 对应 triplet 的 DLL 路径加入系统 PATH

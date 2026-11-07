<!---

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

-->

# Intel(R) XPU Manager 与 XPU 系统管理接口

Intel(R) XPU Manager 是一款用于监控和管理 Intel 数据中心 GPU 的自由开源工具。
它的设计目标是简化运维管理，并帮助提升设备利用率、最大化可靠性和可用性。
Intel(R) XPU 系统管理接口（XPU-SMI）是一个用于在本地管理 GPU 的命令行界面（CLI）。

## 目录

- [Intel(R) XPU Manager 功能特性](#intelr-xpu-manager-功能特性)
- [SMI 输出示例](#smi-输出示例)
  - [xpu-smi 默认输出](#xpu-smi-默认输出)
  - [运行 xpu-smi discovery](#运行-xpu-smi-discovery)
- [如何获取 XPU Manager](#如何获取-xpu-manager)
  - [安装 XPUM](#安装-xpum)
  - [Windows 命令行工具](#windows-命令行工具)
  - [GPU 信息导出器（xpumd）](#gpu-信息导出器xpumd)
- [支持的设备](#支持的设备)
- [支持的操作系统](#支持的操作系统)
- [运行时依赖](#运行时依赖)
- [发布节奏](#发布节奏)
- [文档](#文档)

## Intel(R) XPU Manager 功能特性

* 管理：
  * GPU 发现与信息查询 —— 名称、型号、序列号、步进（stepping）、位置、频率、显存容量、固件版本
  * GPU 拓扑
  * GPU 固件更新，包括 GPU GFX 固件与 AMC（Add-in Card Management Controller，加插卡管理控制器）固件更新
  * GPU 崩溃日志管理（仅 Linux）—— 通过 `iclg` 工具启用、禁用、触发、清除并导出 Intel 崩溃日志记录
* 监控：
  * GPU 遥测数据 —— 利用率、功耗、频率、温度、互连（fabric）速率、显存吞吐、错误
  * GPU 健康状况 —— 显存、功耗、温度、互连端口
  * [GPU 指标导出守护进程](xpumd/README.md)
* 配置：
  * GPU 设置 —— GPU 功耗上限、频率范围、待机模式、调度器模式、ECC 开关、互连端口状态

## SMI 输出示例

下面 `xpu-smi` 输出示例中的部分字段可能因所使用的平台不同而有所差异。

### xpu-smi 默认输出

```
$ xpu-smi
+----------------------------------------------+----------------------------+------------------------+
|                     Intel XPU-SMI v2.0    KMD Version: xeb_v7.1.4.31_260728.26                     |
|                            UMD Version: 1.3.36012    Level Zero: 1.27.0                            |
+----------------------------------------------+----------------------------+------------------------+
| GPU  Name                    Persistence-M   | Bus-Id            Disp.A   |   Volatile Uncorr. ECC |
| Fan  Temp  Pwr:Usage/Cap                     | Memory-Usage               |   GPU-Util  Compute M. |
+----------------------------------------------+----------------------------+------------------------+
|   0  Intel(R) Arc(TM) B580   Off             | 0000:03:00.0      Off      |               Disabled |
| N/A    25C  66W / 380W                       | 239MiB / 12216MiB          |      0%        Default |
+----------------------------------------------+----------------------------+------------------------+

+-------+-----------+--------+----------------+------------------------------------------------------+
| Processes:                                                                                         |
|   GPU         PID    Type    Process Name                                         GPU Memory Usage |
+-------+-----------+--------+----------------+------------------------------------------------------+
|     0     2874292     C      xpu-smi                                                       206 MiB |
+-------+-----------+--------+----------------+------------------------------------------------------+
```

### 运行 xpu-smi discovery

```
$ xpu-smi discovery -d 0
+-----------+--------------------------------------------------------------------------------------+
| Device ID | Device Information                                                                   |
+-----------+--------------------------------------------------------------------------------------+
| 0         | Device Type: GPU                                                                     |
|           | Device Name: Intel(R) Arc(TM) B580 Graphics                                          |
|           | Device State: normal                                                                 |
|           | PCI Device ID: 0xe20b                                                                |
|           | Vendor Name: Intel(R) Corporation                                                    |
|           | SOC UUID: 00000000-0000-0003-0000-0000e20b8086                                       |
|           | Serial Number: unknown                                                               |
|           | Core Clock Rate: 2850 MHz                                                            |
|           | Stepping: A0                                                                         |
|           | SKU Type: Production ES                                                              |
|           |                                                                                      |
|           | UMD Version: 1.3.36012                                                               |
|           | KMD Version: xeb_v7.1.4.31_260728.26                                                 |
|           | Kernel Version: 6.19.0-rc6                                                           |
|           | GFX Firmware Name: GFX                                                               |
|           | GFX Firmware Version:                                                                |
|           | GFX Firmware Status: normal                                                          |
|           | GFX Data Firmware Name: GFX_DATA                                                     |
|           | GFX Data Firmware Version: Major : , OEM Manufacturing Data : , Major VCN :          |
|           |                                                                                      |
|           | PCI BDF Address: 0000:03:00.0                                                        |
|           | PCI Slot: PCIEx16(G5)                                                                |
|           | PCIe Generation: 4                                                                   |
|           | PCIe Max Link Width: 8                                                               |
|           | PCIe Max Bandwidth: 15.75 GB/s                                                       |
|           |                                                                                      |
|           | Memory Physical Size: 12216.00 MiB                                                   |
|           | Max Mem Alloc Size: 11605.20 MiB                                                     |
|           | ECC State: disabled                                                                  |
|           | Number of Memory Channels: 12                                                        |
|           | Memory Bus Width: 384                                                                |
|           | Max Hardware Contexts: 65536                                                         |
|           | Max Command Queue Priority: 0                                                        |
|           |                                                                                      |
|           | Number of EUs: 160                                                                   |
|           | Number of Tiles: 1                                                                   |
|           | Number of Slices: 5                                                                  |
|           | Number of Sub Slices per Slice: 4                                                    |
|           | Number of Threads per EU: 8                                                          |
|           | Physical EU SIMD Width: 16                                                           |
|           | Number of Media Engines: 2                                                           |
|           | Number of Media Enhancement Engines: 2                                               |
+-----------+--------------------------------------------------------------------------------------+
```

## 如何获取 XPU Manager

### 安装 XPUM

以下说明适用于 Ubuntu（使用 APT 安装）。

由于 Linux 发行版可能未提供 XPUM 所需的全部驱动软件包，或提供的版本过旧，
请先参考
[Intel GPU 驱动文档](https://dgpu-docs.intel.com/installation-guides/installing-packages-from-the-intel-ppa.html)
中"Installing packages from the Intel PPA"一节的说明，配置 APT 仓库以添加
`kobuk-team/intel-graphics` PPA。该 PPA 同时提供 Intel GPU 驱动软件包和 `xpu-smi`。

**方式一：从 Intel 软件包仓库安装**

安装 `xpu-smi` 及其（驱动）依赖项：

````bash
sudo apt install xpu-smi
````

> **注意：** Kobuk PPA 中发布的版本可能比
> [GitHub Releases](https://github.com/intel/xpumanager/releases) 滞后数周，
> 因此此方式可能无法安装到最新的构建版本。如果需要在版本发布后立即获取，请使用下方的方式二。

**方式二：从发布页面（releases page）安装**

从[发布页面](https://github.com/intel/xpumanager/releases)下载与宿主发行版相对应的 XPUM 软件包。

进行安装：

````bash
sudo apt install ./xpu-smi_*.deb
````

使用 `apt` 从本地 `.deb` 路径安装会自动解析驱动依赖项。如有需要，也可显式安装：

````bash
sudo apt install libhwloc15 libpciaccess0 libigsc1 libze1 libze-intel-gpu1
````

**故障排查**

如果驱动软件包相对本版本 XPUM 过旧，安装可能会失败。可尝试从
[Kobuk 预发布仓库](https://launchpad.net/~kobuk-team/+archive/ubuntu/intel-graphics-staging)获取更新的版本。

如果仍未解决，可执行以下命令修复损坏的依赖关系：

````bash
sudo apt --fix-broken install
````

提交工单：https://github.com/intel/xpumanager/issues

### Windows 命令行工具

最新的安装包/二进制文件可从 XPUM [发布页面](https://github.com/intel/xpumanager/releases)下载。

### GPU 信息导出器（xpumd）

XPUM 守护进程以容器镜像形式提供，请参阅 [`xpumd` README](xpumd/README.md)。

## 支持的设备

* Intel(R) Arc Pro 系列 GPU（[GPU 驱动安装指南](https://dgpu-docs.intel.com/installation-guides/index.html)）

## 支持的操作系统

* XPU-SMI
  * Ubuntu 24.04.3、Ubuntu 26.04
  * Windows Server 2022 与 2025（仅部分功能：GPU 设备信息、GPU 遥测、GPU 固件更新与 GPU 配置）

## 运行时依赖

对于 Ubuntu 22.04 及更高版本，可从 [Intel 软件包仓库](https://dgpu-docs.intel.com/)安装所需的 Intel 驱动。

依赖项版本：

- [LevelZero Loader](https://github.com/oneapi-src/level-zero/releases)（v1.27.0 或更高版本）与 [Intel Graphics Compute Runtime](https://github.com/intel/compute-runtime/releases)
- [meetee](https://github.com/intel/metee/releases)
- [IGSC](https://github.com/intel/igsc/releases)
- [CLI11](https://github.com/CLIUtils/CLI11/releases)（v2.6.2 或更高版本）

系统软件包：

关于系统软件包依赖项，请参阅[前置条件](./BUILDING.md#prerequisites)。

## 发布节奏

XPU Manager 采用 `MAJOR.MINOR.PATCH` 形式的[语义化版本](https://semver.org/)。

- 次版本号（Minor）会定期发布，用于在功能与平台支持趋于稳定后交付。
- 修订版本号（Patch）按需发布，用于关键缺陷修复与安全更新。

## 文档

* 使用：命令参考与使用细节请参阅生成的 [XPU-SMI 文档](https://intel.github.io/xpumanager/)。
* 构建：请参阅 [Building XPU Manager Installer](./BUILDING.md)以构建 XPU-SMI 安装包。
* 贡献：请参阅 [CONTRIBUTING](./CONTRIBUTING.md)
* 问题反馈：报告问题时，请使用建议的模板：https://github.com/intel/xpumanager/issues

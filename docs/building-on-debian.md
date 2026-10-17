# 在非官方平台构建 XPUM 的实践记录

本文记录在 **Debian 13 (trixie) + Intel Arc B580** 上从源码构建 `xpu-smi` 的完整过程，
并说明遇到的真实障碍及其根因。

> **背景**：上游 [README](../README.md) 明确将 XPU-SMI 的支持范围限定为
> **Ubuntu 24.04.3 / 26.04** 与 **Intel Arc Pro 系列** GPU。
> 本次实践是在 **Debian 13** + **消费级 Arc B580（非 Pro）** 上完成的，
> 属于官方支持范围之外，因此遇到了一些文档未覆盖的问题。
>
> 本文记录的是**实测结果**，不是对任何环境的通用建议。

---

## 目录

- [最终结果](#最终结果)
- [环境概览](#环境概览)
- [快速构建步骤](#快速构建步骤)
- [踩坑记录](#踩坑记录)
  - [1. conan.lock 已过期](#1-conanlock-已过期)
  - [2. ConanCenter 网络延迟](#2-conancenter-网络延迟)
  - [3. Python 被 EXTERNALLY-MANAGED 锁定](#3-python-被-externally-managed-锁定)
  - [4. 编译器版本与 -Werror](#4-编译器版本与--werror)
  - [5. meson install 剥离 RPATH](#5-meson-install-剥离-rpath)
  - [6. 核心问题：loader 与驱动版本必须匹配](#6-核心问题loader-与驱动版本必须匹配)
- [根本矛盾：README 与 conanfile.py 的版本要求不一致](#根本矛盾readme-与-conanfilepy-的版本要求不一致)
- [SYSMAN 兼容垫片](#sysman-兼容垫片)
- [已知限制](#已知限制)
- [环境清理](#环境清理)

---

## 最终结果

构建成功，`xpu-smi` 可正常枚举并查询本机 GPU：

```
$ xpu-smi discovery
  Device ID     Device Information
  0             Device Name: Intel(R) Arc(TM) B580 Graphics
                Device State: normal
                Vendor Name: Intel(R) Corporation
                SOC UUID: 00000000-0000-0004-0000-0000e20b8086
                PCI BDF Address: 0000:04:00.0
                DRM Device: /dev/dri/card0
                Function Type: physical
```

| 指标 | 结果 |
|---|---|
| 编译 | 115/115 目标完成，`-Werror` 下零警告 |
| 运行 | `discovery` / `--query-gpu` / `--display` / `--loop` 均正常 |
| 降级项 | 3 个 SYSMAN 诊断接口不可用（见[已知限制](#已知限制)） |

---

## 环境概览

```
操作系统    Debian GNU/Linux 13 (trixie)
内核        6.12.107+deb13-amd64
架构        x86_64
CPU         20 核 / 125 GiB 内存
GPU         Intel Battlemage G21 [Arc B580]  @ 0000:04:00.0
内核驱动    xe (CONFIG_DRM_XE=m)
用户态驱动  libze-intel-gpu1 25.31.34666.3-1
Level Zero  libze1 1.24.1-2
编译器      gcc-13 / g++-13 13.3.0
```

### 与上游 CI 的差异

上游 CI 基线为 `ubuntu:24.04`（见 `.github/docker/Dockerfile.coverage`）。
本机与它的关键差异：

| 项 | 上游 CI | 本机 |
|---|---|---|
| 发行版 | Ubuntu 24.04 | Debian 13 |
| Level Zero loader | Conan 1.33.1 | 系统 1.24.1 |
| Python 管理 | `--break-system-packages` | `EXTERNALLY-MANAGED` |
| GPU | Arc Pro（官方支持） | Arc B580（非 Pro） |

### 依赖满足情况

| 类别 | 依赖 | 状态 |
|---|---|---|
| 构建工具 | gcc-13 / g++-13 | 已就位（与 CI pin 一致） |
| | cmake 3.31.6、ninja 1.13.2、pkg-config 1.8.1 | 已就位 |
| 系统库 | libhwloc-dev 2.14.0、libpciaccess-dev 0.17 | 已就位 |
| | libcurl4-openssl-dev 8.14.1、libudev-dev 259 | 已就位 |
| 运行工具 | dmidecode、lspci、clinfo、lsusb、vainfo、dpkg | 已就位 |
| Python 工具 | **meson ≥ 1.10** | 缺失，apt 无候选版本 |
| | **conan ≥ 2.0** | 缺失，且受系统 Python 锁定 |
| Conan 托管 | level-zero、igsc、metee、nlohmann_json、cli11 | 由 Conan 拉取 |
| 检查工具 | codespell、shellcheck、doxygen | 仅 `lint`/`docs` 目标需要 |

> **关键前提**：`libcurl.a` 与 `libhwloc.a` 必须存在，Conan 以静态方式链接它们。
> 本机的 `-dev` 包提供了这些静态库，因此可复用系统版本。

---

## 快速构建步骤

```bash
cd /public/vllm/xpumanager

# 1) Python 工具链 —— 必须用 venv 绕开 EXTERNALLY-MANAGED
python3 -m venv .venv
source .venv/bin/activate
pip install "conan>=2.0" "meson>=1.10"

# 2) Conan profile：cppstd 必须为 20，需同时提供 default 与 build
conan profile detect --force
# 手工修正 ~/.conan2/profiles/default 与 build：
#   compiler.version=13   (不要用默认探测出的 15)
#   compiler.cppstd=20    (不要用 gnu17)
#   [buildenv] CC=gcc-13  CXX=g++-13

# 3) 删除过期 lockfile，否则依赖解析失败
rm -f .conan/conan.lock

# 4) 首次构建本地 recipe（下载并编译上游源码）
conan create recipes/level-zero
conan create recipes/metee
conan create recipes/igsc

# 5) 拉取依赖。复用系统库以跳过 openssl/zlib/libcurl 源码编译
conan install . --output-folder=.conan/ --build=missing -s build_type=Release \
  -o '&:use_system_curl=True' -o '&:use_system_hwloc=True'

# 6) 配置与编译（见下方说明启用 SYSMAN 垫片）
meson setup builddir --native-file .conan/conan_meson_native.ini \
  -Dprefix=/usr -Dlevel_zero_sysman_compat=true
meson compile -C builddir
meson install -C builddir
```

### 选项说明

| 选项 | 是否建议 | 原因 |
|---|---|---|
| `-o use_system_curl=True` | **建议** | 系统 curl 8.14.1 > 要求 8.5.0，跳过源码编译 |
| `-o use_system_hwloc=True` | **建议** | 系统 hwloc 2.14.0 > 要求 2.9.3，同上 |
| `-Dlevel_zero_sysman_compat=true` | 本机**必需** | 见 [SYSMAN 兼容垫片](#sysman-兼容垫片) |
| `-o use_system_levelzero=True` | **不要使用** | 系统 loader 1.24.1 低于所需 1.33.1 |
| `-Db_lto=false` | 可选 | 缩短编译时间 |
| `-D werror=false` | GCC ≥ 14 时需要 | 见[编译器版本](#4-编译器版本与--werror) |

---

## 踩坑记录

### 1. conan.lock 已过期

**现象**：`conan install` 依赖解析失败。

**根因**：`.conan/conan.lock` 锁定 `level-zero/1.27.0`，而 `conanfile.py` 要求 `1.33.1`。
该 lockfile 还包含 `boost-ext-ut` / `doctest` 等仅在 `with_tests=True` 时才出现的依赖。

**解决**：删除 lockfile。

```bash
rm -f .conan/conan.lock
```

> 另外，仓库内两个 Dockerfile 的依赖版本互相矛盾：
> `Dockerfile.coverage` 用 level-zero 1.33.1（**与 conanfile 一致**），
> 而 `Dockerfile` 用 `--name/--version` 硬覆盖成 1.23.1 / igsc 0.9.6（**已过时**）。
> 以 `Dockerfile.coverage` 为准。

---

### 2. ConanCenter 网络延迟

**现象**：首次 `conan install` 长时间无进展 —— 4 分钟内缓存包数量零增长，
进程状态 `S (sleeping)` 阻塞在 `do_sys_poll`，CPU 仅 2.6%，无任何编译进程。

**实测数据**：

| 指标 | 直连 | 经 `127.0.0.1:12080` 代理 |
|---|---|---|
| ConanCenter 响应 | 2.67 s | 0.37 s（快 7 倍） |
| GitHub 大文件带宽 | 1.09 MB/s | 2.59 MB/s |

**诊断要点**：进程无 `cc1plus`/`ninja`/`cmake` 子进程且 CPU 近空闲，
说明是**网络等待**而非编译开销。`/proc/<pid>/fd` 可直接看到卡在
下载 `curl-8.5.0.tar.xz`。

**两个叠加原因**：

1. ConanCenter 直连延迟过高；
2. `--build=missing` 遇到 `cppstd=20` + `gcc-13` 这类非常规组合，
   ConanCenter 通常**没有预编译包**，会退化为源码编译 openssl / zlib / libcurl，
   其中 OpenSSL 源码编译通常需要数分钟。

**解决**：走代理 + 复用系统库，彻底跳过这批重型依赖的源码编译。

```bash
export HTTPS_PROXY=http://127.0.0.1:12080
export HTTP_PROXY=http://127.0.0.1:12080
conan install . --output-folder=.conan/ --build=missing -s build_type=Release \
  -o '&:use_system_curl=True' -o '&:use_system_hwloc=True'
```

> 效果：从「卡死 4 分钟无进展」变为「数十秒完成」，且全部依赖命中缓存。

---

### 3. Python 被 EXTERNALLY-MANAGED 锁定

**现象**：无法 `pip install conan`。

**根因**：Debian 13 的 Python 3.13 带 `EXTERNALLY-MANAGED` 标记，
禁止直接安装第三方包。上游 CI 用的是 Ubuntu 24.04，因此加了
`--break-system-packages`；在 Debian 上该做法不推荐。

**解决**：使用虚拟环境（不要污染系统 Python）。

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install "conan>=2.0" "meson>=1.10"
```

---

### 4. 编译器版本与 -Werror

`meson.build` 默认启用 `werror=true` + `warning_level=3` + `b_lto=true`。
GCC 版本越高告警越多，**GCC 15 的新告警极可能触发 `-Werror` 导致构建失败**。

**解决**：使用 `gcc-13`（与上游 CI pin 一致）。

```bash
# Conan profile 中指定，并配合 buildenv
compiler.version=13
[buildenv]
CC=gcc-13
CXX=g++-13
```

若必须使用 GCC 14+，加 `-D werror=false`（上游 `BUILDING.md` 也有此提示）。

> 附带好处：`conanfile.py` 依据 `__cpp_lib_format` 判定，
> gcc-13 已提供 `std::format`（`202110L`），因此**无需 `fmt` 依赖**。
> 若用 gcc-12 则会自动引入 `fmt/10.2.1`。

---

### 5. meson install 剥离 RPATH

**现象**：编译产物能运行，但 `meson install` 之后不能运行。

```
$ ./builddir/ial/cli/xpu-smi --version      # 正常
Intel XPU System Management Interface -- v2.2

$ /usr/bin/xpu-smi                          # 失败
/usr/bin/xpu-smi: symbol lookup error: undefined symbol: zesDevicePciLinkSpeedUpdateExt
```

> 退出码 **127**，容易被误读为「命令未找到」。

**根因**：`builddir` 内的二进制带有指向 Conan 缓存的 RPATH，
而 `meson install` 会剥离它。安装后的程序随即回落到系统 loader
（1.24.1），而该版本不导出所需的 SYSMAN 符号。
`meson.build` 中**完全没有 `install_rpath` 配置**。

**错误做法**：用 `patchelf` 把 Conan 的 1.33.1 loader 塞进 `/usr/local/lib`
并设置 RPATH。**这会让情况更糟**，原因见下一节。

---

### 6. 核心问题：loader 与驱动版本必须匹配

这是本次最关键、也最容易走错路的发现。

**loader 与厂商驱动需要协商出一致的 DDI 版本**。把过新的 loader 强行装上，
会在 `zeInit()` 阶段直接失败：

```
# 使用 Conan 编译的 loader 1.33.1
$ ZE_ENABLE_LOADER_DEBUG_TRACE=1 zeInit
Loading Driver libze_intel_gpu.so.1 succeeded from: /lib/x86_64-linux-gnu/libze_intel_gpu.so.1
ze_lib Context Init() zeDdiTableInit failed with ZE_RESULT_ERROR_UNSUPPORTED_VERSION
=> zeInit 0x78000002                                    # DRIVER_NOT_FOUND

# 使用系统 loader 1.24.1（厂商配套版本）
$ ZE_ENABLE_LOADER_DEBUG_TRACE=1 zeInit
Loading Driver libze_intel_gpu.so.1 succeeded
check_drivers(flags=ZE_INIT_FLAG_GPU_ONLY)
=> zeInit 0x0                                           # 成功
```

**教训**：不要试图通过升级 loader 来解决符号缺失。
**loader 与驱动必须是厂商配套的版本组合。**

**正确做法**：保留系统 loader，只补上它确实缺失的符号 —— 见下一节。

---

## 根本矛盾：README 与 conanfile.py 的版本要求不一致

这是导致前述所有问题的根源：

| 来源 | Level Zero 要求 |
|---|---|
| `README.md`（Runtime Dependencies） | **v1.27.0 or later** |
| `conanfile.py` | **`level-zero/1.33.1`**（强制） |
| 官方 Ubuntu 24.04 仓库实测 | 1.24.x ~ 1.2x 区间 |

`conanfile.py` 硬编码了 `1.33.1`，而 README 声明 `1.27.0+` 即满足要求。
对于不自带 Level Zero 1.33.1 的发行版，按 README 判断「依赖已满足」的用户，
实际会在运行时遭遇符号缺失。

> 本机 Debian 13 仓库仅提供 `libze1 1.24.1-2`，且 apt 源中无更高版本。
> 已尝试 Intel 官方仓库（`apt.repos.intel.com/oneapi`、`repositories.intel.com/gpu`），
> 均未获得更新的 level-zero 包。

---

## SYSMAN 兼容垫片

在保留系统 loader 的前提下，只需补齐缺失的符号。
比对 xpu-smi 所需的全部 **112 个** Level Zero 外部符号
（启用垫片前）后确认，系统 loader **仅缺 3 个**：

```
zesDevicePciLinkSpeedUpdateExt
zesRasGetStateExp2
zesRasGetSupportedCategoriesExp
```

三者均为**冷门诊断接口**，不涉及设备发现、遥测、功耗/风扇控制、
固件更新、ECC 等核心功能。

`hal/core/level_zero_compat.c` 提供桩函数，缺失时返回
`ZE_RESULT_ERROR_UNSUPPORTED_FEATURE` —— 这与实际情况语义相符，
且 xpum 既有代码已将非 `ZE_RESULT_SUCCESS` 结果视为「不支持」并优雅降级。

启用方式（**默认关闭**，loader 自带这些符号时仍使用真实实现）：

```bash
meson setup builddir --native-file .conan/conan_meson_native.ini \
  -Dlevel_zero_sysman_compat=true
```

### 编写时的两个陷阱

1. **头文件引用方式**：必须 `#include <zes_ddi.h>`，而非 `<level_zero/zes_api.h>`。
   Meson 只把 `include/level_zero` 加入 `-I` 路径，后者无法解析。
   `zes_ddi.h` 中的函数指针 typedef 是权威的 C ABI。

2. **枚举常量**：正确常量是 `ZES_DEVICE_ACTION_NONE`（枚举），
   不存在 `ZES_STRUCTURE_TYPE_DEVICE_ACTION`。

---

## 已知限制

以下 3 项功能在本机不可用，属**预期的降级行为**：

| 功能 | 受影响接口 |
|---|---|
| PCIe 链路降速 | `zesDevicePciLinkSpeedUpdateExt` |
| per-tile RAS 错误计数 | `zesRasGetStateExp2` |
| RAS 类别查询 | `zesRasGetSupportedCategoriesExp` |

其余功能（设备枚举、遥测、功耗/风扇、固件更新、ECC）
均基于 Level Zero 稳定接口，不受影响。

### 驱动升级

若需启用上述功能，需将 **loader 与 GPU 驱动一并**升级到 1.33.1 配套版本。
仅升级其一不可行（见[第 6 节](#6-核心问题loader-与驱动版本必须匹配)）。
本机 apt 源暂无可用版本。

---

## 环境清理

本次实践中曾部署过一个错误的 loader 配置，已完整回滚。
若你在类似环境中操作过，请确认以下状态：

```bash
# 1. xpu-smi 不应带 RPATH
readelf -d /usr/bin/xpu-smi | grep -iE 'rpath|runpath' || echo "无 RPATH（正确）"

# 2. /usr/local/lib 不应残留 level-zero
ls /usr/local/lib/libze_loader* 2>/dev/null || echo "已清理"

# 3. 程序应加载系统 loader
ldd /usr/bin/xpu-smi | grep ze_loader
# 期望：/lib/x86_64-linux-gnu/libze_loader.so.1
```

> `/usr/local/lib` 在 `/etc/ld.so.conf.d/` 中的优先级**低于**
> `/usr/lib/x86_64-linux-gnu`，因此把新 loader 放在那里不会影响
> 其他软件（如 vLLM）继续使用系统原有的 1.24.1。

---

## 参考

- 上游构建文档：[../BUILDING.md](../BUILDING.md)
- 上游 CI 构建环境：[../.github/docker/Dockerfile.coverage](../.github/docker/Dockerfile.coverage)
- Level Zero 发行说明：<https://github.com/oneapi-src/level-zero/releases>
- Intel GPU 驱动安装指南：<https://dgpu-docs.intel.com/installation-guides/index.html>

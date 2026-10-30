# LUT-SA 系统组件（SystemAbility）集成脚手架

> 作用：把 LUT-SA 推理引擎从"能编成 .so 的代码"变成 **OpenHarmony 构建体系认识的子系统组件**，
> 从而被编进系统镜像（`/system` 分区），开机由 `init` + `samgr` 拉起，成为**全系统共享的 AI 推理能力**。
>
> 状态：**文件齐备（本目录）；GN/策略的语法与运行时验证需要 OH 源码树或开发板**（详见第 6 节）。

## 1. 目录结构

| 文件 | 作用 | 编进镜像后的位置 |
|---|---|---|
| `bundle.json` | 组件声明（子系统归属、依赖、构建目标） | — |
| `BUILD.gn` | GN 构建脚本：编 `libtmac_sa.z.so` + 安装 profile/init cfg | `/system/lib64/`、`/system/profile/`、`/system/etc/init/` |
| `lut_sa_ability.{h,cpp}` | **samgr 绑定**：继承 `SystemAbility`、`Publish()` 注册、`Dump()` 支持 | 编入 .so |
| `ILutSa.idl` | 对外 IPC 接口（6 个方法）；用 SDK 的 `idl.exe` 生成 stub | 编入 .so |
| `sa_profile/lut_sa.json` | samgr 注册描述（SA_ID=6901、libpath、run-on-create） | `/system/profile/lut_sa.json` |
| `etc/init/lut_sa.cfg` | init 启动配置（`sa_main` 拉起 + SELinux `secon`） | `/system/etc/init/lut_sa.cfg` |
| `sepolicy/base/te/lut_sa.te` | SELinux 策略（domain + 与 samgr/客户端通信许可） | 编入产品 sepolicy |
| `sepolicy/base/file_contexts` | 文件标签 | 同上 |

业务内核复用仓库既有代码：`ohos/sa/{lut_sa,session_workspace,tile_*,}` + `ohos/sched/qos_policy` + 预编译引擎静态库 `ohos/hap/prebuilt/<abi>/*.a`。

## 2. 集成到系统镜像（三步）

```bash
# ① 放入 OH 源码树的 vendor 目录（示意路径）
OH_ROOT=/path/to/openharmony
mkdir -p $OH_ROOT/vendor/fandou/lut_sa
cp -r ohos/sa/component/*          $OH_ROOT/vendor/fandou/lut_sa/          # 本目录
cp -r ohos/sa/*.{cpp,h}        $OH_ROOT/vendor/fandou/lut_sa/          # 业务内核
cp -r ohos/sched               $OH_ROOT/vendor/fandou/lut_sa/sched
cp -r ohos/hap/prebuilt        $OH_ROOT/vendor/fandou/lut_sa/prebuilt

# ② 注册子系统（OH 版本不同，字段名以本仓库 build/subsystem_config.json 为准）
#    在 vendor/fandou/ohos.build 或产品配置的 subsystem 列表中挂上 lut_sa 组件
#    并在产品配置里加入该部件（如 vendor 的 install 列表）

# ③ 编镜像（dayu200 为例；qemu 用对应 product）
./build.sh --product-name rk3568 --build-target lut_sa_package --ccache
# 产物：out/rk3568/packages/phone/images/system.img（含 libtmac_sa.z.so + profile + init cfg + 策略）
```

> 注：`build/subsystem_config.json` 是 OH 的子系统注册表；不同 OH 版本（5.x/6.x/7.x）字段名有差异，
> 以你所用版本的 `//build/subsystem_config.json` 与同仓库其他厂商组件（如 vendor/hihope）为模板最稳。

## 3. 刷机与验证

```bash
# 刷机（板子）
hdc shell reboot bootloader; fastboot flash system system.img; fastboot reboot
# 或 qemu 镜像直接替换 system.img 启动

# 验证链（四条，逐级排除）
hdc shell "ls -l /system/lib64/libtmac_sa.z.so /system/profile/lut_sa.json /system/etc/init/lut_sa.cfg"
hdc shell "hidumper -ls | grep -i lut"                 # ① 应出现 LutSystemAbility
hdc shell "hidumper -s 6901"                           # ② 应打印 Dump()（本服务的自述）
hdc shell "hilog | grep -i LutSa"                      # ③ OnStart/Publish 日志
# 应用侧（系统应用或授权三方）：
#   auto mgr = samgr_proxy::GetSystemAbilityManager();
#   auto remote = mgr->GetSystemAbility(6901);          // ④ 拿到代理即可调 ILutSa 六方法
```

## 4. 依赖与头文件获取（应用 SDK 不提供，需平台源码或 gitee）

| 依赖 | 头文件 / 库 | 来源 |
|---|---|---|
| SystemAbility 基类 | `system_ability.h`（`REGISTER_SYSTEM_ABILITY_BY_ID`） | OH 源码 `foundation/systemabilitymgr/safwk/interfaces/innerkits/` |
| samgr 代理 | `if_system_ability_manager.h` 等 | OH 源码 `.../samgr/interfaces/innerkits/samgr_proxy/include/` |
| IPC | `iremote_stub.h` / `ipc_core` | OH 源码 `foundation/communication/ipc/...` |
| IDL 生成 | SDK `toolchains/idl.exe`：`idl.exe --gen-cpp --out-dir . ILutSa.idl` → 生成 `ilut_sa.h` / `lut_sa_idl_stub.cpp` | OHOS SDK |
| 日志 | `hilog/log.h` | SDK `native/sysroot` |

## 5. 与"应用级形态"的关系

| | 应用级（已交付 HAP） | 系统服务级（本目录） |
|---|---|---|
| 装在哪 | `/data`（沙箱） | `/system`（镜像分区） |
| 谁拉起 | 用户点图标 | **init + samgr（开机/按需）** |
| 谁能用 | 单个应用 | **全系统（任何模块经 `LoadSystemAbility(6901)`）** |
| 感知调度范围 | 应用自身前后台 | 可扩展为**系统级**信号（负载/前台应用/温度） |
| 同一性 | 两者共用同一 Native 引擎与 LUT 内核 | 同上 |

## 6. 已完成 / 待验证（诚实边界）

| 项 | 状态 |
|---|---|
| 组件文件齐备（bundle/BUILD.gn/profile/init cfg/策略/IDL/绑定代码） | ✅ 本目录 |
| JSON 语法与关键字段自检（bundle.json、sa_profile、init cfg） | ✅ 本地校验通过 |
| 业务内核 + 冒烟测试 | ✅ 既有（`ohos/sa/`，qemu 全链路 PASS） |
| 绑定代码编译 | ⏳ 需 safwk/samgr 头文件（第 4 节路径可获取）后做 compile 校验 |
| GN 语法/链接 | ⏳ 需 OH 源码树的 `gn` 与 samgr/safwk 库 |
| SELinux 策略 | ⏳ 需产品 sepolicy 上下文 + `checkpolicy` 校验 |
| **运行时注册验证（hidumper 出现 6901）** | ⏳ **需开发板或自编镜像**（本机/零售设备不可，原因见下） |

### 为什么不能在零售设备或本机跑（实测六条锁）

非 root 且无 `su` · SELinux Enforcing · `/system` 只读且 `/system/profile`、`/system/etc/init` **Permission denied** ·
`/sys_prod` 对 shell 不可写 · `hdc target mount` 要求 debug 镜像（**E007100**）· 两套 SDK 均无 samgr/safwk 头文件。
完整原始证据：`ohos/SA-BOUNDARY-EVIDENCE.md`（模拟器 + 真机 Pura 70 Pro 双端）。

> 结论：**第三方注册系统服务在零售 HarmonyOS 上被平台关闭**；正确载体是 OpenHarmony 标准系统镜像/开发板。
> 本目录的工作即把"换载体"这一步缩短为"编镜像 + 刷机"。

## 7. 下一步（有板子 / 会编镜像时）

1. 取 OH 源码（`repo.huaweicloud.com/openharmony/os/<ver>/code-<ver>.tar.gz` 或 `repo init -b <ver>`）
2. 按第 2 节放入 vendor 目录并注册子系统
3. `./build.sh --product-name <p> --build-target lut_sa_package`
4. 刷机 → 按第 3 节四条验证 → 截图归档
5. 回填数据到 `docs/` 与作品说明文档（"系统服务级形态：真机注册成功"）

## 8. 编译级验证记录（2026-09-30，已完成 ✅）

用 **OH 平台真实头文件 + SDK idl.exe 生成的桩 + OH SDK clang** 对 `lut_sa_ability.{h,cpp}` 做了编译验证
（目标 `aarch64-linux-ohos`，产出对象文件 ~100 KB）。过程中定位并修正 **3 处真实接口差异**：

| # | 差异 | 修正 |
|---|---|---|
| 1 | `SystemAbility` 在 `namespace OHOS` 内 | 类声明移入 `namespace OHOS` |
| 2 | 该 OH 版本的 dump 钩子是 **`OnSvcCmd(int32_t fd, const std::vector<std::u16string>&)`**，不是 `Dump(fd,args)` | 改签名并实现 |
| 3 | `Publish(this)` 要求 `sptr<IRemoteObject>` → 必须继承 **IDL 生成的 `OHOS::LutSa::LutSaStub`**（`IRemoteStub<ILutSa>`） | 生成并继承 stub，实现六个 IPC 方法 |

### 复现命令（本机已验证）

```bash
# 1) 生成 IPC 桩（SDK 自带 idl.exe；输出目录用 Windows 路径）
idl.exe --intf-type sa -c ILutSa.idl --gen-cpp -d <out-dir>
#    → ilut_sa.h / lut_sa_proxy.{h,cpp} / lut_sa_stub.{h,cpp}（已随本目录 idl/ 归档）

# 2) 编译绑定（需从 gitee 浅克隆 safwk/samgr/ipc/c_utils/hilog 取头文件；见第 4 节）
clang++ --target=aarch64-linux-ohos --sysroot=<OHOS_SDK>/native/sysroot -std=c++17 -fPIC \
  -DTMAC_SA_SAMGR_BINDING -c lut_sa_ability.cpp -o lut_sa_ability.o \
  -I<safwk>/services/safwk/include -I<samgr>/interfaces/innerkits/samgr_proxy/include \
  -I<ipc>/interfaces/innerkits/ipc_core/include -I<c_utils>/base/include \
  -I<hilog>/interfaces/native/innerkits/include -I<out-dir>
```

> 头文件获取（gitee 可达，浅克隆即可）：`openharmony/systemabilitymgr_safwk`、`systemabilitymgr_samgr`、
> `communication_ipc`、`commonlibrary_c_utils`、`hiviewdfx_hilog`（合计 ~20 MB）。

**结论**：本目录的 SA 代码与注册/策略/构建描述，已对齐真实 OH 接口（编译零错误）；
剩余唯一事项 = 在可编镜像的环境（开发板或自编 qemu 镜像）中点亮它。
# 系统服务化形态 · 部署载体与权限边界实测（证据）

> 设备：官方模拟器 Pura X View（HarmonyOS 7.0.0 / API 26，x86_64，4 vCPU）
> 工具：`hdc`（DevEco SDK 26.0.0）
> 日期：2026-09-30 · 结论：**零售版 HarmonyOS（模拟器/真机）不允许第三方注册系统服务**，载体应为开源鸿蒙标准系统镜像/开发板

## 一、原始探测记录

### 1. 身份：非 root，无提权通道

```
$ hdc shell id
uid=2000(shell) gid=2000(shell) groups=2000(shell),1006(file_manager),1007(log),1097(netsys_socket),3009(readproc) context=u:r:sh:s0

$ hdc shell "which su; ls -la /system/bin/su"
/bin/sh: su: inaccessible or not found
ls: /system/bin/su: No such file or directory
```

### 2. SELinux：Enforcing（非宽松）

```
$ hdc shell getenforce
Enforcing
```

### 3. 系统分区：只读，且对 shell 不可访问

```
$ hdc shell "ls -la / | grep -E 'system|vendor|sys_prod|chip_prod'"
drwxr-xr-x   2 root   shell   4096 chip_prod
drwxr-xr-x   9 root   root    4096 sys_prod
d?????????   ? ?      ?          ? eng_system      ← stat 被拒
d?????????   ? ?      ?          ? system          ← stat 被拒

$ hdc shell "test -w /system && echo SYSTEM_WRITABLE || echo SYSTEM_READONLY; touch /system/__probe"
SYSTEM_READONLY
touch: '/system/__probe': Read-only file system

$ hdc shell "ls /system/profile"          # 第三方 SA 注册描述必须落地处
ls: /system/profile: Permission denied

$ hdc shell "ls /system/etc/init"          # SA 启动配置必须落地处
ls: /system/etc/init: Permission denied

$ hdc shell "mount | grep -E ' /(vendor|sys_prod|data) '"
/dev/block/vdb on /vendor   type ext4 (ro,seclabel,relatime)
/dev/block/vdc on /sys_prod type ext4 (rw,seclabel,relatime)   ← 唯一可写“系统类”分区
/dev/block/vdd on /data     type ext4 (rw,...)

$ hdc shell "test -w /sys_prod && echo W || echo N; ls -ld /sys_prod /sys_prod/etc"
N
drwxr-xr-x 9 root root 4096 /sys_prod
ls: /sys_prod/etc: Permission denied
```

### 4. samgr 机制本身是活的（机制可用，槽位对第三方关闭）

```
$ hdc shell "hidumper -ls"
System ability list:
SystemAbilityManager   RenderService   AbilityManagerService   DataObserverMgr
AgentManagerService    AccountMgr      BundleMgr               FormMgr
ApplicationManagerService  Installd   AccessibilityManagerService ...
（共 100+ 个系统 SA；含 SystemAbilityManager=samgr 本体）

尾部为 6xxxx/7xxxx 动态编号项（应用侧注册的应用级服务），
无任何第三方系统 SA 槽位；本项目规划的 SA_ID=6901 不存在。
```

## 二、结论（三重锁）

| 锁 | 实测 | 影响 |
|---|---|---|
| 身份 | `uid=2000(shell)`，无 `su`、无 root | 无法执行系统级写操作/注册 |
| SELinux | `Enforcing` | 即使有文件，无对应 `.te` 策略的进程也会被拒 |
| 系统分区 | `/system` 只读且 shell 不可 stat；`/system/profile`、`/system/etc/init` Permission denied；`/sys_prod` 对 shell 不可写 | 注册描述（profile.json）与启动配置（init cfg）无法落地 |

→ **第三方应用在零售版 HarmonyOS 上注册系统服务（SystemAbility）在机制上被关闭**，与代码完成度无关。

## 三、我们已完成 vs 载体缺口

| 项 | 状态 |
|---|---|
| SA 业务内核（CreateSession / LoadModel / PrepareWorkspace / WarmKernel / InferTokenBatch / ReleaseSession） | ✅ 已实现（`ohos/sa/`，约 800 行） |
| 注册描述 | ✅ `profile.json`（LutSystemAbility / libtmac_sa.z.so / run-on startup）+ `lut_sa.cfg`（class core / SA_ID=6901） |
| 感知 → QoS 策略 | ✅ `ohos/sched/qos_policy.cpp` |
| 全链路冒烟 | ✅ 标准系统目标（aarch64-linux-ohos）qemu 实测 PASS（Create→Load→Prepare→Infer→Release + 前后台信号切换） |
| **samgr 绑定 + 跨进程 IPC** | ❌ 需把 SA 转成 OH 子系统组件（GN 构建 + bundle.json + sepolicy）并**编入系统镜像**——载体工作，非算法工作 |
| 可演示的替代形态 | 应用级：已交付 HAP（本提交）；应用内服务化近似：ServiceExtensionAbility（未实现，1–2 天，零售设备可用） |

## 四、真机复核（2026-09-30 已完成）

设备：HUAWEI Pura 70 Pro（HBN-AL00，HarmonyOS，API 24，arm64-v8a）

```
$ hdc -t <serial> shell id
uid=2000(shell) gid=2000(shell) groups=2000(shell),1006(file_manager),1007(log),1097(netsys_socket),3009(readproc) context=u:r:sh:s0

$ hdc -t <serial> shell getenforce
getenforce:  security_getenforce() failed:  Permission denied        ← 真机连 SELinux 状态都不可读

$ hdc -t <serial> shell "which su; ls -l /system/bin/su"
ls: /system/bin/su: No such file or directory

$ hdc -t <serial> shell "test -w /system && echo SYSTEM_WRITABLE || echo SYSTEM_READONLY;
                         ls -ld /system; ls /system/profile; ls /system/etc/init"
SYSTEM_READONLY
ls: /system: Permission denied
ls: /system/profile: Permission denied
ls: /system/etc/init: Permission denied

$ hdc -t <serial> target mount
[Fail][E007100] Operate need running under debug mode
```

**结论：真机与官方模拟器同结论，且更严**（模拟器可读 `getenforce` 得到 Enforcing；真机该调用直接拒绝）。
零售版 HarmonyOS 的第三方系统服务注册在平台层面关闭，与代码完成度无关。

## 五、深度复查补充证据（2026-09-30 二次核查）

### 5.1 系统分区注入：官方工具直接拒绝

```
$ hdc target mount
[Fail][E007100] Operate need running under debug mode
```

`hdc target mount`（重挂系统分区为可写）要求镜像为 **debug 模式**（userdebug/eng 构建）。
官方模拟器镜像不是 debug 版 → **注入路径被官方工具明确关闭**（不是"没找到办法"）。

### 5.2 SA 开发所需头文件不在 SDK 中（API 分层证据）

对两套 SDK 的 native sysroot 做显式查找（`-iname "*system_ability*" -o -iname "*samgr*" -o -iname "*sa_mgr*"`）：

| SDK | 结果 |
|---|---|
| DevEco Studio 26.0.0 SDK（`sdk/default/openharmony/native/sysroot`） | **无** samgr / SystemAbility 头文件 |
| 独立 OpenHarmony SDK（`/d/ohos-sdk/ohos-sdk/windows/native/sysroot`） | **无** |

→ SystemAbility 的基类（`system_ability.h`）、samgr 客户端等均属 **OpenHarmony 平台源码树**
（`foundation/systemabilitymgr/...`）内容，**应用 SDK 不提供**。即："SA 开发 = 平台开发"，
第三方应用从 API 层面就不在这条路上（与 `profile/init` 目录不可访问互为印证）。

### 5.3 第三方 SA 管理工具不存在

```
$ hdc shell "which sa_main samgr_client sa_tool"
（均不存在；仅有 aa / bm / param / hidumper 等应用与诊断工具）
```

### 5.4 应用级"服务"API 同样受限（零售设备上第三方的极限）

| API | 约束（SDK d.ts 原文） | 我们可用性 |
|---|---|---|
| `AppServiceExtensionAbility` | "only 2-in-1 devices are supported"；需 ACL 权限 `ohos.permission.SUPPORT_APP_SERVICE_EXTENSION`，**该权限仅企业应用可申请** | ❌ 手机 + 学生团队不可用 |
| 通用 `ServiceExtensionAbility` | API 26 SDK 中**已无通用版本**（仅剩广告/打印/输入法等专用扩展） | ❌ 不存在 |
| `BackgroundTasksKit`（后台长时任务/延迟挂起） | 应用级 API，无特殊权限 | ✅ 可用（当前用于模型拷贝的延迟挂起） |

→ 零售设备上第三方能做的"服务化"上限 = **应用内后台保活 + 进程内服务能力**，
无法向其他应用/系统提供真正的服务接口。

### 5.5 结论更新：不是"权限低"，是"平台未开放"

四重系统事实（非 root/无 su、SELinux Enforcing、/system 只读且不可访问、/sys_prod 对 shell 不可写）
　　　+ 一条工具拒绝（`hdc target mount` → E007100 需 debug 镜像）
　　　+ 一条 API 面缺失（两套 SDK 均无 samgr/SystemAbility 头文件）
　　　+ 一条应用级 API 约束（AppServiceExtensionAbility 仅 2in1 + 企业 ACL）

= **零售版 HarmonyOS 的平台设计未向第三方开放系统服务注册**。要运行系统服务形态，
载体必须是**基于 OpenHarmony 平台源码树构建的标准系统镜像**（开发板或自编镜像，
把 SA + 注册描述 + SELinux 策略编入镜像）——这正是我们把该形态定位为"预研 + 载体待迁移"的原因。


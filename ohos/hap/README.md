# LUT-SA · HAP 演示工程（T-MAC on HarmonyOS）

> 此文件保留源码包原有的工程操作记录，部分命令和测试状态写于 2026-09-27，可能早于最终交付。项目定位、最终版验证口径及已知的模型导入权限问题，请先看仓库根 `README.md`、`docs/verification/` 与 `docs/KNOWN_ISSUES.md`。正式发布前需重新验证 SDK、签名、模型格式与设备兼容性。

把 T-MAC 的低比特 LLM 推理装进一个正经的鸿蒙应用：**ArkTS 界面 → NAPI → 静态 llama.cpp + LUT 内核**，
真机/模拟器一键运行（DevEco 点 Run 即可）。

> SystemAbility 版本还在 `ohos/sa/`（需要开发板）；这里是"应用形态"。

## 当前状态（2026-09-27）

| 项目 | 状态 |
|---|---|
| HAP 构建（双 ABI，命令行 hvigor） | ✅ `BUILD SUCCESSFUL`，产物 `entry/build/default/outputs/default/entry-default-unsigned.hap`（6.6 MB） |
| `libtmac_hap.so`（x86_64 2.2 MB / arm64 1.8 MB） | ✅ 内含静态 llama.cpp + T-MAC 内核（库内嵌 kcfg） |
| 内核自测 / 基准（`selfTest` / `bench`） | ✅ 代码就绪（沿用已验证的 m128-k3200 形状） |
| LLM 推理（`loadModelAsync` / `generateAsync`） | ✅ 代码就绪；待设备上跑（模拟器原先那份 `model.gguf` 可直接用） |

## 你需要做什么（装 DevEco → 跑模拟器）

1. **安装 DevEco Studio**（Windows 版，developer.huawei.com 下载），安装时让它装好默认 SDK（5.0 / API 12）。
2. **登录华为账号**（免费注册）。模拟器需要账号授权。
3. **打开工程**：`File → Open` 选本目录（`ohos/hap`），等右上角 Sync 完成。
4. **配置签名**：`File → Project Structure → Signing Configs → 勾 Automatically generate signature`（登录后自动完成）。
5. **创建模拟器**：`Device Manager → Local Emulator → New Emulator → Phone，镜像选 x86_64`，下载并启动。
6. **推模型**（966 MB，不进 HAP）：
   ```bash
   hdc file send bitnet-3b-tmac-ags64.gguf /data/local/tmp/llm-tmac/model.gguf
   ```
   （模拟器上一般已有这份；没有就用上面命令推。真机请用 `bitnet-3b-tmac-arm64.gguf`）
7. **运行**：目标选模拟器 → 点 **▶ Run 'entry'**。
8. **推模型**（见下一节，966 MB **必须走沙箱覆盖法**，`/data/local/tmp` 读不了）。
9. **预期结果**（2026-09-27 实测通过）：
   - 「内核自测」→ `✅ 内核自测通过 · PASS: LUT kernel ran, zero-in => zero-out`
   - 「内核基准」→ 200 次内核调用总耗时 / 平均耗时
   - 「加载模型」→ `✅ 模型已加载`，`load = 4956 ms`（含 288 张量内核变换）
   - 「生成」→ `the city of Paris. The city is located in France. Paris was a Roman`
     + `eval ≈10.5 tok/s`（同一设备三次独立测量 10.38 / 10.51 / 10.52，误差 <1.5%；截图：`ohos/screenshots/emulator/hap-demo-screenshot.png`）

## 模型怎么进 App（两种方式）

### 方式 1（产品级，任何设备都能用 ✅ 推荐）：App 内「📂 选择模型文件」

点「📂 选择模型文件」→ 系统文件选择器（可按 `.gguf` 过滤）→ 选中后 App **自己把模型拷进沙箱**并自动加载。
不需要 hdc/shell 权限，不受设备沙箱策略影响——真机同样可用。
（实测：选择器可正常拉起并带隐私提示；取消分支正确。模拟器用户存储里没有文件所以选不了实际文件。）

### 方式 2（开发捷径，仅当设备允许 shell 覆盖 App 文件）：hdc 覆盖沙箱占位

**HarmonyOS 7 实测的限制**：App 读不了 `/data/local/tmp`（SELinux 拦截）；shell 也不能在 App 沙箱**新建**文件。
可行通道：App 启动时 `chmod(沙箱,0777)` 并创建一个 **0666 的 `model.gguf` 占位**，shell 就能**覆盖**它：

```bash
# App 内看到的路径：/data/storage/el2/base/haps/entry/files/…
# shell 侧真实路径： /data/app/el2/100/base/com.fandou.lutsa/haps/entry/files/…
hdc file send bitnet-3b-tmac-ags64.gguf \
  /data/app/el2/100/base/com.fandou.lutsa/haps/entry/files/model.gguf
```
（App 每次启动会把已存在的 `model.gguf` 重新 chmod 到 0666，保证这条通道长期可用。）

> 备选：App 通过 HTTP 从主机下载模型到沙箱（模拟器网关 `10.0.2.2` 实测可达），需加 `ohos.permission.INTERNET`。

## 命令行构建（可选，只产未签名 HAP）

```powershell
$env:DEVECO_SDK_HOME = "D:\DevEco Studio\sdk"
& "D:\DevEco Studio\tools\node\node.exe" "D:\DevEco Studio\tools\hvigor\bin\hvigorw.js" `
    assembleHap --mode module -p product=default --no-daemon
```
安装到设备需签名：IDE 的 Run 会做自动签名；命令行安装要自行用 `hap-sign-tool.jar`。

## 工程结构

```
hap/
├── AppScope/                     # 应用级配置（包名 com.fandou.lutsa）
├── prebuilt/
│   ├── x86_64/{libllama.a,libggml.a}     # AVX2 内核（staging-x64；模拟器无 FMA，勿加 -mfma）
│   └── arm64-v8a/{libllama.a,libggml.a}  # NEON fp16 内核（staging-arm64，armv8.2a+fp16）
└── entry/
    └── src/main/
        ├── ets/
        │   ├── entryability/EntryAbility.ets
        │   └── pages/Index.ets                 # 界面：模型路径/prompt/加载/生成/自测/基准
        ├── cpp/
        │   ├── CMakeLists.txt                  # 按 ABI 链接 prebuilt 静态库（T-MAC 内核在 libggml.a 内）
        │   ├── napi_init.cpp                   # NAPI 桥（同步 + 异步两套入口）
        │   └── types/libtmac_hap/index.d.ts    # ArkTS 类型声明
        └── resources/
```

## 接口

| NAPI | 说明 | 返回 |
|---|---|---|
| `nativeVersion()` | 架构 / tfloat 宽度 / 链接信息 | 字符串 |
| `selfTest()` | 零输入冒烟：跑一次内核，校验零入零出 | 含 `PASS`/`FAIL` |
| `bench(steps)` | 内核重复调用计时（1–100000 次） | 形状 / 总耗时 / 平均 |
| `loadModel(path, filesDir, threads?, nCtx?, useMmap?)` | 同步加载（会阻塞） | 状态字符串 |
| `loadModelAsync(...)` | **异步（Promise）**，UI 用这个 | `Promise<string>` |
| `generate(prompt, nPredict?, temp?, topK?)` | 同步生成 | 文本 + tok/s |
| `generateAsync(...)` | **异步（Promise）**，UI 用这个 | `Promise<string>` |
| `release()` | 释放模型/上下文 | 状态字符串 |

```typescript
import tmac from 'libtmac_hap.so';
const ok = await tmac.loadModelAsync('/data/local/tmp/llm-tmac/model.gguf', ctx.filesDir, 4, 512, false);
const out = await tmac.generateAsync('The capital of France is', 16, 0.8, 40);
```

## 设计要点

- **kcfg 自包含**：内核所需的 `kcfg.ini` 以字符串编进 `libtmac_hap.so`，首次加载时写入应用沙箱并
  `setenv("TMAC_KCFG_FILE", …)`（wrapper 优先读环境变量）→ 不依赖主机构建路径。
- **异步**：加载/生成耗时较长，`*Async` 在 NAPI 工作线程执行，避免卡死 UI（系统对无响应应用有看门狗）。
- **采样器自带**：本 fork 的采样器在 `common/`（未链接进 HAP），NAPI 内实现 top-k + temperature
  （`std::mt19937(42)`，确定性）。
- **模型必须与 ABI 匹配**：

| ABI | 模型 | 说明 |
|---|---|---|
| x86_64（模拟器） | `bitnet-3b-tmac-ags64.gguf` | kcfg 与 `staging-x64` 一致 |
| arm64-v8a（真机） | `bitnet-3b-tmac-arm64.gguf` | kcfg 与 `staging-arm64` 一致（m6400 形状 bm 不同，需按 arm64 重转） |

## 故障排查

| 现象 | 处理 |
|---|---|
| 安装失败 `signature verification failed` | 用 DevEco Run（自动签名），或自行签名后 `hdc install` |
| `FAIL: llama_load_model_from_file` | 路径不对/模型与 ABI 不匹配；先 `hdc shell ls -l <path>` |
| 加载成功但进程被杀（OOM） | UI 打开 `mmap` 开关（权重走文件映射，匿名内存大降） |
| 生成很慢 | 模拟器 x86_64（无 FMA）+ 虚拟化本就慢；真机 arm64+NEON 才是目标环境 |
| `libtmac_hap.so` 加载失败 | 确认设备 ABI（模拟器=x86_64，真机=arm64-v8a）在 `abiFilters` 里 |
| Sync 报 SDK 版本不匹配 | 改 `build-profile.json5` 的 `compatibleSdkVersion` |
| native 编译找不到 `deploy/tuned/...` | 本工程需放在完整仓库里（`ohos/hap` 不能单独拿出仓库） |

## 注意

- 模拟器（x86_64/AVX2，**无 FMA**）与真机（arm64/NEON fp16）是**两套内核代码路径**，报告分开表述。
- 模拟器性能数字只能演示流程；真机数据用 `ohos/scripts/deploy_arm64_device.ps1` 或本 App 在真机跑。

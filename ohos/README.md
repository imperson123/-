# ohos/ · LUT-SA OpenHarmony 封装层

> 竞赛层代码，不污染上游 `python/` / 根 `CMakeLists.txt`。  
> 规格：`docs/output/prd/prd.md` · 规划：`docs/output/report/ohos-port-feasibility.md`

## 目录

| 路径 | 阶段 | 说明 |
|------|------|------|
| `hello/` | P0 | aarch64-ohos hello world |
| `shim/dmlc/logging.h` | P1 | 注入 dmlc，不改 wrapper |
| `CMakeLists.txt` | P1 | `libtmac_kernels.a` + selftest + `libtmac_sa.so` |
| `selftest/` | P1 | `qgemm_lut_int8(128,3200,1,2)` NMSE |
| `sa/` | P3 | SystemAbility 入口 + session workspace + tile pool + `sa_smoke` |
| `sched/` | P4 | QoS / 感知信号（默认进程内；`-DTMAC_USE_OH_QOS` 接官方 API） |
| `napi/` | P3 辅轨 | C ABI：`tmac_napi_create_session` 等 |
| `scripts/` | — | `env.ps1` / `build_*.ps1` / `push_hello.ps1` |

## 本机构建

```powershell
. .\ohos\scripts\env.ps1
powershell -File .\ohos\scripts\build_hello.ps1
powershell -File .\ohos\scripts\build_kernels.ps1
```

默认 `OHOS_SDK_NATIVE`：`D:\dev_software\DevEco Studio\sdk\default\openharmony\native`

## 板上（需 hdc 设备）

```powershell
powershell -File .\ohos\scripts\push_hello.ps1
hdc file send .\ohos\build\tmac_selftest /data/local/tmp/tmac_selftest
hdc shell chmod 755 /data/local/tmp/tmac_selftest
hdc shell /data/local/tmp/tmac_selftest
```

## llama.cpp (P2)

```powershell
# 需已 submodule init 3rdparty/llama.cpp，且 install/ 有 T-MAC 头与 kcfg
$env:OHOS_SDK_NATIVE = "D:\dev_software\DevEco Studio\sdk\default\openharmony\native"
python tools\run_pipeline.py -d ohos -u -m hf-bitnet-3b --ohos-ndk $env:OHOS_SDK_NATIVE -s 4,5
```

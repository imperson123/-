# P2 status · llama.cpp on OHOS

Date: 2026-07-26

## Done (host)

| 项 | 状态 |
|----|------|
| `3rdparty/llama.cpp` submodule | `eb07ecf` |
| `platform.py` / `run_pipeline.py` ohos | OK |
| `GGML_TMAC_TVM_THREADPOOL` | 默认 OFF（直连） |
| Staging | `ohos/scripts/stage_tmac_prefix.ps1` |
| **llama-cli / llama-bench 交叉编译** | **成功** → `3rdparty/llama.cpp/build-ohos-check/bin/`（aarch64） |
| dmlc shim | 补 `LOG_WARNING`（ggml-tmac 需要） |

## Board pending

推送并跑通（需设备 + gguf）：

```powershell
. .\ohos\scripts\env.ps1
hdc file send .\3rdparty\llama.cpp\build-ohos-check\bin /data/local/tmp/bin
hdc file send .\ohos\staging\t-mac\lib\kcfg.ini /data/local/tmp/kcfg.ini
# hdc file send <model.gguf> /data/local/tmp/model.gguf
hdc shell "chmod -R 755 /data/local/tmp/bin; TMAC_KCFG_FILE=/data/local/tmp/kcfg.ini /data/local/tmp/bin/llama-cli -m /data/local/tmp/model.gguf -n 128 -t 4 -ngl 0 -p 'hello'"
```

对照：`GGML_TMAC=OFF` 重配同目录另编一份，或 pipeline `--disable-t-mac`。

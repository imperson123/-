# 源码迁移记录

- 目标仓库：`imperson123/-`，基点 `main@d47c5c00be658d7341624051030821a290a611f5`（2025-12-16）。工作分支 `openatom-2026-linglong` 沿用该提交的历史，不改写旧提交。
- 源码来源：用户提供的 `附加材料/附-源代码包+翻斗花园.zip`，SHA-256 `1A26DB077E0068E0E057895775425FCA69DF7C273ABF9A3F87BEC2C4CEC11254`，从其中已解压的 `LUT-SA-源码-翻斗花园/fandou-t-mac/` 复制。外层交付包中的 `05-源代码/LUT-SA-源码-翻斗花园.zip` 与此 ZIP 的 SHA-256 相同。
- 源码包说明声明主仓库坐标为 `7fbe456`、内嵌 llama.cpp fork 坐标为 `181ad23a`；压缩包不含两者的 `.git` 对象，这两个坐标是包内声明，迁移时未独立核实。
- 旧树的 `HarmonyOS-Linglong-Kernel/`、`HarmonyOS-intelligent-operation/`、`Harmonyos-Application/` 是另一套 MLLM/MindSpore Lite/Web 方案，且含大量 `node_modules`。本迁移在同一 Git 历史上删除旧树，使用源码包的原生 HarmonyOS/T-MAC 工程；没有把两套不兼容应用叠加。
- 解压目录共 1,296 个文件，最终迁入 1,189 个源文件及 5 份新增整理文档/验证记录，工作树合计约 25.9 MB。未迁入模型 GGUF 测试样本、APK/HAP、ZIP/PDF/DOCX、演示 GIF、竞赛提交包、截图、备份、内部工作笔记、含个人路径的本地调试/签名/打包脚本，以及过时的上游支持模板与 CI 工作流；原始材料保持不变。
- `3rdparty/llama.cpp/` 作为普通源码目录提交。原源码包的 `.gitmodules` 指向可选或无法由包内 gitlink 重建的组件，与内嵌源码状态不符，故本次未复制 `.gitmodules`。若后续需要 TVM、ExecuTorch 或 Kompute，应单独确认版本与许可证后恢复依赖声明。
- 源码包中的旧 README 和部分历史技术文档是不同阶段的记录，可能含过时结论。仓库根 README 以当前代码与 2026-09-30 的验证材料为准；本次没有改动推理算法或模型格式。

原始 ZIP、HAP、演示视频和全部证明材料仍在用户提供的目录中，本仓不包含其副本。

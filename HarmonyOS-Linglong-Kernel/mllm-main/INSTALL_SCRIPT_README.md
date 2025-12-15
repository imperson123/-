# MLLM 安装脚本使用说明


脚本说明

1. `install_mllm_ubuntu.sh` 


使用方法：

```bash
# 1. 添加执行权限
chmod +x install_mllm_ubuntu.sh

# 2. 运行脚本
./install_mllm_ubuntu.sh

# 3. 或者使用选项
./install_mllm_ubuntu.sh --skip-deps        # 跳过系统依赖安装
./install_mllm_ubuntu.sh --skip-clone       # 跳过项目克隆
./install_mllm_ubuntu.sh --skip-build        # 跳过项目构建
./install_mllm_ubuntu.sh --skip-pymllm       # 跳过 Python 包安装
./install_mllm_ubuntu.sh --non-interactive   # 非交互模式
```


使用示例

示例 1: 首次完整安装

```bash
# 下载脚本（如果还没有项目）
wget https://raw.githubusercontent.com/UbiquitousLearning/mllm/main/install_mllm_ubuntu.sh
chmod +x install_mllm_ubuntu.sh
./install_mllm_ubuntu.sh
```
示例 2: 项目已存在，只构建

```bash
cd mllm
chmod +x install_mllm_ubuntu.sh
./install_mllm_ubuntu.sh --skip-deps --skip-clone
```
示例 3: 只安装依赖，不构建

```bash
chmod +x install_mllm_ubuntu.sh
./install_mllm_ubuntu.sh --skip-clone --skip-build --skip-pymllm
```
示例 4: 快速安装（非交互）

```bash
chmod +x install_mllm_quick.sh
./install_mllm_quick.sh
```



## 脚本维护

如果您发现脚本有问题或想要改进，欢迎提交 Issue 或 Pull Request！

- 脚本在 Ubuntu 20.04+ 上测试
- 需要 sudo 权限安装系统包
- 需要稳定的网络连接下载依赖
- 建议至少有 10GB 可用磁盘空间



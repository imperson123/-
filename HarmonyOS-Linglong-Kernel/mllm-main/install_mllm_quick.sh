#!/bin/bash

###############################################################################
# MLLM 项目 Ubuntu 快速安装脚本（简化版）
# 
# 此脚本执行最基本的安装步骤，适合快速安装
#
# 使用方法:
#   chmod +x install_mllm_quick.sh
#   ./install_mllm_quick.sh
###############################################################################

set -e

echo "=========================================="
echo "  MLLM 项目快速安装脚本"
echo "=========================================="
echo ""

# 更新系统并安装依赖
echo "[1/6] 更新系统并安装依赖..."
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    python3-venv \
    python3-dev \
    pkg-config \
    libomp-dev \
    wget \
    curl

# 升级 pip
echo "[2/6] 升级 pip..."
python3 -m pip install --upgrade pip --break-system-packages 2>/dev/null || \
python3 -m pip install --upgrade pip --user 2>/dev/null || \
python3 -m pip install --upgrade pip 2>/dev/null || true

# 检查项目目录（不需要克隆，因为项目已存在）
if [ -f "CMakeLists.txt" ]; then
    echo "[3/6] 已在项目根目录中"
elif [ -d "mllm" ]; then
    echo "[3/6] 进入 mllm 目录"
    cd mllm
else
    echo "[3/6] 未找到项目，尝试克隆..."
    git clone https://github.com/UbiquitousLearning/mllm.git
    cd mllm
fi

# 确认在正确的目录
if [ ! -f "CMakeLists.txt" ]; then
    echo "错误: 未找到 CMakeLists.txt，请确保在项目根目录中运行此脚本"
    echo "当前目录: $(pwd)"
    exit 1
fi

# 初始化子模块
echo "[4/6] 初始化子模块..."
git submodule update --init --recursive

# 安装 Python 依赖（处理 externally-managed-environment 错误）
echo "[5/6] 安装 Python 依赖..."
if python3 -m pip install --break-system-packages -r requirements.txt 2>/dev/null; then
    echo "使用 --break-system-packages 安装成功"
elif python3 -m pip install --user -r requirements.txt 2>/dev/null; then
    echo "使用 --user 安装成功"
else
    echo "尝试使用 sudo 全局安装..."
    sudo python3 -m pip install -r requirements.txt
fi

# 构建项目
echo "[6/6] 构建项目（这可能需要 30-60 分钟）..."
python3 task.py tasks/build_x86.yaml

# 安装 pymllm
echo "安装 pymllm..."
if [ -f "scripts/install_pymllm.sh" ]; then
    # 修改安装脚本以处理 externally-managed-environment
    bash ./scripts/install_pymllm.sh || {
        echo "安装脚本失败，尝试手动安装..."
        python3 -m pip wheel -v -w dist .
        if python3 -m pip install --break-system-packages dist/*.whl --force-reinstall 2>/dev/null; then
            echo "使用 --break-system-packages 安装成功"
        elif python3 -m pip install --user dist/*.whl --force-reinstall 2>/dev/null; then
            echo "使用 --user 安装成功"
        else
            sudo python3 -m pip install dist/*.whl --force-reinstall
        fi
    }
else
    python3 -m pip wheel -v -w dist .
    if python3 -m pip install --break-system-packages dist/*.whl --force-reinstall 2>/dev/null; then
        echo "使用 --break-system-packages 安装成功"
    elif python3 -m pip install --user dist/*.whl --force-reinstall 2>/dev/null; then
        echo "使用 --user 安装成功"
    else
        sudo python3 -m pip install dist/*.whl --force-reinstall
    fi
fi

echo ""
echo "=========================================="
echo "  安装完成！"
echo "=========================================="
echo ""
echo "验证安装："
echo "  export PATH=\$PATH:~/.local/bin"
echo "  mllm-convertor --help"
echo "  python3 -c 'import pymllm; print(\"Success!\")'"
echo ""


#!/bin/bash
# 安装 pymllm Python 包
# 处理 externally-managed-environment 错误（Ubuntu 23.04+）

set -e

echo "构建 wheel 包..."
python3 -m pip wheel -v -w dist . || {
    echo "错误: 构建 wheel 包失败"
    exit 1
}

echo "安装 wheel 包..."
# 尝试多种安装方法以处理 externally-managed-environment 错误
if python3 -m pip install --break-system-packages dist/*.whl --force-reinstall 2>/dev/null; then
    echo "使用 --break-system-packages 安装成功"
elif python3 -m pip install --user dist/*.whl --force-reinstall 2>/dev/null; then
    echo "使用 --user 安装成功"
else
    echo "尝试使用 sudo 全局安装..."
    sudo python3 -m pip install dist/*.whl --force-reinstall || {
        echo "错误: 安装失败"
        echo "请手动运行: python3 -m pip install --break-system-packages dist/*.whl --force-reinstall"
        exit 1
    }
fi

echo "pymllm 安装完成！"

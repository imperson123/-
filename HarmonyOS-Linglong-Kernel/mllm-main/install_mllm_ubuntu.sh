#!/bin/bash

###############################################################################
# MLLM 项目 Ubuntu 自动安装脚本
# 
# 此脚本将自动完成 MLLM 项目在 Ubuntu 系统上的安装
# 包括：依赖安装、项目构建、Python 包安装等
#
# 使用方法:
#   chmod +x install_mllm_ubuntu.sh
#   ./install_mllm_ubuntu.sh
#
# 选项:
#   --skip-deps     跳过系统依赖安装
#   --skip-clone    跳过项目克隆（假设项目已存在）
#   --skip-build    跳过项目构建
#   --skip-pymllm   跳过 Python 包安装
#   --non-interactive  非交互模式（自动回答 yes）
###############################################################################

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 脚本选项
SKIP_DEPS=false
SKIP_CLONE=false
SKIP_BUILD=false
SKIP_PYMLLM=false
NON_INTERACTIVE=false

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-deps)
            SKIP_DEPS=true
            shift
            ;;
        --skip-clone)
            SKIP_CLONE=true
            shift
            ;;
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --skip-pymllm)
            SKIP_PYMLLM=true
            shift
            ;;
        --non-interactive)
            NON_INTERACTIVE=true
            shift
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            echo "使用 --help 查看帮助信息"
            exit 1
            ;;
    esac
done

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查命令是否存在
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# 询问用户确认
ask_yes_no() {
    if [ "$NON_INTERACTIVE" = true ]; then
        echo "y"
        return
    fi
    
    local prompt="$1"
    local default="${2:-n}"
    local answer
    
    if [ "$default" = "y" ]; then
        read -p "$prompt [Y/n]: " answer
    else
        read -p "$prompt [y/N]: " answer
    fi
    
    answer=${answer:-$default}
    if [[ "$answer" =~ ^[Yy]$ ]]; then
        echo "y"
    else
        echo "n"
    fi
}

# 检查系统要求
check_system_requirements() {
    print_info "检查系统要求..."
    
    # 检查操作系统
    if [ ! -f /etc/os-release ]; then
        print_error "无法检测操作系统版本"
        exit 1
    fi
    
    . /etc/os-release
    print_info "检测到操作系统: $NAME $VERSION"
    
    if [[ "$ID" != "ubuntu" ]]; then
        print_warning "此脚本专为 Ubuntu 设计，其他发行版可能不兼容"
        if [ "$(ask_yes_no "是否继续？" "n")" != "y" ]; then
            exit 1
        fi
    fi
    
    # 检查 Python 版本
    if ! command_exists python3; then
        print_error "未找到 python3，请先安装 Python 3"
        exit 1
    fi
    
    PYTHON_VERSION=$(python3 -c 'import sys; print(".".join(map(str, sys.version_info[:2])))')
    PYTHON_MAJOR=$(echo $PYTHON_VERSION | cut -d. -f1)
    PYTHON_MINOR=$(echo $PYTHON_VERSION | cut -d. -f2)
    
    if [ "$PYTHON_MAJOR" -lt 3 ] || ([ "$PYTHON_MAJOR" -eq 3 ] && [ "$PYTHON_MINOR" -lt 10 ]); then
        print_error "需要 Python 3.10 或更高版本，当前版本: $PYTHON_VERSION"
        exit 1
    fi
    
    print_success "Python 版本检查通过: $PYTHON_VERSION"
    
    # 检查 CMake 版本
    if command_exists cmake; then
        CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
        CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
        CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)
        
        if [ "$CMAKE_MAJOR" -lt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 21 ]); then
            print_warning "CMake 版本过低: $CMAKE_VERSION (需要 >= 3.21)"
            SKIP_DEPS=false  # 需要升级 CMake
        else
            print_success "CMake 版本检查通过: $CMAKE_VERSION"
        fi
    else
        print_warning "未找到 CMake"
        SKIP_DEPS=false  # 需要安装 CMake
    fi
    
    # 检查 Git
    if ! command_exists git; then
        print_warning "未找到 Git"
        SKIP_DEPS=false
    else
        print_success "Git 已安装"
    fi
    
    # 检查磁盘空间（至少需要 10GB）
    AVAILABLE_SPACE=$(df -BG . | tail -1 | awk '{print $4}' | sed 's/G//')
    if [ "$AVAILABLE_SPACE" -lt 10 ]; then
        print_warning "可用磁盘空间不足 10GB，当前: ${AVAILABLE_SPACE}GB"
        if [ "$(ask_yes_no "是否继续？" "n")" != "y" ]; then
            exit 1
        fi
    else
        print_success "磁盘空间充足: ${AVAILABLE_SPACE}GB"
    fi
}

# 安装系统依赖
install_system_dependencies() {
    if [ "$SKIP_DEPS" = true ]; then
        print_info "跳过系统依赖安装"
        return
    fi
    
    print_info "开始安装系统依赖..."
    
    # 更新包列表
    print_info "更新包列表..."
    sudo apt update
    
    # 安装基础开发工具
    print_info "安装基础开发工具..."
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
        curl \
        software-properties-common
    
    # 检查并升级 CMake（如果需要）
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
    CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)
    
    if [ "$CMAKE_MAJOR" -lt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 21 ]); then
        print_info "升级 CMake 到最新版本..."
        if command_exists snap; then
            sudo snap install cmake --classic
        else
            print_warning "未找到 snap，尝试从源码编译 CMake..."
            CMAKE_VERSION="3.28.0"
            cd /tmp
            wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.tar.gz
            tar -xzf cmake-${CMAKE_VERSION}.tar.gz
            cd cmake-${CMAKE_VERSION}
            ./bootstrap
            make -j$(nproc)
            sudo make install
            cd -
        fi
    fi
    
    # 升级 pip
    print_info "升级 pip..."
    python3 -m pip install --upgrade pip --user
    
    print_success "系统依赖安装完成"
}

# 克隆项目（可选，因为项目可能已存在）
clone_project() {
    if [ "$SKIP_CLONE" = true ]; then
        print_info "跳过项目克隆"
        return
    fi
    
    if [ -f "CMakeLists.txt" ]; then
        print_info "项目已存在，跳过克隆"
        return
    fi
    
    if [ -d "mllm" ]; then
        print_info "检测到 mllm 目录，跳过克隆"
        return
    fi
    
    print_info "克隆 MLLM 项目..."
    git clone https://github.com/UbiquitousLearning/mllm.git || {
        print_error "克隆项目失败，请检查网络连接"
        print_info "如果项目已存在，请确保在项目目录中运行此脚本"
        exit 1
    }
    
    if [ -d "mllm" ]; then
        cd mllm
    fi
    print_success "项目克隆完成"
}

# 初始化子模块
init_submodules() {
    print_info "初始化 Git 子模块..."
    
    # 检查是否在项目目录中
    if [ ! -f ".gitmodules" ]; then
        print_error "未找到 .gitmodules 文件，请确保在项目根目录中运行此脚本"
        exit 1
    fi
    
    # 初始化子模块（使用浅克隆以加快速度）
    print_info "正在下载子模块（这可能需要一些时间）..."
    git submodule update --init --recursive --depth 1 || {
        print_warning "某些子模块可能下载失败，尝试完整克隆..."
        git submodule update --init --recursive
    }
    
    print_success "子模块初始化完成"
}

# 安装 Python 依赖
install_python_dependencies() {
    print_info "安装 Python 依赖..."
    
    if [ ! -f "requirements.txt" ]; then
        print_error "未找到 requirements.txt 文件"
        exit 1
    fi
    
    # 升级 pip（尝试多种方法）
    print_info "升级 pip..."
    python3 -m pip install --upgrade pip --break-system-packages 2>/dev/null || \
    python3 -m pip install --upgrade pip --user 2>/dev/null || \
    python3 -m pip install --upgrade pip 2>/dev/null || true
    
    # 安装依赖（处理 externally-managed-environment 错误）
    print_info "安装 requirements.txt 中的依赖..."
    
    # 首先尝试使用 --break-system-packages（适用于 Ubuntu 23.04+）
    if python3 -m pip install --break-system-packages -r requirements.txt 2>/dev/null; then
        print_success "使用 --break-system-packages 安装成功"
    # 然后尝试使用 --user
    elif python3 -m pip install --user -r requirements.txt 2>/dev/null; then
        print_success "使用 --user 安装成功"
    # 最后尝试全局安装（需要 sudo）
    else
        print_warning "尝试使用 sudo 全局安装..."
        sudo python3 -m pip install -r requirements.txt || {
            print_error "Python 依赖安装失败"
            print_info "如果遇到 externally-managed-environment 错误，请使用以下方法之一："
            print_info "1. 使用虚拟环境: python3 -m venv venv && source venv/bin/activate"
            print_info "2. 使用 --break-system-packages: pip install --break-system-packages -r requirements.txt"
            exit 1
        }
    fi
    
    print_success "Python 依赖安装完成"
}

# 构建项目
build_project() {
    if [ "$SKIP_BUILD" = true ]; then
        print_info "跳过项目构建"
        return
    fi
    
    print_info "开始构建项目..."
    print_warning "这可能需要 30-60 分钟，请耐心等待..."
    
    # 检查 task.py 是否存在
    if [ ! -f "task.py" ]; then
        print_error "未找到 task.py 文件"
        exit 1
    fi
    
    # 检查构建任务文件是否存在
    if [ ! -f "tasks/build_x86.yaml" ]; then
        print_error "未找到 tasks/build_x86.yaml 文件"
        exit 1
    fi
    
    # 运行构建任务
    print_info "执行构建任务: tasks/build_x86.yaml"
    python3 task.py tasks/build_x86.yaml || {
        print_error "构建失败"
        print_info "您可以尝试手动构建："
        print_info "  mkdir -p build && cd build"
        print_info "  cmake .. -DCMAKE_BUILD_TYPE=Release -DMLLM_ENABLE_PY_MLLM=ON"
        print_info "  make -j\$(nproc)"
        exit 1
    }
    
    print_success "项目构建完成"
}

# 安装 Python 包
install_pymllm() {
    if [ "$SKIP_PYMLLM" = true ]; then
        print_info "跳过 Python 包安装"
        return
    fi
    
    print_info "安装 pymllm Python 包..."
    
    # 检查安装脚本是否存在
    if [ -f "scripts/install_pymllm.sh" ]; then
        print_info "使用安装脚本安装..."
        bash ./scripts/install_pymllm.sh || {
            print_warning "安装脚本执行失败，尝试手动安装..."
            install_pymllm_manual
        }
    else
        print_info "使用手动方式安装..."
        install_pymllm_manual
    fi
    
    print_success "pymllm 安装完成"
}

# 手动安装 pymllm
install_pymllm_manual() {
    print_info "构建 wheel 包..."
    python3 -m pip wheel -v -w dist . || {
        print_error "构建 wheel 包失败"
        exit 1
    }
    
    print_info "安装 wheel 包..."
    # 尝试多种安装方法
    if python3 -m pip install --break-system-packages dist/*.whl --force-reinstall 2>/dev/null; then
        print_success "使用 --break-system-packages 安装成功"
    elif python3 -m pip install --user dist/*.whl --force-reinstall 2>/dev/null; then
        print_success "使用 --user 安装成功"
    else
        print_warning "尝试使用 sudo 全局安装..."
        sudo python3 -m pip install dist/*.whl --force-reinstall || {
            print_error "安装 wheel 包失败"
            exit 1
        }
    fi
}

# 验证安装
verify_installation() {
    print_info "验证安装..."
    
    # 检查 mllm-convertor
    if command_exists mllm-convertor; then
        print_success "mllm-convertor 已安装"
        mllm-convertor --help > /dev/null 2>&1 && print_success "mllm-convertor 工作正常"
    else
        print_warning "mllm-convertor 未在 PATH 中找到"
        print_info "尝试添加到 PATH: export PATH=\$PATH:~/.local/bin"
        if [ -f ~/.local/bin/mllm-convertor ]; then
            print_success "mllm-convertor 存在于 ~/.local/bin/"
        fi
    fi
    
    # 检查 Python 模块
    if python3 -c "import pymllm" 2>/dev/null; then
        print_success "pymllm Python 模块可以正常导入"
    else
        print_warning "pymllm Python 模块导入失败"
        print_info "请检查安装路径和 Python 环境"
    fi
    
    # 检查构建产物
    if [ -d "build" ] && [ -f "build/bin/libMllmRT.so" ] || [ -f "build/libMllmRT.so" ]; then
        print_success "找到构建产物"
    else
        print_warning "未找到构建产物，构建可能未完成"
    fi
}

# 显示安装总结
show_summary() {
    echo ""
    echo "=========================================="
    print_success "安装完成！"
    echo "=========================================="
    echo ""
    echo "下一步操作："
    echo ""
    echo "1. 如果 mllm-convertor 命令不可用，请运行："
    echo "   export PATH=\$PATH:~/.local/bin"
    echo "   或将其添加到 ~/.bashrc 中"
    echo ""
    echo "2. 转换模型："
    echo "   mllm-convertor --help"
    echo ""
    echo "3. 查看示例："
    echo "   cd examples/"
    echo "   ls"
    echo ""
    echo "4. 阅读文档："
    echo "   https://ubiquitouslearning.github.io/mllm/"
    echo ""
    echo "5. 获取帮助："
    echo "   https://github.com/UbiquitousLearning/mllm/issues"
    echo ""
}

# 主函数
main() {
    echo ""
    echo "=========================================="
    echo "  MLLM 项目 Ubuntu 自动安装脚本"
    echo "=========================================="
    echo ""
    
    # 记录开始时间
    START_TIME=$(date +%s)
    
    # 检查系统要求
    check_system_requirements
    
    # 安装系统依赖
    install_system_dependencies
    
    # 检查项目目录（不需要克隆，因为项目已存在）
    CURRENT_DIR=$(pwd)
    if [ -f "CMakeLists.txt" ]; then
        print_info "已在项目根目录中: $(pwd)"
    elif [ -d "mllm" ]; then
        print_info "进入 mllm 目录"
        cd mllm
    else
        print_warning "未找到项目目录，尝试克隆..."
        clone_project
        if [ -d "mllm" ]; then
            cd mllm
        fi
    fi
    
    # 确认在正确的目录
    if [ ! -f "CMakeLists.txt" ]; then
        print_error "未找到 CMakeLists.txt，请确保在项目根目录中运行此脚本"
        print_info "当前目录: $(pwd)"
        exit 1
    fi
    
    # 初始化子模块
    init_submodules
    
    # 安装 Python 依赖
    install_python_dependencies
    
    # 构建项目
    build_project
    
    # 安装 Python 包
    install_pymllm
    
    # 验证安装
    verify_installation
    
    # 显示总结
    show_summary
    
    # 计算总耗时
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    MINUTES=$((DURATION / 60))
    SECONDS=$((DURATION % 60))
    
    print_success "总耗时: ${MINUTES} 分 ${SECONDS} 秒"
    echo ""
}

# 运行主函数
main


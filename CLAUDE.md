# CLAUDE.md

此文件为 Claude Code (claude.ai/code) 在处理此代码仓库时提供指导。

## 项目概述

MNN 是一个高效且轻量级的深度学习框架。它支持深度学习模型的推理和训练，在设备端推理和训练方面具有业界领先的性能。

主要特性：
- 支持 TensorFlow, Caffe, ONNX, Torchscripts 和常见的神经网络如 CNN, RNN, GAN, Transformer
- 轻量化，针对设备部署进行了优化 (iOS: ~12MB, Android: ~800KB)
- 高性能，针对 ARM/x64 CPU 优化的汇编代码和 GPU 推理支持 (Metal/OpenCL/Vulkan)
- 支持 iOS 8.0+, Android 4.3+, 以及带有 POSIX 接口的嵌入式设备

## 架构

MNN 架构由几个关键组件组成：

1. **MNN Converter**: 将模型从 TensorFlow, Caffe, ONNX, Torchscripts 转换为 MNN 格式
2. **MNN Engine**: 核心推理引擎，支持多种后端 (CPU, GPU, NPU)
3. **MNN Express**: 基于表达式的 API，用于模型构建和训练
4. **MNN CV**: 类似 OpenCV 的轻量级图像处理模块
5. **MNN Train**: 用于设备端训练的训练框架

## 常见开发任务

### 构建项目

**Linux/MacOS:**
```bash
mkdir build && cd build
cmake .. [options]
make -j8
```

关键构建选项：
- `-DMNN_BUILD_LLM=ON` - 启用 LLM 支持
- `-DMNN_LOW_MEMORY=ON` - 启用低内存模式
- `-DMNN_BUILD_TRAIN=ON` - 启用训练框架
- `-DMNN_OPENCL=ON` - 启用 OpenCL 后端
- `-DMNN_VULKAN=ON` - 启用 Vulkan 后端
- `-DMNN_CUDA=ON` - 启用 CUDA 后端

**Android:**
```bash
cd project/android
mkdir build_64 && cd build_64
../build_64.sh [options]
```

**iOS:**
```bash
sh package_scripts/ios/buildiOS.sh [options]
```

### 运行测试

在构建目录中：
```bash
# 单元测试
./run_test.out

# 模型测试
./testModel.out model.mnn input.txt output.txt

# 性能分析
./timeProfile.out model.mnn
```

### LLM 开发

对于 LLM 相关的开发：
1. 使用 `transformers/llm/export/llmexport.py` 导出模型
2. 使用 `-DMNN_BUILD_LLM=ON -DMNN_LOW_MEMORY=ON -DMNN_SUPPORT_TRANSFORMER_FUSE=ON` 构建
3. 通过 `config.json` 文件进行运行时配置

关键目录：
- `transformers/llm/` - LLM 引擎和运行时
- `transformers/llm/export/` - 模型导出工具
- `apps/Android/MnnLlmChat/` - Android LLM 聊天应用
- `apps/iOS/MNNLLMChat/` - iOS LLM 聊天应用

### 代码风格

- 使用 `clang-format` 和项目的 `.clang-format` 文件进行代码格式化
- 遵循 Google C++ 风格指南，并有以下修改：
  - AccessModifierOffset: -4
  - ColumnLimit: 120
  - IndentWidth: 4
- 使用 camelCase 命名约定
- 私有/受保护成员以 `m` 为前缀 (例如, `mCat`)
- 全局/静态变量以 `g` 为前缀 (例如, `gWorld`)
- 所有公共函数/类必须用 `MNN_PUBLIC` 标记
- 使用防御性编程进行参数验证
- 遵循"临近释放原则" - 在相邻代码块中分配和释放内存

## 重要约束

- 类中不允许操作符重载
- 结构体不允许自定义构造函数
- 不允许使用 C++ 流 (cout/cin, ifstream/ofstream 等)
- 不允许使用 C++ 异常处理 (try/catch/throw)
- 汇编代码必须有 C 语言等价实现且仅使用兼容类型
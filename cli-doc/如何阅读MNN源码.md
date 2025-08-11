# 如何阅读MNN源码

✦ 要阅读 MNN 框架的源码，可以按照以下步骤进行：

   1. 理解整体架构：首先阅读 README.md 文件，了解 MNN
      的基本功能、特性、支持的平台和模型格式。其中的架构图 (doc/architecture.png) 也很有帮助。
   2. 入口点分析：
       * 从用户 API 入手，include/MNN/Interpreter.hpp 和 source/core/Interpreter.cpp
         是模型推理的入口点。Interpreter 类负责加载模型、创建会话和执行推理。
       * Interpreter::createFromFile 或 Interpreter::createFromBuffer 用于加载模型。
       * Interpreter::createSession 用于根据配置（如后端类型、线程数）创建会话。
       * Interpreter::runSession 用于执行推理。
   3. 核心概念：
       * Tensor: include/MNN/Tensor.hpp 和 source/core/Tensor.cpp 定义了张量，是数据的基本单位。
       * Session: source/core/Session.hpp 代表一次推理会话，包含模型运行时的上下文。
       * Backend: source/core/Backend.hpp
         定义了后端接口，不同的后端（CPU、GPU、NPU）实现了具体的计算逻辑。后端类型由
         MN[package_scripts](../package_scripts)NForwardType 定义。
       * Execution: source/core/Execution.hpp 是具体算子（Operator）的执行单元，由 Backend
         创建。
       * Pipeline: source/core/Pipeline.hpp
         管理模型中的一段计算图（可能是一个子图），负责算子的调度、内存分配和执行。
       * Schedule: source/core/Schedule.hpp 负责根据模型结构和会话配置，将模型划分为多个
         Pipeline。
   4. 深入源码：
       * 模型加载与解析：从 Interpreter::createFromFile/Buffer 追踪，会涉及到 FileLoader
         (加载文件) 和 FlatBuffers (解析模型结构 schema/current/MNN.fbs)。
       * 会话创建：Interpreter::createSession 会调用 Schedule 来规划模型，生成 Session 对象。
       * 模型执行：Interpreter::runSession 会调用 Session::run()，进而调用各个
         Pipeline::execute()。
       * 后端实现：在 source/backend 目录下，有针对不同硬件平台的后端实现，例如 CPU
         (source/backend/cpu)、OpenCL (source/backend/opencl)
         等。可以深入特定后端，了解具体算子的实现。
       * 算子实现：算子实现在各个后端目录下的 execution 子目录中。例如，CPU 后端的卷积算子可能在
         source/backend/cpu/compute/ConvolutionCommon.cpp 或类似文件中。
   5. 工具和文档：
       * 查看 docs 目录下的文档，了解更多细节。
       * tools 目录下有一些有用的工具，例如模型转换器 (MNNConverter)、模型压缩工具 (MNNCompress)
         等，可以帮助理解模型格式和优化过程。
       * demo 目录下的示例代码可以作为参考。
       * test 目录下的测试用例也是很好的学习资源。

  通过以上步骤，你可以逐步深入理解 MNN
  框架的源码结构和工作原理。建议从简单的模型推理流程开始，逐步深入到具体的后端和算子实现。
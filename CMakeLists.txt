# 设置所需的最低CMake版本
cmake_minimum_required(VERSION 3.6)

# 版本信息处理
# 从MNNDefine.h文件中读取版本定义字符串
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/include/MNN/MNNDefine.h" MNN_DEFINE)
# 使用正则表达式提取主版本号定义
string(REGEX MATCH "MNN_VERSION_MAJOR [0-9]+" MNN_VERSION_MAJOR_DEFINE ${MNN_DEFINE})
# 从定义中提取主版本号数字
string(REGEX MATCH "[0-9]+" MNN_VERSION_MAJOR ${MNN_VERSION_MAJOR_DEFINE})
# 使用正则表达式提取次版本号定义
string(REGEX MATCH "MNN_VERSION_MINOR [0-9]+" MNN_VERSION_MINOR_DEFINE ${MNN_DEFINE})
# 从定义中提取次版本号数字
string(REGEX MATCH "[0-9]+" MNN_VERSION_MINOR ${MNN_VERSION_MINOR_DEFINE})
# 使用正则表达式提取修订版本号定义
string(REGEX MATCH "MNN_VERSION_PATCH [0-9]+" MNN_VERSION_PATCH_DEFINE ${MNN_DEFINE})
# 从定义中提取修订版本号数字
string(REGEX MATCH "[0-9]+" MNN_VERSION_PATCH ${MNN_VERSION_PATCH_DEFINE})
# 组合完整版本号
set(MNN_VERSION ${MNN_VERSION_MAJOR}.${MNN_VERSION_MINOR}.${MNN_VERSION_PATCH})

# 当项目未指定版本时清除版本变量
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()

# MSVC运行时库标志通过抽象层选择
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

# 定义项目名称、版本和使用的编程语言
project(MNN VERSION ${MNN_VERSION} LANGUAGES C CXX ASM)

# 编译器选项设置
# 设置C语言标准为C99
set(CMAKE_C_STANDARD 99)
# 如果C++标准不是C++17，则设置为C++11
IF (NOT (CMAKE_CXX_STANDARD EQUAL 17))
  set(CMAKE_CXX_STANDARD 11)
ENDIF()
# 设置CMake模块路径，用于查找自定义模块
set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_LIST_DIR}/cmake"
)

# Windows平台特定设置
if(WIN32)
  # 如果不是MSVC编译器
  if(NOT MSVC)
    # 清空MSVC运行时库设置
    set(CMAKE_MSVC_RUNTIME_LIBRARY "")
    set(MSVC_RUNTIME_LIBRARY "")
  endif()
endif()

# 构建选项配置
# 是否使用系统库（OpenCL和Vulkan）或使用dlopen动态加载
option(MNN_USE_SYSTEM_LIB "For opencl and vulkan, use system lib or use dlopen" OFF)
# 是否构建硬浮点ABI版本
option(MNN_BUILD_HARD "Build -mfloat-abi=hard or not" OFF)
# MNN构建共享库还是静态库
option(MNN_BUILD_SHARED_LIBS "MNN build shared or static lib" ON)
# Windows上是否使用/MT运行时库
option(MNN_WIN_RUNTIME_MT "MNN use /MT on Windows dll" OFF)
# 是否禁用多线程
option(MNN_FORBID_MULTI_THREAD "Disable Multi Thread" OFF)
# 是否使用OpenMP线程池实现（在iOS或Mac OS上不工作）
option(MNN_OPENMP "Use OpenMP's thread pool implementation. Does not work on iOS or Mac OS" OFF)
# 是否使用MNN自己的线程池实现
option(MNN_USE_THREAD_POOL "Use MNN's own thread pool implementation" ON)
# 是否构建MNN训练框架
option(MNN_BUILD_TRAIN "Build MNN's training framework" OFF)
# 是否构建演示程序
option(MNN_BUILD_DEMO "Build demo/exec or not" OFF)
# 是否构建工具程序
option(MNN_BUILD_TOOLS "Build tools/cpp or not" ON)
# 是否构建量化工具
option(MNN_BUILD_QUANTOOLS "Build Quantized Tools or not" OFF)
# 是否构建评估工具
option(MNN_EVALUATION "Build Evaluation Tools or not" OFF)
# 是否构建转换器
option(MNN_BUILD_CONVERTER "Build Converter" OFF)
# 是否移除不常用的算子并优化代码
option(MNN_REDUCE_SIZE "Remove not common op and optimize code" OFF)
# 是否支持扩展的int8算子（层归一化、二元运算、一元运算、缩放、softmax、插值）
option(MNN_SUPPORT_QUANT_EXTEND "Add int8 op for layernorm, binary, unary, scale, softmax, interp" ON)
# 是否启用MNN的TFLite量化算子
option(MNN_SUPPORT_DEPRECATED_OP "Enable MNN's tflite quantized op" OFF)
# 是否启用MNN的已弃用算子几何变换（如3D卷积/反卷积，已在转换器中处理）
option(MNN_SUPPORT_DEPRECATED_OPV2 "Enable MNN's DEPRECATED op's geometry such as conv3d / deconv3d, which has been treated in converter" ON)
# 是否启用MNN内存访问调试
option(MNN_DEBUG_MEMORY "MNN Debug Memory Access" OFF)
# 是否启用张量大小调试
option(MNN_DEBUG_TENSOR_SIZE "Enable Tensor Size" OFF)
# 是否启用MNN GPU调试
option(MNN_GPU_TRACE "Enable MNN Gpu Debug" OFF)
# 是否启用MNN渲染操作
option(MNN_SUPPORT_RENDER "Enable MNN Render Ops" OFF)
# 是否启用MNN变压器融合操作
option(MNN_SUPPORT_TRANSFORMER_FUSE "Enable MNN transformer Fuse Ops" OFF)
# 是否分别构建MNN后端和表达式库（仅在MNN_BUILD_SHARED_LIBS=ON时有效）
option(MNN_SEP_BUILD "Build MNN Backends and expression separately. Only works with MNN_BUILD_SHARED_LIBS=ON" ON)
# 原生库输出路径
option(NATIVE_LIBRARY_OUTPUT "Native Library Path" OFF)
# 原生头文件输出路径
option(NATIVE_INCLUDE_OUTPUT "Native Include Path" OFF)
# 是否构建MNN.framework而不是传统的.a/.dylib
option(MNN_AAPL_FMWK "Build MNN.framework instead of traditional .a/.dylib" OFF)
# 是否构建插件算子支持
option(MNN_WITH_PLUGIN "Build with plugin op support." OFF)
# 是否跳过构建MNN-Geometry（仅支持固定形状模型）
option(MNN_SKIPBUILD_GEOMETRY "Skip Build MNN-Geometry, then only supports fixed shape models." OFF)
# 是否构建最小化MNN库（自动设置MNN_SKIPBUILD_GEOMETRY和MNN_REDUCE_SIZE为ON）
option(MNN_BUILD_MINI "Build minimal MNN so, set MNN_SKIPBUILD_GEOMETRY and MNN_REDUCE_SIZE ON" OFF)
# 是否使用SSE优化（x86平台）
option(MNN_USE_SSE "Use SSE optimization for x86 if possiable" ON)
# 是否构建代码生成器
option(MNN_BUILD_CODEGEN "Build with codegen" OFF)
# 是否启用代码覆盖率构建
option(MNN_ENABLE_COVERAGE "Build with coverage enable" OFF)
# 是否在MNN中构建Protocol Buffer
option(MNN_BUILD_PROTOBUFFER "Build with protobuffer in MNN" ON)
# 是否在MNN中构建OpenCV API
option(MNN_BUILD_OPENCV "Build OpenCV api in MNN." OFF)
# 是否构建基于MNN的大语言模型库
option(MNN_BUILD_LLM "Build llm library based MNN." OFF)
# 是否构建基于MNN的扩散模型演示
option(MNN_BUILD_DIFFUSION "Build diffusion demo based MNN." OFF)
# 是否构建MNN内部功能（如模型认证、指标日志）
option(MNN_INTERNAL "Build with MNN internal features, such as model authentication, metrics logging" OFF)
# 是否构建MNN JNI供Java使用
option(MNN_JNI "Build MNN Jni for java to use" OFF)
# 是否启用MNN的bf16算子
option(MNN_SUPPORT_BF16 "Enable MNN's bf16 op" OFF)
# 是否构建支持低内存的权重量化模型
option(MNN_LOW_MEMORY "Build MNN support low memory for weight quant model." OFF)
# 是否构建MNN CPU权重反量化相关的GEMM内核
option(MNN_CPU_WEIGHT_DEQUANT_GEMM "Build MNN CPU weight dequant related gemm kernels." OFF)
# 是否在MNN中构建音频API
option(MNN_BUILD_AUDIO "Build audio api in MNN." OFF)
# 是否使用Arm SME2指令
option(MNN_SME2 "Use Arm sme2 instructions" ON)

# MNN_BUILD_MINI选项处理
# 如果启用了最小构建，则自动设置相关选项
if (MNN_BUILD_MINI)
    # 跳过构建MNN-Geometry
    set(MNN_SKIPBUILD_GEOMETRY ON)
    # 移除不常用的算子并优化代码
    set(MNN_REDUCE_SIZE ON)
endif()

# MNN_REDUCE_SIZE选项处理
# 如果启用了代码精简，则关闭相关功能
if (MNN_REDUCE_SIZE)
    # 关闭已弃用的TFLite量化算子支持
    set(MNN_SUPPORT_DEPRECATED_OP OFF)
    # 关闭已弃用算子的几何变换支持
    set(MNN_SUPPORT_DEPRECATED_OPV2 OFF)
    # 关闭量化扩展算子支持
    set(MNN_SUPPORT_QUANT_EXTEND OFF)
    # 关闭稀疏计算
    set(MNN_USE_SPARSE_COMPUTE OFF)
endif()

# 鸿蒙系统内部构建处理
IF (OHOS AND MNN_INTERNAL)
  # 包含鸿蒙系统的包管理配置
  include($ENV{NODE_PATH}/@ali/tcpkg/tcpkg.cmake)
  # 导出MNN头文件
  export_headers(DIR ${CMAKE_SOURCE_DIR}/include/MNN)
  # 如果构建OpenCV支持，则导出OpenCV头文件
  IF (MNN_BUILD_OPENCV)
    export_headers(DIR ${CMAKE_SOURCE_DIR}/tools/cv/include/cv)
  ENDIF()
ENDIF()

# 稀疏计算选项默认值设置
IF (NOT DEFINED MNN_USE_SPARSE_COMPUTE)
   # 默认启用稀疏计算
   set(MNN_USE_SPARSE_COMPUTE ON)
ENDIF()

# 构建配置冲突检查和处理
# 静态库构建时关闭分离构建
IF(NOT MNN_BUILD_SHARED_LIBS AND MNN_SEP_BUILD)
  message(WARNING "Close MNN_SEP_BUILD for static library")
  SET(MNN_SEP_BUILD OFF CACHE BOOL "<docstring>" FORCE)
ENDIF()

# Apple平台框架构建与分离构建冲突处理
IF(APPLE AND MNN_AAPL_FMWK AND MNN_SEP_BUILD)
  message(WARNING "MNN_SEP_BUILD AND MNN_AAPL_FMWK can't coexist. Turning off MNN_SEP_BUILD")
  SET(MNN_SEP_BUILD OFF CACHE BOOL "<docstring>" FORCE)
ENDIF()

# Windows平台分离构建警告和处理
IF(WIN32)
  IF(MNN_SEP_BUILD)
    message(WARNING "MNN_SEP_BUILD IS TROUBLESOME ON Windows. Forcing OFF...")
    SET(MNN_SEP_BUILD OFF CACHE BOOL "<docstring>" FORCE)
  ENDIF()
  # 添加定义以禁用CRT安全警告
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)

  # MSVC编译器特定设置
  IF(MSVC)
    # 生成优化的发布版可执行文件和库，并包含pdb调试文件
    # 参考: https://stackoverflow.com/a/31264946
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    # 设置C和C++编译器的调试信息标志
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")

    # 禁用特定的MSVC编译器警告
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4267 /wd4018 /wd4251 /wd4996 /wd4244 /wd4146 /wd4129 /wd4305 /wd4275 /wd4101")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4018 /wd4251 /wd4996 /wd4244 /wd4146 /wd4129 /wd4305 /wd4275 /wd4101")
  ENDIF()
ENDIF()

# 覆盖率测试配置
# 如果启用了覆盖率测试，则添加相应的编译标志
IF( MNN_ENABLE_COVERAGE)
    # 为C++编译器添加覆盖率测试标志
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    # 为C编译器添加覆盖率测试标志
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
ENDIF()

# macOS架构设置处理
# 如果是Darwin系统且设置了CMAKE_OSX_ARCHITECTURES，则使用该设置作为系统处理器
if ((CMAKE_SYSTEM_NAME STREQUAL "Darwin") AND CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_OSX_ARCHITECTURES})
endif()

# MSVC运行时库配置（在protobuf之前执行，确保protobuf和MNN的wincrt配置一致）
if(MSVC)
    # 与protobuf保持一致，否则配置会不一致
    # 如果CMake版本大于等于3.15
    if(CMAKE_VERSION VERSION_GREATER 3.15 OR CMAKE_VERSION VERSION_EQUAL 3.15)
      # 设置MSVC运行时库为多线程版本，调试模式下为调试版本
      set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
      # 如果不使用/MT运行时库，则添加DLL后缀
      if(NOT MNN_WIN_RUNTIME_MT)
        set(CMAKE_MSVC_RUNTIME_LIBRARY ${CMAKE_MSVC_RUNTIME_LIBRARY}DLL)
      endif()
    else()
      # 对于较老的CMake版本，遍历所有编译标志变量进行设置
      foreach(flag_var
          CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
          CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
          CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
          # 如果使用/MT运行时库
          if (MNN_WIN_RUNTIME_MT)
              # 如果当前标志包含/MD，则替换为/MT
              if(${flag_var} MATCHES "/MD")
                  string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
              endif()
          else ()
              # 如果当前标志包含/MT，则替换为/MD
              if(${flag_var} MATCHES "/MT")
                  string(REGEX REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
              endif()
          endif ()
      endforeach()
    endif()
    # 设置protobuf的共享库构建选项与MNN保持一致
    set(protobuf_BUILD_SHARED_LIBS ${MNN_BUILD_SHARED_LIBS})
endif()

# 包含自定义宏定义文件
include(${CMAKE_CURRENT_LIST_DIR}/cmake/macros.cmake)

# Protocol Buffer构建配置
# 如果启用了Protocol Buffer构建
IF(MNN_BUILD_PROTOBUFFER)
  # 如果启用了转换器构建
  IF(MNN_BUILD_CONVERTER)
    # MSVC编译器特定设置
    IF(MSVC)
      # 设置protobuf的共享库构建选项与MNN保持一致
      set(protobuf_BUILD_SHARED_LIBS ${MNN_BUILD_SHARED_LIBS})
      # 检查配置冲突：当启用转换器且构建静态库但不使用/MT运行时库时，抛出致命错误
      # 因为protobuf不支持静态库与/MD配置的组合
      IF((NOT MNN_BUILD_SHARED_LIBS) AND (NOT MNN_WIN_RUNTIME_MT))
        message(FATAL_ERROR "When MNN_BUILD_CONVERTER=ON and MNN_BUILD_SHARED_LIBS=OFF, MNN_WIN_RUNTIME_MT must be ON. Because protobuf not support the config(static /MD)")
      ENDIF()
    ENDIF()
    # 添加protobuf子目录进行构建
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/3rd_party/protobuf/cmake)
  ENDIF()
ENDIF()

# 显式指定源文件编码，修复跨平台乱码问题
# 需要在protobuf之后执行，因为protobuf设置了不同的执行字符集
IF(MSVC)
  # 为C和C++编译器设置源字符集为UTF-8
  set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /source-charset:utf-8")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /source-charset:utf-8")
ENDIF()

# GNU编译器静态库构建配置
# 如果使用GNU编译器且构建静态库且不是Windows平台
IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT MNN_BUILD_SHARED_LIBS AND NOT (MSVC OR WIN32))
  # 设置可执行文件链接标志
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
  # 强制关闭分离构建
  SET(MNN_SEP_BUILD OFF CACHE BOOL "<docstring>" FORCE)
ENDIF()

# 多线程禁用定义
# 如果禁用了多线程，则添加相应的预处理器定义
if(MNN_FORBID_MULTI_THREAD)
    add_definitions(-DMNN_FORBIT_MULTI_THREADS)
endif()

# 已弃用算子支持定义
# 如果启用了已弃用的TFLite量化算子支持，则添加相应的预处理器定义
if(MNN_SUPPORT_DEPRECATED_OP)
    add_definitions(-DMNN_SUPPORT_DEPRECATED_OP)
endif()

# 已弃用算子V2支持定义
# 如果启用了已弃用算子的几何变换支持，则添加相应的预处理器定义
if(MNN_SUPPORT_DEPRECATED_OPV2)
    add_definitions(-DMNN_SUPPORT_DEPRECATED_OPV2)
endif()

# 代码精简定义
# 如果启用了代码精简，则添加相应的预处理器定义
if (MNN_REDUCE_SIZE)
    add_definitions(-DMNN_REDUCE_SIZE)
endif()

# 量化扩展算子支持定义
# 如果启用了量化扩展算子支持，则添加相应的预处理器定义
if(MNN_SUPPORT_QUANT_EXTEND)
    add_definitions(-DMNN_SUPPORT_QUANT_EXTEND)
endif()

# 低内存支持定义
# 如果启用了低内存支持，则添加相应的预处理器定义
if(MNN_LOW_MEMORY)
    add_definitions(-DMNN_LOW_MEMORY)
endif()

# 渲染操作支持定义
# 如果启用了渲染操作支持，则添加相应的预处理器定义
if(MNN_SUPPORT_RENDER)
    add_definitions(-DMNN_SUPPORT_RENDER)
endif()

# 变压器融合操作支持定义
# 如果启用了变压器融合操作支持，则添加相应的预处理器定义
if(MNN_SUPPORT_TRANSFORMER_FUSE)
    add_definitions(-DMNN_SUPPORT_TRANSFORMER_FUSE)
endif()

# 调试选项定义
# 如果启用了内存调试，则添加相应的预处理器定义
if(MNN_DEBUG_MEMORY)
    add_definitions(-DMNN_DEBUG_MEMORY)
endif()

# 张量大小调试定义
# 如果启用了张量大小调试，则添加相应的预处理器定义
if(MNN_DEBUG_TENSOR_SIZE)
    add_definitions(-DMNN_DEBUG_TENSOR_SIZE)
endif()

# GPU跟踪定义
# 如果启用了GPU跟踪，则添加相应的预处理器定义
if(MNN_GPU_TRACE)
    add_definitions(-DMNN_GPU_FORCE_FINISH)
endif()

# 后端选项配置
# 是否启用Metal后端（Apple平台）
option(MNN_METAL "Enable Metal" OFF)
# 是否启用OpenCL后端
option(MNN_OPENCL "Enable OpenCL" OFF)
# 是否启用OpenGL后端
option(MNN_OPENGL "Enable OpenGL" OFF)
# 是否启用Vulkan后端
option(MNN_VULKAN "Enable Vulkan" OFF)
# 是否启用ARMv8.2的FP16计算支持
option(MNN_ARM82 "Enable ARMv8.2's FP16 Compute" ON)
# 是否为armv7架构启用ARMv8.2的FP16计算支持（可能导致32位CPU上的库无效）
option(MNN_SUPPORT_FP16_ARMV7 "Enable ARMv8.2's FP16 Compute for armv7 arch, may cause library not valid for 32 bit cpu" OFF)
# 是否启用KLEIDIAI后端
option(MNN_KLEIDIAI "Enable KLEIDIAI" OFF)
# 是否启用oneDNN后端
option(MNN_ONEDNN "Enable oneDNN" OFF)
# 是否为x86平台启用AVX2编译优化
option(MNN_AVX2 "Open AVX2 Compile for x86 if possible" ON)
# 是否启用AVX512指令集支持
option(MNN_AVX512 "Enable AVX512" OFF)
# 是否启用CUDA后端
option(MNN_CUDA "Enable CUDA" OFF)
# 是否启用TensorRT后端
option(MNN_TENSORRT "Enable TensorRT" OFF)
# 是否启用CoreML后端（Apple平台）
option(MNN_COREML "Enable CoreML" OFF)
# 是否启用NNAPI后端（Android平台）
option(MNN_NNAPI "Enable NNAPI" OFF)

# 是否启用GPU时间性能分析（OpenCL和Vulkan后端）
option(MNN_GPU_TIME_PROFILE "Enable time profiling for the OpenCL backend and Vulkan backend." OFF)

# 是否启用CUDA性能分析
option(MNN_CUDA_PROFILE "Enable CUDA profile" OFF)

# CUDA性能分析选项处理
# 如果未启用CUDA或系统不是Linux，则关闭CUDA性能分析
if (NOT MNN_CUDA OR NOT CMAKE_SYSTEM_NAME MATCHES "^Linux")
  set(MNN_CUDA_PROFILE OFF)
endif()

# 线程池选项处理
# 如果使用MNN自己的线程池实现
if (MNN_USE_THREAD_POOL)
    # 输出状态信息：使用线程池，禁用OpenMP
    message(STATUS "Use Threadpool, forbid openmp")
    # 关闭OpenMP支持
    set(MNN_OPENMP OFF)
    # 添加预处理器定义
    add_definitions(-DMNN_USE_THREAD_POOL)
endif()

# Android arm32构建处理
# 当基于arm32架构为Android构建时，强制关闭MNN_ARM82
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^armv7" OR ARCHS MATCHES "^armv7(;armv7s)?")
    # 如果未启用armv7的FP16支持
    if (NOT MNN_SUPPORT_FP16_ARMV7)
        # 输出状态信息：为Android arm32构建时强制关闭MNN_ARM82
        message(STATUS "force turn off MNN_ARM82 when build for Android based on arm32 by MTL")
        # 强制关闭MNN_ARM82选项
        SET(MNN_ARM82 OFF CACHE BOOL "Enable ARM82" FORCE)
    endif()
endif()

# 目标选项配置
# 是否构建性能基准测试程序
option(MNN_BUILD_BENCHMARK "Build benchmark or not" OFF)
# 是否构建测试程序
option(MNN_BUILD_TEST "Build tests or not" OFF)
# 是否通过命令行构建Android版本
option(MNN_BUILD_FOR_ANDROID_COMMAND "Build from command" OFF)
# 是否使用Logcat替代print输出信息（Android平台）
option(MNN_USE_LOGCAT "Use Logcat intead of print for info" ON)

# 隐藏符号选项设置
# 默认不隐藏符号
set (MNN_HIDDEN FALSE)
# 如果构建类型不是Debug，则隐藏符号
IF(CMAKE_BUILD_TYPE MATCHES Debug)
ELSE()
    set(MNN_HIDDEN TRUE)
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

# 输出构建信息
message(STATUS ">>>>>>>>>>>>>")
message(STATUS "MNN BUILD INFO:")
message(STATUS "\tSystem: ${CMAKE_SYSTEM_NAME}")
message(STATUS "\tProcessor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "\tVersion: ${MNN_VERSION}")
message(STATUS "\tMetal: ${MNN_METAL}")
message(STATUS "\tOpenCL: ${MNN_OPENCL}")
message(STATUS "\tOpenGL: ${MNN_OPENGL}")
message(STATUS "\tVulkan: ${MNN_VULKAN}")
message(STATUS "\tARM82: ${MNN_ARM82}")
message(STATUS "\tKleidiAI: ${MNN_KLEIDIAI}")
message(STATUS "\toneDNN: ${MNN_ONEDNN}")
message(STATUS "\tTensorRT: ${MNN_TENSORRT}")
message(STATUS "\tCoreML: ${MNN_COREML}")
message(STATUS "\tNNAPI: ${MNN_NNAPI}")
message(STATUS "\tCUDA: ${MNN_CUDA}")
message(STATUS "\tOpenMP: ${MNN_OPENMP}")
message(STATUS "\tBF16: ${MNN_SUPPORT_BF16}")
message(STATUS "\tThreadPool: ${MNN_USE_THREAD_POOL}")
message(STATUS "\tHidden: ${MNN_HIDDEN}")
message(STATUS "\tBuild Path: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "\tCUDA PROFILE: ${MNN_CUDA_PROFILE}")

# Android和Linux平台特定设置
# 如果是Android或Linux系统，则添加-fPIC标志以生成位置无关代码
if(CMAKE_SYSTEM_NAME MATCHES "^Android" OR CMAKE_SYSTEM_NAME MATCHES "^Linux")
    add_definitions(-fPIC)
endif()

# 树莓派32位系统修复
# 如果是Linux系统且处理器是armv7架构，则添加特定的编译器标志
if(CMAKE_SYSTEM_NAME MATCHES "^Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^armv7")
    add_definitions(-march=armv7-a -mfpu=neon-vfpv4)
endif()

# Android平台特定设置
# 如果是Android系统
if(CMAKE_SYSTEM_NAME MATCHES "^Android")
    # 添加Android构建定义
    add_definitions(-DMNN_BUILD_FOR_ANDROID)
    # 如果处理器是ARM架构，则添加浮点ABI和FPU设置
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
        add_definitions(-mfloat-abi=softfp -mfpu=neon)
    endif()
endif()

# C++11支持选项
option(MNN_USE_CPP11 "Enable MNN use c++11" ON)

# C++标准设置（非MSVC编译器）
if (NOT MSVC)
    # 如果启用了CUDA且支持变压器融合，或者C++标准是C++17
    if((MNN_CUDA AND MNN_SUPPORT_TRANSFORMER_FUSE) OR (CMAKE_CXX_STANDARD EQUAL 17))
        # 设置C++标准为C++17
        set(CMAKE_CXX_STANDARD 17)
        # 设置C标准为GNU99
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
        # 设置C++标准标志为C++17
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    # 如果启用了C++11
    elseif(MNN_USE_CPP11)
        # 设置C标准为GNU99
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
        # 设置C++标准标志为C++11
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    # 其他情况
    else()
        # 设置C标准为GNU99
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
        # 设置C++标准标志为C++0x
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    endif()
endif()

# Linux平台特定设置
# 如果是Linux系统
if(CMAKE_SYSTEM_NAME MATCHES "^Linux")
    # 添加严格ANSI标准定义
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STRICT_ANSI__")
    # 如果处理器是armv7架构，则添加NEON FPU支持
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^armv7")
        add_definitions(-mfpu=neon)    #请在project/cross-compile/arm.toolchain.cmake中定义
    endif()
    # 如果启用了硬浮点ABI构建，则添加硬浮点ABI定义
    if(MNN_BUILD_HARD)
        add_definitions(-mfloat-abi=hard)  #最好在project/cross-compile/arm.toolchain.cmake中定义
    endif()
endif()

# 内存调试设置
# 如果启用了内存调试，则添加地址消毒器标志
IF(MNN_DEBUG_MEMORY)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

# 依赖项变量初始化
# MNN依赖项列表
set(MNN_DEPS "")
# MNN额外依赖项列表
set(MNN_EXTRA_DEPENDS "")

# 调试和发布构建配置
# 如果构建类型是Debug
IF(CMAKE_BUILD_TYPE MATCHES Debug)
    # 添加调试定义
    add_definitions(-DMNN_DEBUG -DDEBUG)
    # MSVC编译器调试设置
    if(MSVC)
        # 添加调试信息标志
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DEBUG")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEBUG")
    # 非MSVC编译器调试设置
    else()
        # 添加调试信息标志
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    endif()
# 如果构建类型不是Debug（即发布版本）
else()
    # MSVC编译器优化设置
    if (MSVC)
        # 添加优化标志
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2")
    # 非MSVC编译器优化设置
    else()
        # 添加优化标志
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
        # Android平台优化设置
        if(CMAKE_SYSTEM_NAME MATCHES "^Android")
            # 如果通过命令行构建Android版本
            if(MNN_BUILD_FOR_ANDROID_COMMAND)
                # 添加链接器优化标志：剥离符号、位置无关可执行文件、垃圾回收段
                set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
                set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -fPIE -s")
                set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
            endif()
        endif()
    endif()
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

# 鸿蒙系统特定设置
# 如果是鸿蒙系统
if(OHOS)
    # 如果使用Logcat
    IF(MNN_USE_LOGCAT)
        # 添加Logcat定义
        add_definitions(-DMNN_USE_LOGCAT)
        # 禁用格式安全警告
        add_definitions(-Wno-format-security)
        # 添加鸿蒙日志库依赖
        list(APPEND MNN_EXTRA_DEPENDS libhilog_ndk.z.so)
    ENDIF()
endif()

# Android平台特定设置
# 如果是Android系统
if(CMAKE_SYSTEM_NAME MATCHES "^Android")
    # 如果使用Logcat
    IF(MNN_USE_LOGCAT)
        # 添加Logcat定义
        add_definitions(-DMNN_USE_LOGCAT)
    ENDIF()
    # 如果不是通过命令行构建Android版本
    IF (NOT MNN_BUILD_FOR_ANDROID_COMMAND)
        # 设置库输出目录
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${NATIVE_LIBRARY_OUTPUT}/${ANDROID_ABI})
    ENDIF()
endif()

# Linux ARM架构包含目录设置
# 如果是Linux系统且处理器是ARM或AArch64架构
if(${CMAKE_SYSTEM_NAME} MATCHES "^Linux")
    if((CMAKE_SYSTEM_PROCESSOR MATCHES "^arm") OR (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64"))
        # 设置ARM64 Linux包含目录（注释掉的路径仅供参考）
        set(aarch64_linux_include
            #/usr/include/c++/4.9
            #/usr/lib/gcc/x86_64-linux-gnu/4.9
            #/usr/lib/gcc/x86_64-linux-gnu/4.9/include
            #/usr/include/x86_64-linux-gnu/c++/4.9
        )
        # 添加包含目录
        include_directories(${aarch64_linux_include})
    endif()
endif()

# 添加项目包含目录
# 包含MNN核心头文件目录
include_directories(${CMAKE_CURRENT_LIST_DIR}/include/
                    # 包含MNN源码目录
                    ${CMAKE_CURRENT_LIST_DIR}/source/
                    # 包含表达式API目录
                    ${CMAKE_CURRENT_LIST_DIR}/express/
                    # 包含工具目录
                    ${CMAKE_CURRENT_LIST_DIR}/tools/
                    # 包含代码生成目录
                    ${CMAKE_CURRENT_LIST_DIR}/codegen/
                    # 包含Schema目录
                    ${CMAKE_CURRENT_LIST_DIR}/schema/current/
                    # 包含第三方库目录
                    ${CMAKE_CURRENT_LIST_DIR}/3rd_party/
                    # 包含FlatBuffers头文件目录
                    ${CMAKE_CURRENT_LIST_DIR}/3rd_party/flatbuffers/include
                    # 包含Half浮点数库目录
                    ${CMAKE_CURRENT_LIST_DIR}/3rd_party/half
                    # 包含图像处理助手目录
                    ${CMAKE_CURRENT_LIST_DIR}/3rd_party/imageHelper
                    # 包含OpenCL头文件目录
                    ${CMAKE_CURRENT_LIST_DIR}/3rd_party/OpenCLHeaders/
                  )

# 构建MNN包含目录列表
# 将所有包含目录添加到MNN_INCLUDES变量中
list(APPEND MNN_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}/include/
        ${CMAKE_CURRENT_LIST_DIR}/source/
        ${CMAKE_CURRENT_LIST_DIR}/express/
        ${CMAKE_CURRENT_LIST_DIR}/tools/
        ${CMAKE_CURRENT_LIST_DIR}/codegen/
        ${CMAKE_CURRENT_LIST_DIR}/schema/current/
        ${CMAKE_CURRENT_LIST_DIR}/3rd_party/
        ${CMAKE_CURRENT_LIST_DIR}/3rd_party/flatbuffers/include
        ${CMAKE_CURRENT_LIST_DIR}/3rd_party/half
        ${CMAKE_CURRENT_LIST_DIR}/3rd_party/imageHelper
        ${CMAKE_CURRENT_LIST_DIR}/3rd_party/OpenCLHeaders/
)

# 添加MNN包含目录
include_directories(${MNN_INCLUDES})

# 初始化链接对象和目标列表
# MNN对象链接列表
set(MNN_OBJECTS_TO_LINK "")
# MNN目标列表
set(MNN_TARGETS "")

# 核心模块构建
# 查找核心源文件
FILE(GLOB MNN_Core_SRC ${CMAKE_CURRENT_LIST_DIR}/source/core/*)
# 创建核心对象库
add_library(MNNCore OBJECT ${MNN_Core_SRC})
# 将核心对象添加到链接列表
list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNNCore>)
# 将核心目标添加到目标列表
list(APPEND MNN_TARGETS MNNCore)
# 如果跳过构建几何模块，则为核心模块添加编译选项
if(MNN_SKIPBUILD_GEOMETRY)
    target_compile_options(MNNCore PRIVATE -DMNN_SKIPBUILD_GEOMETRY)
endif()

# 计算机视觉模块构建
# 查找CV源文件
FILE(GLOB MNN_CV_SRC ${CMAKE_CURRENT_LIST_DIR}/source/cv/*)
# 创建CV对象库
add_library(MNNCV OBJECT ${MNN_CV_SRC})
# 将CV对象添加到链接列表
list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNNCV>)
# 将CV目标添加到目标列表
list(APPEND MNN_TARGETS MNNCV)
# x86架构特定设置
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64)|(X86_64)|(x64)|(X64)|(amd64)|(AMD64)|(i686)")
    # Apple平台特定设置
    if (APPLE)
        # 添加定义以禁用堆栈检查（解决Xcode 11.X的bug）
        add_definitions(-fno-stack-check)
    endif()
endif()

# 数学模块构建
# 查找数学源文件
FILE(GLOB MNN_Math_SRC ${CMAKE_CURRENT_LIST_DIR}/source/math/*)
# 创建数学对象库
add_library(MNNMath OBJECT ${MNN_Math_SRC})
# 将数学对象添加到链接列表
list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNNMath>)
# 将数学目标添加到目标列表
list(APPEND MNN_TARGETS MNNMath)

# 变换模块构建
# 如果不跳过构建几何模块
IF (NOT MNN_SKIPBUILD_GEOMETRY)
    # 递归查找变换源文件（形状和几何）
    FILE(GLOB_RECURSE MNN_Transform_SRC ${CMAKE_CURRENT_LIST_DIR}/source/shape/* ${CMAKE_CURRENT_LIST_DIR}/source/geometry/*)
    # 创建变换对象库
    add_library(MNNTransform OBJECT ${MNN_Transform_SRC})
    # 将变换对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNNTransform>)
    # 将变换目标添加到目标列表
    list(APPEND MNN_TARGETS MNNTransform)
ENDIF()

# 工具模块构建
# 查找工具源文件
FILE(GLOB MNN_Utils_SRC ${CMAKE_CURRENT_LIST_DIR}/source/utils/*)
# 创建工具对象库
add_library(MNNUtils OBJECT ${MNN_Utils_SRC})
# 将工具对象添加到链接列表
list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNNUtils>)
# 将工具目标添加到目标列表
list(APPEND MNN_TARGETS MNNUtils)

# 包含CPU后端CMakeLists
include(${CMAKE_CURRENT_LIST_DIR}/source/backend/cpu/CMakeLists.txt)


# 公共头文件列表初始化
# MNN公共头文件列表
SET(MNN_PUB_HDRS "")
# MNN表达式API公共头文件列表
SET(MNN_EXPR_PUB_HDRS "")
# MNN额外头文件列表
set(MNN_EXTRA_HEADERS "")

# 添加MNN核心公共头文件
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/MNNDefine.h")
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/Interpreter.hpp")
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/HalideRuntime.h")
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/Tensor.hpp")
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/ErrorCode.hpp")
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/ImageProcess.hpp")
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/Matrix.h")
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/Rect.h")
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/MNNForwardType.h")
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/AutoTime.hpp")
list(APPEND MNN_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/MNNSharedContext.h")

# 添加MNN表达式API公共头文件
list(APPEND MNN_EXPR_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/expr/Expr.hpp")
list(APPEND MNN_EXPR_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/expr/ExprCreator.hpp")
list(APPEND MNN_EXPR_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/expr/MathOp.hpp")
list(APPEND MNN_EXPR_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/expr/NeuralNetWorkOp.hpp")
list(APPEND MNN_EXPR_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/expr/Optimizer.hpp")
list(APPEND MNN_EXPR_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/expr/Executor.hpp")
list(APPEND MNN_EXPR_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/expr/Module.hpp")
list(APPEND MNN_EXPR_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/expr/NeuralNetWorkOp.hpp")
list(APPEND MNN_EXPR_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/expr/ExecutorScope.hpp")
list(APPEND MNN_EXPR_PUB_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/include/MNN/expr/Scope.hpp")

# 添加额外头文件
# 如果构建OpenCV支持
IF(MNN_BUILD_OPENCV)
  # 查找OpenCV头文件
  file(GLOB MNN_CV_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/tools/cv/include/cv/*.hpp PARENT_SCOPE)
  file(GLOB MNN_CV_IMGHDRS ${CMAKE_CURRENT_SOURCE_DIR}/tools/cv/include/cv/imgproc/*.hpp PARENT_SCOPE)
  # 将OpenCV头文件添加到额外头文件列表
  list(APPEND MNN_EXTRA_HEADERS ${MNN_CV_HDRS})
  list(APPEND MNN_EXTRA_HEADERS ${MNN_CV_IMGHDRS})
ENDIF()

# 如果构建音频支持
IF(MNN_BUILD_AUDIO)
  # 查找音频头文件
  file(GLOB MNN_AUDIO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/tools/audio/include/audio/*.hpp PARENT_SCOPE)
  # 将音频头文件添加到额外头文件列表
  list(APPEND MNN_EXTRA_HEADERS ${MNN_AUDIO_HDRS})
ENDIF()

# 如果构建大语言模型支持
IF(MNN_BUILD_LLM)
  # 查找LLM头文件
  file(GLOB MNN_LLM_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/transformers/llm/engine/include/llm/*)
  # 将LLM头文件添加到额外头文件列表
  list(APPEND MNN_EXTRA_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/transformers/llm/engine/include/llm/llm.hpp)
ENDIF()

# 如果构建扩散模型支持
IF(MNN_BUILD_DIFFUSION)
  # 查找扩散模型头文件
  file(GLOB MNN_DIFFUSION_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/transformers/diffusion/engine/include/diffusion/*)
  # 将扩散模型头文件添加到额外头文件列表
  list(APPEND MNN_EXTRA_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/transformers/diffusion/engine/include/diffusion/diffusion.hpp)
ENDIF()



# 添加线程依赖
# 查找线程包
find_package(Threads)
# 将线程库添加到额外依赖列表
list(APPEND MNN_EXTRA_DEPENDS ${CMAKE_THREAD_LIBS_INIT})

# Windows平台特定链接设置
if(WIN32)
  # 如果不是MSVC编译器
  if(NOT MSVC)
    # 设置共享库链接器标志
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld-link -lmsvcrt")
    # 设置可执行文件链接器标志
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld-link -lmsvcrt")
  # 如果是MSVC编译器
  else()
    # 设置可执行文件链接器标志，增加栈大小
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608")
  endif()
endif()

# 非Apple平台OpenMP设置
if (NOT APPLE)
  # 如果启用OpenMP
  if(MNN_OPENMP)
      # 输出状态信息：检查OpenMP
      message(STATUS "[*] Checking OpenMP")
      # 查找OpenMP包
      find_package(OpenMP)
      # 对于CMake < 3.9版本，需要手动创建目标
      if(NOT TARGET OpenMP::OpenMP_CXX)
          # 添加OpenMP导入接口库
          add_library(OpenMP::OpenMP_CXX IMPORTED INTERFACE)
          # 设置目标属性：接口编译选项
          set_property(TARGET OpenMP::OpenMP_CXX
              PROPERTY INTERFACE_COMPILE_OPTIONS ${OpenMP_CXX_FLAGS})
          # 设置目标属性：接口链接库
          # 仅当相同标志传递给链接器时有效；否则使用CMake 3.9+（Intel, AppleClang）
          set_property(TARGET OpenMP::OpenMP_CXX
              PROPERTY INTERFACE_LINK_LIBRARIES ${OpenMP_CXX_FLAGS} Threads::Threads)
      endif()
      # TODO: 不要污染全局CFLAGS
      # 设置C和C++编译标志
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
      # 设置链接器标志
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OpenMP_SHARED_LINKER_FLAGS}")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
      # MSVC编译器OpenMP标志设置
      if (MSVC)
          set(OpenMP_C_FLAGS "/openmp ${OpenMP_C_FLAGS}")
          set(OpenMP_CXX_FLAGS "/openmp ${OpenMP_CXX_FLAGS}")
      endif()
      # 将OpenMP库添加到额外依赖列表
      list(APPEND MNN_EXTRA_DEPENDS OpenMP::OpenMP_CXX)
    endif()
endif()

# 隐藏符号设置（非MSVC且启用隐藏符号）
if ((NOT MSVC) AND MNN_HIDDEN)
    # 设置C++编译标志：隐藏内联函数和符号
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden -fvisibility=hidden")
    # 设置C编译标志：隐藏符号
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    # 设置汇编编译标志：隐藏符号
    set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -fvisibility=hidden")
    # 省略帧指针可能导致调试困难
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer")
endif()

# 编译器特定设置（非MSVC）
if (NOT MSVC)
    # 设置C++编译标志：严格别名、函数段、数据段、禁用RTTI、禁用异常
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstrict-aliasing -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions ")
    # 设置C编译标志：严格别名、函数段、数据段
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstrict-aliasing -ffunction-sections -fdata-sections ")
# MSVC编译器设置
else()
    # 设置C++编译标志：精确浮点
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise")
    # 设置C编译标志：精确浮点
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fp:precise")
endif()

# Metal依赖项
# 将MNN添加到依赖项列表
list(APPEND MNN_DEPS MNN)

# 插件支持
# 如果启用插件支持
if(MNN_WITH_PLUGIN)
    # 添加插件定义
    add_definitions(-DMNN_WITH_PLUGIN)
    # 包含插件CMakeLists
    include(${CMAKE_CURRENT_LIST_DIR}/source/plugin/CMakeLists.txt)
endif()

# Metal后端（Apple平台）
# 如果启用Metal且是Apple平台
if(MNN_METAL AND APPLE)
    # 为核心模块添加Metal启用编译选项
    target_compile_options(MNNCore PRIVATE -DMNN_METAL_ENABLED=1)
    # 包含Metal后端CMakeLists
    include(${CMAKE_CURRENT_LIST_DIR}/source/backend/metal/CMakeLists.txt)
    # 将Metal目标添加到目标列表
    list(APPEND MNN_TARGETS MNNMetal)
    # 将Metal对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNNMetal>)
endif()

# CoreML后端
# 如果启用CoreML
IF(MNN_COREML)
    # 添加CoreML启用定义
    add_definitions(-DMNN_COREML_ENABLED=1)
    # 包含CoreML后端CMakeLists
    include(${CMAKE_CURRENT_LIST_DIR}/source/backend/coreml/CMakeLists.txt)

    # 将CoreML目标添加到目标列表
    list(APPEND MNN_TARGETS MNNCoreML)
    # 将CoreML对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNNCoreML>)

    # 查找CoreML相关系统库
    find_library(COREML CoreML)
    find_library(FOUNDATION Foundation)
    find_library(METAL Metal)
    find_library(VIDEO CoreVideo)
    # 将系统库添加到额外依赖列表
    list(APPEND MNN_EXTRA_DEPENDS ${COREML})
    list(APPEND MNN_EXTRA_DEPENDS ${FOUNDATION})
    list(APPEND MNN_EXTRA_DEPENDS ${METAL})
    list(APPEND MNN_EXTRA_DEPENDS ${VIDEO})
ENDIF()

# NNAPI后端
# 如果启用NNAPI
IF(MNN_NNAPI)
    # 添加NNAPI启用定义
    add_definitions(-DMNN_NNAPI_ENABLED=1)
    # 添加NNAPI后端子目录
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/backend/nnapi/)
    # 将NNAPI对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNN_NNAPI>)
ENDIF()

# QNN后端
# 如果启用QNN
IF(MNN_QNN)
    # 添加QNN启用定义
    add_definitions(-DMNN_QNN_ENABLED=1)
    # 添加QNN后端子目录
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/backend/qnn/)
    # 将QNN对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNN_QNN>)
ENDIF()

# Vulkan后端
# 如果启用Vulkan
IF(MNN_VULKAN)
  # 添加Vulkan后端子目录
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/backend/vulkan/)
  # 如果启用分离构建
  IF(MNN_SEP_BUILD)
    # 将Vulkan依赖添加到依赖列表
    list(APPEND MNN_DEPS MNN_Vulkan)
  # 如果不启用分离构建
  ELSE()
    # 将Vulkan目标添加到目标列表
    list(APPEND MNN_TARGETS MNN_Vulkan)
    # 将Vulkan对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNN_Vulkan>)
    # 将Vulkan库添加到额外依赖列表
    list(APPEND MNN_EXTRA_DEPENDS ${MNN_VULKAN_LIBS})
  ENDIF()
ENDIF()

# oneDNN后端
# 如果启用oneDNN
IF(MNN_ONEDNN)
    # 为CPU后端添加oneDNN使用定义
    target_compile_definitions(MNNCPU PRIVATE "-DMNN_USE_ONEDNN")
    # 为CPU后端添加oneDNN依赖
    add_dependencies(MNNCPU oneDNN)
    # 包含oneDNN配置
    include(cmake/oneDNN.cmake)
    # 设置oneDNN目录
    set(ONEDNN_DIR ${CMAKE_CURRENT_LIST_DIR}/3rd_party/oneDNN)
    # 创建oneDNN通用对象库
    add_library(ONEDNN_COMMON OBJECT IMPORTED)
    # 查找通用对象文件
    file(GLOB_RECURSE OBJECT_FILES ${ONEDNN_DIR}/src/common/CMakeFiles/dnnl_common.dir/*.o)
    # 设置导入对象属性
    set_property(TARGET ONEDNN_COMMON PROPERTY IMPORTED_OBJECTS ${OBJECT_FILES})
    # 创建oneDNN CPU对象库
    add_library(ONEDNN_CPU OBJECT IMPORTED)
    # 查找CPU对象文件
    file(GLOB_RECURSE OBJECT_FILES ${ONEDNN_DIR}/src/cpu/CMakeFiles/dnnl_cpu.dir/*.o)
    # 设置导入对象属性
    set_property(TARGET ONEDNN_CPU PROPERTY IMPORTED_OBJECTS ${OBJECT_FILES})
    # 创建oneDNN CPU x64对象库
    add_library(ONEDNN_CPU_X64 OBJECT IMPORTED)
    # 查找CPU x64对象文件
    file(GLOB_RECURSE OBJECT_FILES ${ONEDNN_DIR}/src/cpu/x64/CMakeFiles/dnnl_cpu_x64.dir/*.o)
    # 设置导入对象属性
    set_property(TARGET ONEDNN_CPU_X64 PROPERTY IMPORTED_OBJECTS ${OBJECT_FILES})
    # 添加oneDNN包含目录
    include_directories(${ONEDNN_DIR}/include)
    # 将oneDNN目标添加到目标列表
    list(APPEND MNN_TARGETS ${ONEDNN_COMMON})
    list(APPEND MNN_TARGETS ${ONEDNN_CPU})
    list(APPEND MNN_TARGETS ${ONEDNN_CPU_X64})
    # 将oneDNN对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:ONEDNN_COMMON>)
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:ONEDNN_CPU>)
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:ONEDNN_CPU_X64>)
ENDIF()

# OpenCL后端
# 如果启用OpenCL
IF(MNN_OPENCL)
  # 添加OpenCL后端子目录
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/backend/opencl/)
  # 如果启用分离构建
  IF(MNN_SEP_BUILD)
    # 将OpenCL依赖添加到依赖列表
    list(APPEND MNN_DEPS MNN_CL)
  # 如果不启用分离构建
  ELSE()
    # 添加OpenCL启用定义
    add_definitions(-DMNN_OPENCL_ENABLED=1)
    # 将OpenCL目标添加到目标列表
    list(APPEND MNN_TARGETS MNN_CL)
    # 将OpenCL对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNN_CL>)
    # 将OpenCL库添加到额外依赖列表
    list(APPEND MNN_EXTRA_DEPENDS ${MNN_OCL_LIBS})
  ENDIF()
ENDIF()

# OpenGL后端
# 如果启用OpenGL
IF(MNN_OPENGL)
  # 添加OpenGL后端子目录
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/backend/opengl/)
  # 如果启用分离构建
  IF(MNN_SEP_BUILD)
    # 将OpenGL依赖添加到依赖列表
    list(APPEND MNN_DEPS MNN_GL)
  # 如果不启用分离构建
  ELSE()
    # 将OpenGL目标添加到目标列表
    list(APPEND MNN_TARGETS MNN_GL)
    # 将OpenGL对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNN_GL>)
    # 将OpenGL系统库添加到额外依赖列表
    list(APPEND MNN_EXTRA_DEPENDS GLESv3)
    list(APPEND MNN_EXTRA_DEPENDS EGL)
  ENDIF()
ENDIF()

# CUDA后端
# 如果启用CUDA
IF(MNN_CUDA)
  # 添加CUDA后端子目录
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/backend/cuda/)
  # 将CUDA目标添加到目标列表
  list(APPEND MNN_TARGETS MNN_CUDA)
  # 如果不是MSVC编译器
  if (NOT MSVC)
    # 将CUDA对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNN_CUDA>)
  endif()
  # 将CUDA库添加到额外依赖列表
  list(APPEND MNN_EXTRA_DEPENDS ${MNN_CUDA_LIBS})
ENDIF()

# 表达式API
# 如果不跳过构建几何模块
if(NOT MNN_SKIPBUILD_GEOMETRY)
    # 添加表达式API子目录
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/express/)
    # 如果启用分离构建
    IF(MNN_SEP_BUILD)
      # 将表达式API依赖添加到依赖列表
      list(APPEND MNN_DEPS MNN_Express)
    # 如果不启用分离构建
    ELSE()
       # 将表达式API目标添加到目标列表
       list(APPEND MNN_TARGETS MNN_Express)
       # 将表达式API对象添加到链接列表
       list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNN_Express>)
    ENDIF()
endif()

# 模型内部功能。启用MNN内部功能，如模型认证和指标日志记录。
# 如果启用内部功能且不是鸿蒙系统（TODO: 支持鸿蒙系统日志）
if (MNN_INTERNAL AND NOT OHOS)
    # 为核心模块添加内部功能启用编译选项
    target_compile_options(MNNCore PRIVATE -DMNN_INTERNAL_ENABLED)
    # 为表达式模块添加内部功能启用编译选项
    target_compile_options(MNN_Express PRIVATE -DMNN_INTERNAL_ENABLED)
    # 包含内部日志记录CMakeLists
    include(${CMAKE_CURRENT_LIST_DIR}/source/internal/logging/CMakeLists.txt)
    # 如果是Linux系统
    if(CMAKE_SYSTEM_NAME MATCHES "^Linux")
        # 将curl、ssl和crypto库添加到额外依赖列表
        list(APPEND MNN_EXTRA_DEPENDS "-lcurl -lssl -lcrypto")
    endif()
endif()

# 训练框架
# 如果构建训练框架或量化工具
IF(MNN_BUILD_TRAIN OR MNN_BUILD_QUANTOOLS)
  # 添加训练工具子目录
  add_subdirectory(tools/train)
  # 如果启用分离构建
  IF(MNN_SEP_BUILD)
    # 将训练依赖添加到依赖列表
    list(APPEND MNN_DEPS MNNTrain)
    list(APPEND MNN_DEPS MNNTrainUtils)
  # 如果不启用分离构建
  ELSE()
    # 将训练目标添加到目标列表
    list(APPEND MNN_TARGETS MNNTrain)
    list(APPEND MNN_TARGETS MNNTrainUtils)
    # 将训练对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNNTrain>)
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNNTrainUtils>)
  ENDIF()
ENDIF()

# 代码生成器
# 如果构建代码生成器
IF(MNN_BUILD_CODEGEN)
    # 添加代码生成器定义
    add_definitions(-DMNN_BUILD_CODEGEN)
    # 包含代码生成器CMakeLists
    include(${CMAKE_CURRENT_LIST_DIR}/codegen/CMakeLists.txt)
ENDIF()

# NPU后端
# 如果启用NPU
IF(MNN_NPU)
    # 如果是Android系统
    if (CMAKE_SYSTEM_NAME MATCHES "^Android")
        # 设置HIAI路径为Android ABI
        set(HIAI_PATH ${ANDROID_ABI})
    endif()
    # 如果是鸿蒙系统
    if (OHOS)
        # 设置HIAI路径为鸿蒙架构
        set(HIAI_PATH ${OHOS_ARCH})
    endif()
    # 添加HIAI后端子目录
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/backend/hiai/)
    # 如果启用分离构建
    IF(MNN_SEP_BUILD)
        # 将NPU依赖添加到依赖列表
        list(APPEND MNN_DEPS MNN_NPU)
    # 如果不启用分离构建
    ELSE()
        # 将NPU目标添加到目标列表
        list(APPEND MNN_TARGETS MNN_NPU)
        # 将NPU对象添加到链接列表
        list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNN_NPU>)
        # 将HIAI库添加到额外依赖列表
        list(APPEND MNN_EXTRA_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/source/backend/hiai/3rdParty/${HIAI_PATH}/libhiai.so)
        list(APPEND MNN_EXTRA_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/source/backend/hiai/3rdParty/${HIAI_PATH}/libhiai_ir_build.so)
        list(APPEND MNN_EXTRA_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/source/backend/hiai/3rdParty/${HIAI_PATH}/libhiai_ir.so)
    ENDIF()
ENDIF()

# TensorRT后端
# 如果启用TensorRT
IF(MNN_TENSORRT)
  # 添加TensorRT后端子目录
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/backend/tensorrt/)
  # 将TensorRT目标添加到目标列表
  list(APPEND MNN_TARGETS MNN_TRT)
  # 将TensorRT对象添加到链接列表
  list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNN_TRT>)
  # 将TensorRT库添加到额外依赖列表
  list(APPEND MNN_EXTRA_DEPENDS ${MNN_TRT_LIBS})
ENDIF()

# OpenCV工具
# 如果构建OpenCV支持
IF(MNN_BUILD_OPENCV)
  # 添加OpenCV工具子目录
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/cv)
  # 如果启用分离构建
  IF(MNN_SEP_BUILD)
    # 将OpenCV依赖添加到依赖列表
    list(APPEND MNN_DEPS MNNOpenCV)
  # 如果不启用分离构建
  ELSE()
    # 将OpenCV目标添加到目标列表
    list(APPEND MNN_TARGETS MNNOpenCV)
    # 将OpenCV对象添加到链接列表
    list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:MNNOpenCV>)
  ENDIF()
ENDIF()

# 大语言模型
# 如果构建大语言模型
IF(MNN_BUILD_LLM)
    # 添加大语言模型定义（已注释）
    # add_definitions(-DMNN_BUILD_LLM)
    # 包含大语言模型引擎CMakeLists
    include(${CMAKE_CURRENT_LIST_DIR}/transformers/llm/engine/CMakeLists.txt)
    # 如果不启用分离构建
    IF(NOT MNN_SEP_BUILD)
      # 将LLM目标添加到目标列表
      list(APPEND MNN_TARGETS llm)
      # 将LLM对象添加到链接列表
      list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:llm>)
    ENDIF()
ENDIF()

# 扩散模型
# 如果构建扩散模型且构建OpenCV支持且启用图像编解码器
IF(MNN_BUILD_DIFFUSION AND MNN_BUILD_OPENCV AND MNN_IMGCODECS)
    # 包含扩散模型引擎CMakeLists
    include(${CMAKE_CURRENT_LIST_DIR}/transformers/diffusion/engine/CMakeLists.txt)
    # 如果不启用分离构建
    IF(NOT MNN_SEP_BUILD)
      # 将扩散模型目标添加到目标列表
      list(APPEND MNN_TARGETS diffusion)
      # 将扩散模型对象添加到链接列表
      list(APPEND MNN_OBJECTS_TO_LINK $<TARGET_OBJECTS:diffusion>)
    ENDIF()
ENDIF()

# MNN库构建
# 如果启用分离构建
IF(MNN_SEP_BUILD)
  # 创建共享MNN库
  add_library(MNN SHARED ${CMAKE_CURRENT_LIST_DIR}/cmake/dummy.cpp ${MNN_OBJECTS_TO_LINK} ${MNN_PUB_HDRS} ${MNN_EXPR_PUB_HDRS} ${MNN_EXTRA_HEADERS})
  # 链接额外依赖
  target_link_libraries(MNN PUBLIC ${MNN_EXTRA_DEPENDS})
# 如果不启用分离构建
ELSE()
  # 如果构建共享库
  IF(MNN_BUILD_SHARED_LIBS)
    # 创建共享MNN库
    add_library(MNN SHARED ${CMAKE_CURRENT_LIST_DIR}/cmake/dummy.cpp ${MNN_OBJECTS_TO_LINK} ${MNN_PUB_HDRS} ${MNN_EXPR_PUB_HDRS} ${MNN_EXTRA_HEADERS})
    # Windows平台特定设置
    if (WIN32)
      # 为所有目标添加DLL编译定义
      foreach(TARGET ${MNN_TARGETS})
        target_compile_definitions(${TARGET} PRIVATE "-DBUILDING_MNN_DLL")
        target_compile_definitions(${TARGET} INTERFACE "-DUSING_MNN_DLL")
      endforeach()
      # 为MNN库添加DLL编译定义
      target_compile_definitions(MNN PRIVATE "-DBUILDING_MNN_DLL")
      target_compile_definitions(MNN INTERFACE "-DUSING_MNN_DLL")
    endif()
  # 如果构建静态库
  ELSE()
    # 创建静态MNN库
    add_library(MNN STATIC ${CMAKE_CURRENT_LIST_DIR}/cmake/dummy.cpp ${MNN_OBJECTS_TO_LINK} ${MNN_PUB_HDRS} ${MNN_EXPR_PUB_HDRS} ${MNN_EXTRA_HEADERS})
  ENDIF()
  # 链接额外依赖
  target_link_libraries(MNN PUBLIC ${MNN_EXTRA_DEPENDS})
ENDIF()

# MSVC特定链接选项
if (MSVC)
  # 忽略特定链接警告
  target_link_options(MNN PRIVATE "/IGNORE:4049,4217")
  # 如果启用CUDA
  if (MNN_CUDA)
    # 如果构建共享库
    if (MNN_BUILD_SHARED_LIBS)
      # 添加wholearchive链接选项
      target_link_options(MNN PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:MNN_CUDA>")
    # 如果构建静态库
    else()
      # 添加自定义命令合并CUDA库
      add_custom_command(
        TARGET MNN
        POST_BUILD
        COMMAND lib.exe ARGS /OUT:$<TARGET_FILE:MNN> $<TARGET_FILE:MNN> $<TARGET_FILE:MNN_CUDA>
      )
    endif()
  endif()
endif()

# oneDNN依赖
if (MNN_ONEDNN)
    # 为MNN添加oneDNN依赖
    add_dependencies(MNN ONEDNN_COMMON ONEDNN_CPU ONEDNN_CPU_X64)
endif()

# Apple平台特定设置
if(APPLE)
    # 如果构建Apple框架
    IF(MNN_AAPL_FMWK)
      # 设置MNN为目标框架
      set_target_properties(MNN PROPERTIES FRAMEWORK TRUE)
      # 设置框架属性
      set_target_properties(MNN PROPERTIES
          MACOSX_FRAMEWORK_IDENTIFIER com.alibaba.MNN
          MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${MNN_VERSION}
          MACOSX_FRAMEWORK_BUNDLE_VERSION ${MNN_VERSION}
          XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
      )
    ENDIF()
    # 如果启用Metal
    IF(MNN_METAL)
      # 查找系统库并链接到MNN
      find_library(FOUNDATION Foundation REQUIRED)
      target_link_libraries(MNN PUBLIC ${FOUNDATION})
      find_library(METAL Metal REQUIRED)
      target_link_libraries(MNN PUBLIC ${METAL})
      find_library(GRAPHIC CoreGraphics)
      target_link_libraries(MNN PUBLIC ${GRAPHIC})
    ENDIF()
endif()

# 转换器工具
if (NOT MNN_SKIPBUILD_GEOMETRY)
    # 添加转换器子目录
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/converter)
endif()

# Windows转换器依赖
IF(WIN32 AND MNN_BUILD_CONVERTER AND MNN_BUILD_SHARED_LIBS)
  # 由于dllimport/dllexport，我们将MNN和MNNConvertDeps合并，它们依赖protobuf
  target_link_libraries(MNN PUBLIC ${Protobuf_LIBRARIES})
ENDIF()

# 在Windows上将MNN/MNNExpress/MNNOpenCV和其他后端合并到一个.lib/.dll中

# 音频工具
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/audio)
# 如果构建音频支持且不启用分离构建
IF(MNN_BUILD_AUDIO AND NOT MNN_SEP_BUILD)
  # MSVC特定设置
  IF(MSVC)
    # 为音频库添加DLL编译定义
    target_compile_definitions(MNNAudio PRIVATE "-DBUILDING_MNN_DLL" INTERFACE "-DUSING_MNN_DLL")
  ENDIF()
  # 将音频对象添加到MNN源文件
  target_sources(MNN PRIVATE $<TARGET_OBJECTS:MNNAudio>)
ENDIF()


# Linux平台链接设置
if(CMAKE_SYSTEM_NAME MATCHES "^Linux")
  # 使用-pthread而不是-lpthread，因为线程安全的glibc实现需要
  # 参考: https://stackoverflow.com/questions/23250863/difference-between-pthread-and-lpthread-while-compiling
  target_link_libraries(MNN PUBLIC -pthread dl)
# Android平台链接设置
elseif(CMAKE_SYSTEM_NAME MATCHES "^Android")
  # 链接Android日志库和数学库
  target_link_libraries(MNN PUBLIC log m)
# 其他平台
else()
endif()

# 静态库构建设置
if (NOT MNN_BUILD_SHARED_LIBS)
    # 如果使用GNU或Clang编译器
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # 静态链接不会将glibc中的线程相关弱符号替换为pthread库中的强符号，
        # 因此我们需要使用--whole-archive来链接pthread
        # 参考: https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why
        # 如果是Linux系统
        if(CMAKE_SYSTEM_NAME MATCHES "^Linux")
            # 设置MNN依赖项，使用whole-archive链接pthread
            set(MNN_DEPS -Wl,--whole-archive ${MNN_DEPS} -lpthread -Wl,--no-whole-archive)
        else()
          # 如果是Apple平台
          if(APPLE)
            # 设置MNN依赖项，使用force_load
            set(MNN_DEPS -Wl,-force_load ${MNN_DEPS})
          # 其他平台
          else()
            # 设置MNN依赖项，使用whole-archive
            set(MNN_DEPS -Wl,--whole-archive ${MNN_DEPS} -Wl,--no-whole-archive)
          endif()
        endif()
    endif()
endif()

# 更新MNN目标列表
list(APPEND MNN_TARGETS MNN)
list(REMOVE_ITEM MNN_TARGETS MNN)

# 演示程序构建
IF(MNN_BUILD_DEMO)
  # 包含演示程序CMakeLists
  include(${CMAKE_CURRENT_LIST_DIR}/demo/exec/CMakeLists.txt)
ENDIF()

# 工具程序构建
IF(MNN_BUILD_TOOLS)
  # 包含工具程序CMakeLists
  include(${CMAKE_CURRENT_LIST_DIR}/tools/cpp/CMakeLists.txt)
ENDIF()

# 测试程序构建
IF(MNN_BUILD_TEST)
  # 包含测试程序CMakeLists
  include(${CMAKE_CURRENT_LIST_DIR}/test/CMakeLists.txt)
ENDIF()

# 性能基准测试构建
IF(MNN_BUILD_BENCHMARK)
  # 包含性能基准测试CMakeLists
  include(${CMAKE_CURRENT_LIST_DIR}/benchmark/CMakeLists.txt)
ENDIF()

# 量化工具构建
IF(MNN_BUILD_QUANTOOLS)
  # 包含量化工具CMakeLists
  include(${CMAKE_CURRENT_LIST_DIR}/tools/quantization/CMakeLists.txt)
ENDIF()

# 评估工具构建
IF(MNN_EVALUATION)
  # 包含评估工具CMakeLists
  include(${CMAKE_CURRENT_LIST_DIR}/tools/evaluation/CMakeLists.txt)
ENDIF()

# 安装头文件
# 如果是Android系统且不通过命令行构建
IF(CMAKE_SYSTEM_NAME MATCHES "^Android" AND NOT MNN_BUILD_FOR_ANDROID_COMMAND)
    # 如果没有设置原生包含输出路径，则设置为当前目录
    IF(NOT NATIVE_INCLUDE_OUTPUT)
      set(NATIVE_INCLUDE_OUTPUT ".")
    ENDIF()
    # 设置MNN包含输出路径
    set(MNN_INCLUDE_OUTPUT ${NATIVE_INCLUDE_OUTPUT}/MNN)
    # 创建MNN包含目录
    add_custom_command(
      TARGET MNN
      POST_BUILD
      COMMAND ${CMAKE_COMMAND}
      -E make_directory "${MNN_INCLUDE_OUTPUT}/"
    )
    # 创建MNN表达式包含目录
    add_custom_command(
      TARGET MNN
      POST_BUILD
      COMMAND ${CMAKE_COMMAND}
      -E make_directory "${MNN_INCLUDE_OUTPUT}/expr/"
    )
    # 复制MNN公共头文件
    FOREACH(header ${MNN_PUB_HDRS})
      add_custom_command(
        TARGET MNN
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${header} "${MNN_INCLUDE_OUTPUT}/"
      )
    ENDFOREACH()
    # 复制MNN表达式公共头文件
    FOREACH(header ${MNN_EXPR_PUB_HDRS})
      add_custom_command(
        TARGET MNN
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${header} "${MNN_INCLUDE_OUTPUT}/expr/"
      )
    ENDFOREACH()
# 如果不是Apple平台
ELSEIF(NOT APPLE)
  # 安装MNN公共头文件
  INSTALL(FILES ${MNN_PUB_HDRS} DESTINATION include/MNN/)
  # 安装MNN表达式公共头文件
  INSTALL(FILES ${MNN_EXPR_PUB_HDRS} DESTINATION include/MNN/expr/)
  # 安装MNN目标
  install(TARGETS MNN
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
  )
# Apple平台
ELSE()
  # 安装MNN目标为框架
  install(TARGETS MNN
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      FRAMEWORK DESTINATION /Library/Frameworks/
  )
  # 如果构建OpenCV支持
  IF(MNN_BUILD_OPENCV)
    # 如果不构建Apple框架
    if (NOT MNN_AAPL_FMWK)
        # 安装OpenCV头文件
        INSTALL(FILES ${MNN_CV_HDRS} DESTINATION include/MNN/cv)
        INSTALL(FILES ${MNN_CV_IMGHDRS} DESTINATION include/MNN/cv/imgproc)
    endif()
    # 设置OpenCV头文件的包位置属性
    FOREACH(HDR ${MNN_CV_HDRS})
      SET_SOURCE_FILES_PROPERTIES(${HDR} PROPERTIES MACOSX_PACKAGE_LOCATION Headers/cv/ )
    ENDFOREACH()
    FOREACH(HDR ${MNN_CV_IMGHDRS})
      SET_SOURCE_FILES_PROPERTIES(${HDR} PROPERTIES MACOSX_PACKAGE_LOCATION Headers/cv/imgproc )
    ENDFOREACH()
  ENDIF()
  # 如果构建音频支持
  IF(MNN_BUILD_AUDIO)
    # 如果不构建Apple框架
    if (NOT MNN_AAPL_FMWK)
      # 安装音频头文件
      INSTALL(FILES ${MNN_AUDIO_HDRS} DESTINATION include/MNN/audio)
    endif()
    # 设置音频头文件的包位置属性
    FOREACH(HDR ${MNN_AUDIO_HDRS})
      SET_SOURCE_FILES_PROPERTIES(${HDR} PROPERTIES MACOSX_PACKAGE_LOCATION Headers/audio/ )
    ENDFOREACH()
  ENDIF()
  # 如果构建大语言模型
  IF(MNN_BUILD_LLM)
    # 如果不构建Apple框架
    if (NOT MNN_AAPL_FMWK)
        # 安装LLM头文件
        INSTALL(FILES ${MNN_LLM_HDRS} DESTINATION include/MNN/llm)
    endif()
    # 设置LLM头文件的包位置属性
    FOREACH(HDR ${MNN_LLM_HDRS})
      SET_SOURCE_FILES_PROPERTIES(${HDR} PROPERTIES MACOSX_PACKAGE_LOCATION Headers/llm )
    ENDFOREACH()
  ENDIF()

  # 如果构建扩散模型
  IF(MNN_BUILD_DIFFUSION)
    # 如果不构建Apple框架
    if (NOT MNN_AAPL_FMWK)
        # 安装扩散模型头文件
        INSTALL(FILES ${MNN_DIFFUSION_HDRS} DESTINATION include/MNN/diffusion)
    endif()
    # 设置扩散模型头文件的包位置属性
    FOREACH(HDR ${MNN_DIFFUSION_HDRS})
      SET_SOURCE_FILES_PROPERTIES(${HDR} PROPERTIES MACOSX_PACKAGE_LOCATION Headers/diffusion )
    ENDFOREACH()
  ENDIF()

  # 如果不构建Apple框架
  if (NOT MNN_AAPL_FMWK)
      # 安装MNN公共头文件
      INSTALL(FILES ${MNN_PUB_HDRS} DESTINATION include/MNN/)
      # 安装MNN表达式公共头文件
      INSTALL(FILES ${MNN_EXPR_PUB_HDRS} DESTINATION include/MNN/expr/)
  endif()
  # 设置MNN表达式公共头文件的包位置属性
  FOREACH(HDR ${MNN_EXPR_PUB_HDRS})
    SET_SOURCE_FILES_PROPERTIES(${HDR} PROPERTIES MACOSX_PACKAGE_LOCATION Headers/expr/ )
  ENDFOREACH()
  # 设置MNN公共头文件的包位置属性
  FOREACH(HDR ${MNN_PUB_HDRS})
    SET_SOURCE_FILES_PROPERTIES(${HDR} PROPERTIES MACOSX_PACKAGE_LOCATION Headers/ )
  ENDFOREACH()
  # 如果启用Metal
  IF(MNN_METAL)
    # 设置Metal库文件的包位置属性
    SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/mnn.metallib PROPERTIES MACOSX_PACKAGE_LOCATION Resources/)
  ENDIF()
ENDIF()

# JNI支持
# 如果构建JNI
if (MNN_JNI)
    # 添加JNI子目录
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/jni/)
endif()

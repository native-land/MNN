# 更多关于在 Android Studio 中使用 CMake 的信息，请参阅文档：
# https://d.android.com/studio/projects/add-native-code.html
# 更多使用 CMake 的示例，请参见：https://github.com/android/ndk-samples

# 设置此项目所需的最低 CMake 版本
cmake_minimum_required(VERSION 3.22.1)

# 声明项目名称。项目名称可以通过 ${PROJECT_NAME} 访问
# 由于这是顶层 CMakeLists.txt，项目名称也可以通过 ${CMAKE_PROJECT_NAME} 访问
# （这两个 CMake 变量在顶层构建脚本范围内是同步的）
project("mnnllmapp")

# 添加一个共享库，库名与项目名相同
add_library(${CMAKE_PROJECT_NAME} SHARED
        # 列出相对于此 CMakeLists.txt 的 C/C++ 源文件
        llm_mnn_jni.cpp         # LLM 的 JNI 接口实现
        diffusion_jni.cpp       # 扩散模型的 JNI 接口实现
        diffusion_session.cpp   # 扩散模型会话管理
        llm_session.cpp         # LLM 会话管理
        crash_util.cpp          # 崩溃处理工具
)

# MNN 源码根目录设置
set(MNN_SOURCE_ROOT "${CMAKE_SOURCE_DIR}/../../../../../../../")
# MNN 安装根目录设置（预编译库的位置）
set(MNN_INSTALL_ROOT "${MNN_SOURCE_ROOT}/project/android/build_64")

# 输出调试信息，显示 MNN 的源码和安装路径
# MNN_SOURCE_ROOT ： /home/stark/code/Cpp/MNN/apps/Android/MnnLlmChat/app/src/main/cpp/../../../../../../../
# /home/stark/code/Cpp/MNN
# MNN_INSTALL_ROOT ： /home/stark/code/Cpp/MNN/apps/Android/MnnLlmChat/app/src/main/cpp/../../../../../../..//project/android/build_64
# /home/stark/code/Cpp/MNN/project/android/build_64
message(INFO "MNN_SOURCE_ROOT: ${MNN_SOURCE_ROOT}")
message(INFO "MNN_INSTALL_ROOT: ${MNN_INSTALL_ROOT}")

# 添加 MNN 头文件目录
include_directories("${MNN_SOURCE_ROOT}/include/")

# 添加扩散模型引擎的头文件目录
include_directories("${MNN_SOURCE_ROOT}/transformers/diffusion/engine/include/")

# 添加 LLM 引擎的头文件目录
include_directories("${MNN_SOURCE_ROOT}/transformers/llm/engine/include")

# 添加音频处理工具的头文件目录
include_directories("${MNN_SOURCE_ROOT}/tools/audio/include")
# 添加第三方库头文件目录
include_directories("${CMAKE_SOURCE_DIR}/third_party")

# 设置库文件路径
set(LIB_PATH "${MNN_INSTALL_ROOT}/lib")

# 导入 MNN 共享库
add_library(MNN SHARED IMPORTED)
set_target_properties(MNN PROPERTIES IMPORTED_LOCATION "${LIB_PATH}/libMNN.so")

# 以下被注释掉的代码是导入其他可能需要的共享库的示例
# 如果需要启用这些库，请取消注释相应部分
#
# add_library(llm SHARED IMPORTED)
# set_target_properties(llm PROPERTIES IMPORTED_LOCATION "${LIB_PATH}/libllm.so")
#
# add_library(diffusion SHARED IMPORTED)
# set_target_properties(diffusion PROPERTIES IMPORTED_LOCATION "${LIB_PATH}/libdiffusion.so")
#
# add_library(mnn_cl SHARED IMPORTED)
# set_target_properties(mnn_cl PROPERTIES IMPORTED_LOCATION "${LIB_PATH}/libMNN_CL.so")
#
# add_library(mnn_audio SHARED IMPORTED)
# set_target_properties(mnn_audio PROPERTIES IMPORTED_LOCATION "${LIB_PATH}/libMNNAudio.so")
#
# add_library(mnn_cv SHARED IMPORTED)
# set_target_properties(mnn_cv PROPERTIES IMPORTED_LOCATION "${LIB_PATH}/libMNNOpenCV.so")
#
# add_library(mnn_Express SHARED IMPORTED)
# set_target_properties(mnn_Express PROPERTIES IMPORTED_LOCATION "${LIB_PATH}/libMNN_Express.so")

# 指定 CMake 应该链接到目标库的库
# 可以链接此构建脚本中定义的库、预构建的第三方库或 Android 系统库
target_link_libraries(${CMAKE_PROJECT_NAME}
        # 列出链接到目标库的库
        android     # Android 系统库
        log         # 日志库
        MNN         # MNN 库
)
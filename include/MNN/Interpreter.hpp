//
//  Interpreter.hpp
//  MNN
//
//  Created by MNN on 2018/07/23.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#ifndef MNN_Interpreter_hpp
#define MNN_Interpreter_hpp

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <MNN/ErrorCode.hpp>
#include <MNN/MNNForwardType.h>
#include <MNN/Tensor.hpp>

namespace MNN {

/** session schedule config */
struct ScheduleConfig {
    /** which tensor should be kept */
    std::vector<std::string> saveTensors;
    /** forward type */
    MNNForwardType type = MNN_FORWARD_CPU;
    /** CPU:number of threads in parallel , Or GPU: mode setting*/
    union {
        int numThread = 4;
        int mode;
    };

    /** subpath to run */
    struct Path {
        std::vector<std::string> inputs;
        std::vector<std::string> outputs;

        enum Mode {
            /**
             * Op Mode
             * - inputs means the source op, can NOT be empty.
             * - outputs means the sink op, can be empty.
             * The path will start from source op, then flow when encounter the sink op.
             * The sink op will not be compute in this path.
             */
            Op = 0,

            /**
             * Tensor Mode
             * - inputs means the inputs tensors, can NOT be empty.
             * - outputs means the outputs tensors, can NOT be empty.
             * It will find the pipeline that compute outputs from inputs.
             */
            Tensor = 1
        };

        /** running mode */
        Mode mode = Op;
    };
    Path path;

    /** backup backend used to create execution when desinated backend do NOT support any op */
    MNNForwardType backupType = MNN_FORWARD_CPU;

    /** extra backend config */
    BackendConfig* backendConfig = nullptr;
};

class Session;
struct Content;
class Tensor;
class Backend;
class Runtime;

class MNN_PUBLIC OperatorInfo {
    struct Info;

public:
    /** Operator's name*/
    const std::string& name() const;

    /** Operator's type*/
    const std::string& type() const;

    /** Operator's flops, in M*/
    float flops() const;

protected:
    OperatorInfo();
    ~OperatorInfo();
    Info* mContent;
};

typedef std::function<bool(const std::vector<Tensor*>&, const std::string& /*opName*/)> TensorCallBack;
typedef std::function<bool(const std::vector<Tensor*>&, const OperatorInfo*)> TensorCallBackWithInfo;
typedef std::pair< std::map<MNNForwardType, std::shared_ptr<Runtime>>,  std::shared_ptr<Runtime>> RuntimeInfo;

/**
 * @brief get mnn version info.
 * @return mnn version string.
 */
MNN_PUBLIC const char* getVersion();

/**
 * @brief MNN模型解释器类，用于加载模型、创建会话和执行推理
 * @details Interpreter是MNN的核心类之一，负责：
 *          1. 从文件或内存缓冲区加载模型
 *          2. 创建和管理推理会话
 *          3. 设置会话参数和运行时配置
 *          4. 执行模型推理
 *          5. 管理输入输出张量
 *          
 *          多个会话可以共享同一个Interpreter实例，这样可以节省内存。
 */
class MNN_PUBLIC Interpreter {
public:
    /**
     * @brief 从文件创建Interpreter实例
     * @param file  模型文件路径
     * @return 成功时返回Interpreter实例，失败时返回NULL
     * @details 该函数会加载指定路径的MNN模型文件，并创建对应的Interpreter实例
     */
    static Interpreter* createFromFile(const char* file);
    
    /**
     * @brief 从内存缓冲区创建Interpreter实例
     * @param buffer    模型数据缓冲区指针
     * @param size      缓冲区大小（字节）
     * @return 成功时返回Interpreter实例，失败时返回NULL
     * @details 该函数会从内存中的模型数据创建Interpreter实例，适用于模型已加载到内存的场景
     */
    static Interpreter* createFromBuffer(const void* buffer, size_t size);
    
    /**
     * @brief 析构函数，释放Interpreter实例
     */
    ~Interpreter();
    
    /**
     * @brief 销毁Interpreter实例
     * @param net    要释放的Interpreter实例指针
     * @details 该函数用于释放通过createFromFile或createFromBuffer创建的Interpreter实例
     */
    static void destroy(Interpreter* net);

    enum SessionMode {
        /** 回调相关模式，默认为Session_Debug */
        /** 允许使用runSessionWithCallBack，并可以获取内部操作信息 */
        Session_Debug = 0,
        /** 禁用runSessionWithCallBack，无法获取会话中的操作信息 */
        Session_Release = 1,

        /** 输入张量相关模式，默认为Session_Input_Inside */
        /** 输入张量由会话分配，在会话调整大小后输入数据 */
        Session_Input_Inside = 2,
        /** 输入张量由用户分配，在会话调整大小前设置输入数据 */
        Session_Input_User = 3,

        /** 输出张量相关模式 */
        /** 输出张量依赖于会话，不能独立使用 */
        Session_Output_Inside = 4,
        /** 输出张量可以从会话中分离出来独立使用 */
        Session_Output_User = 5,

        /** 会话调整大小模式 */
        /** 创建会话时直接调整大小 */
        Session_Resize_Direct = 6,
        /** 延迟调整会话大小 */
        Session_Resize_Defer = 7,

        /** 执行后端类型确定模式 */
        /** 使用用户指定的后端，当不支持时使用默认后端 */
        Session_Backend_Fix = 8,
        /** 自动确定操作类型 */
        Session_Backend_Auto = 9,

        /** 静态内存管理模式 */
        /** 在resizeSession时回收静态内存，防止内存爆炸 */
        Session_Memory_Collect = 10,
        /** 缓存静态内存供下次前向使用 */
        Session_Memory_Cache = 11,

        /** 代码生成功能控制 */
        /** 禁用代码生成，避免额外的代码生成成本 */
        Session_Codegen_Disable = 12,
        /** 启用代码生成 */
        Session_Codegen_Enable = 13,
        
        /** 动态调整大小优化 */
        /** 开启调整大小跟踪 */
        Session_Resize_Check = 14,
        /** 应用调整大小优化 */
        Session_Resize_Fix = 15,
        
        /** 模块traceOrOptimize API设置 */
        /** 模块前向分离模式：
         *  当输入非空时，模块的onForward只推断形状并分配内存
         *  当输入为空时，模块的onForward只运行会话来计算内容
         *  默认为Module_Forward_Combine
         */
        Module_Forward_Separate = 16,
        Module_Forward_Combine = 17,
    };
    /**
     * @brief 设置会话模式
     * @param mode      会话模式
     * @details 该API应在创建会话之前调用，用于设置会话的各种行为模式
     */
    void setSessionMode(SessionMode mode);

    /**
     * @brief 设置缓存文件
     * @param cacheFile      缓存文件名
     * @param keySize        已废弃，供将来使用
     * @details 该API应在创建会话之前调用。
     *          如果缓存存在，尝试从文件加载缓存。
     *          创建会话后，尝试将缓存保存到文件。
     */
    void setCacheFile(const char* cacheFile, size_t keySize = 128);

    /**
     * @brief 设置外部数据文件
     * @param file      外部数据文件名
     * @param flag      标志位，已废弃，供将来使用
     * @details 该API应在创建会话之前调用
     */
    void setExternalFile(const char* file, size_t flag = 128);
    
    /**
     * @brief 更新缓存文件
     * @param session    指定会话
     * @param flag       保护参数，目前未使用
     * @details 该API应在最后一次调整会话大小后调用。
     *          如果调整会话大小生成了新的缓存信息，尝试重写缓存文件。
     *          如果调整会话大小没有生成任何新的缓存信息，则不执行任何操作。
     */

    ErrorCode updateCacheFile(Session *session, int flag = 0);

    enum HintMode {
        // 异步调优的最大操作数
        MAX_TUNING_NUMBER = 0,
        // 是否严格检查模型文件，默认为1。如果设置为0，将不检查模型文件的有效性
        STRICT_CHECK_MODEL = 1,
        MEM_ALLOCATOR_TYPE = 2,
        // Winograd单元候选数量，默认为3。如果设置为0，将使用较少的单元候选以减少内存占用，但会牺牲性能
        WINOGRAD_MEMORY_LEVEL = 3,

        // 几何计算选项，默认为0xFFFF
        GEOMETRY_COMPUTE_MASK = 4,

        // 默认为0
        // 1: 对于通用卷积，使用一个scale和zeropoint进行量化
        // 2: 对输入数据使用块量化
        DYNAMIC_QUANT_OPTIONS = 5,

        // 对于具有大核-小核的移动CPU，设置降速比率，让MNN根据CPU性能差异分配任务
        // 0-100，50表示小核具有大核50%的性能
        // 默认为50
        CPU_LITTLECORE_DECREASE_RATE = 6,

        // 0: 不量化
        // 1: 仅量化key，使用int8非对称量化
        // 2: 仅量化value，使用fp8量化
        // 3: 同时量化key和value
        // 4: 量化query、key和value，并使用gemm int8内核计算K*V
        QKV_QUANT_OPTIONS = 7,

        // 内存中kvcache的大小限制（对于单个层）
        // 如果kvcache的大小超过限制，将被移动到磁盘
        KVCACHE_SIZE_LIMIT = 8,
        // 提交操作的编码器数量
        OP_ENCODER_NUMBER_FOR_COMMIT = 9,

        // KVCache信息
        KVCACHE_INFO = 10,
        // mmap分配的文件大小，单位KB
        MMAP_FILE_SIZE = 11,
        USE_CACHED_MMAP = 12,
        
        // 多线程加载模块，默认为0（不使用其他线程）
        INIT_THREAD_NUMBER = 13,

        // 使用的CPU核心ID
        CPU_CORE_IDS = 14,

        // 当支持Arm sme2时设置使用的CPU线程数
        CPU_SME2_INSTRUCTIONS = 15
    };

    enum ExternalPathType {
        // KVCache目录路径
        EXTERNAL_PATH_KVCACHE_DIR = 0,
        
        // 中间缓冲区缓存文件
        EXTERNAL_FEATUREMAP_DIR = 1,

        // 权重缓冲区缓存文件
        EXTERNAL_WEIGHT_DIR = 2,

        // 其他类型...
    };

    enum GeometryComputeMask {
        // 支持区域融合
        GEOMETRCOMPUTEMASK_FUSEREGION = 1 << 0,

        // 支持多区域输入的区域融合，例如：pad + concat
        GEOMETRCOMPUTEMASK_FUSEREGION_MULTI = 1 << 1,

        // 如果可能，使用循环代替光栅化+计算
        GEOMETRCOMPUTEMASK_USELOOP = 1 << 2,
        
        // 支持几何缓存，如果形状改变，将尝试重新计算，如果失败则运行计算
        GEOMETRCOMPUTEMASK_OPENCACHE = 1 << 3,
        
        // 全部选项开启掩码，例如，如果想关闭useloop，可以将掩码设置为(GEOMETRCOMPUTEMASK_ALL - GEOMETRCOMPUTEMASK_USELOOP)
        GEOMETRCOMPUTEMASK_ALL = 0xFFFF,
    };

    /**
     * @brief 设置会话提示
     * @param hint      提示类型
     * @param value     提示值
     * @details 该API应在创建会话之前调用
     */
    void setSessionHint(HintMode hint, int value);
    
    /**
     * @brief 设置会话提示（指针版本）
     * @param hint      提示类型
     * @param value     提示值指针
     * @param size      提示值大小（当使用指针时）
     * @details 该API应在创建会话之前调用
     */
    void setSessionHint(HintMode hint, int* value, size_t size);
public:
    /**
     * @brief 根据调度配置单独创建运行时信息
     * @param configs 会话调度配置
     * @return 创建的运行时信息
     */
    static RuntimeInfo createRuntime(const std::vector<ScheduleConfig>& configs);

    /**
     * @brief 根据调度配置创建会话。创建的会话将在网络中管理。
     * @param config 会话调度配置
     * @return 成功时返回创建的会话，失败时返回NULL
     */
    Session* createSession(const ScheduleConfig& config);

    /**
     * @brief 根据调度配置和用户指定的运行时创建会话
     * @param config 会话调度配置
     * @param runtime 创建会话使用的运行时信息
     * @return 成功时返回创建的会话，失败时返回NULL
     */
    Session* createSession(const ScheduleConfig& config, const RuntimeInfo& runtime);

    /**
     * @brief 根据多个调度配置创建多路径会话。创建的会话将在网络中管理。
     * @param configs 会话调度配置列表
     * @return 成功时返回创建的会话，失败时返回NULL
     */
    Session* createMultiPathSession(const std::vector<ScheduleConfig>& configs);

    /**
     * @brief 根据多个调度配置和用户指定的运行时创建多路径会话。
     *        创建的会话将在网络中管理。
     * @param configs 会话调度配置列表
     * @param runtime 运行时信息
     * @return 成功时返回创建的会话，失败时返回NULL
     */
    Session* createMultiPathSession(const std::vector<ScheduleConfig>& configs, const RuntimeInfo& runtime);

    /**
     * @brief 释放会话
     * @param session   要释放的会话
     * @return 如果给定会话由网络持有并被释放则返回true，否则返回false
     */
    bool releaseSession(Session* session);

    /**
     * @brief 调用此函数使张量就绪。在调整任何输入张量大小后，应获取输出张量缓冲区（主机或设备ID）。
     * @param session 给定的会话
     */
    void resizeSession(Session* session);

    /**
     * @brief 调用此函数使张量就绪。在调整任何输入张量大小后，应获取输出张量缓冲区（主机或设备ID）。
     * @param session 给定的会话
     * @param needRelloc 1表示需要重新分配
     */
    void resizeSession(Session* session, int needRelloc);

    
    /**
     * @brief 如果不再需要调整大小或创建会话，请调用此函数，它将节省等于模型缓冲区大小的内存
     */
    void releaseModel();

    /**
     * @brief 获取模型缓冲区供用户保存
     * @return std::make_pair(modelBuffer, modelSize)
     * @example:
     * std::ofstream output("trainResult.alinn")
     * auto buffer = net->getModelBuffer();
     * output.write((const char*)buffer.first, buffer.second);
     */
    std::pair<const void*, size_t> getModelBuffer() const;

    /**
     * @brief 获取模型的版本信息
     * @return 模型版本信息的const char*指针，如"2.0.0"；
     *         如果模型未加载或模型无版本信息，则返回"version info not found"
     */
    const char* getModelVersion() const;

    /**
     * @brief 更新会话的张量到模型的常量操作
     * @param session   给定的会话
     * @return 运行结果
     */
    ErrorCode updateSessionToModel(Session* session);

    /**
     * @brief 运行会话
     * @param session   给定的会话
     * @return 运行结果
     */
    ErrorCode runSession(Session* session) const;

    /*
     * @brief 运行会话
     * @param session   给定的会话
     * @param before    每个操作之前的回调。返回true执行操作；返回false跳过操作
     * @param end       每个操作之后的回调。返回true继续运行；返回false中断会话
     * @param sync      是否同步等待执行完成
     * @return 运行结果
     */
    ErrorCode runSessionWithCallBack(const Session* session, const TensorCallBack& before, const TensorCallBack& end,
                                     bool sync = false) const;

    /*
     * @brief 运行会话
     * @param session   给定的会话
     * @param before    每个操作之前的回调。返回true执行操作；返回false跳过操作
     * @param end       每个操作之后的回调。返回true继续运行；返回false中断会话
     * @param sync      是否同步等待执行完成
     * @return 运行结果
     */
    ErrorCode runSessionWithCallBackInfo(const Session* session, const TensorCallBackWithInfo& before,
                                         const TensorCallBackWithInfo& end, bool sync = false) const;

    /**
     * @brief 根据给定名称获取输入张量
     * @param session   给定的会话
     * @param name      给定的名称。如果为NULL，返回第一个输入
     * @return 如果找到返回张量，否则返回NULL
     */
    Tensor* getSessionInput(const Session* session, const char* name);
    /**
     * @brief 根据给定名称获取输出张量
     * @param session   给定的会话
     * @param name      给定的名称。如果为NULL，返回第一个输出
     * @return 如果找到返回张量，否则返回NULL
     */
    Tensor* getSessionOutput(const Session* session, const char* name);

    enum SessionInfoCode {
        /** 会话使用的内存大小（MB），float* */
        MEMORY = 0,

        /** 会话中需要的浮点运算次数（百万次），float* */
        FLOPS = 1,

        /** 会话中的后端信息，int*，长度 >= 1 + 创建会话时的配置数量 */
        BACKENDS = 2,

        /** 调整大小信息，int*，不同API的含义：
         Interpreter::getSessionInfo: 0: 准备执行，1: 需要分配内存，2: 需要调整大小
         RuntimeManager::getInfo: 0: 无需调整大小，1: 重新分配内存，2: 调整大小
         */
        RESIZE_STATUS = 3,
        
        /** 模式/线程数，int* */
        THREAD_NUMBER = 4,

        ALL
    };

    /**
     * @brief 获取会话信息
     * @param session   给定的会话
     * @param code      给定的信息代码
     * @param ptr       给定的信息指针，详见SessionInfoCode
     * @return 如果支持该代码则返回true，否则返回false
     */
    bool getSessionInfo(const Session* session, SessionInfoCode code, void* ptr);

    /**
     * @brief 获取所有输出张量
     * @param session   给定的会话
     * @return 以名称映射的所有输出张量
     */
    const std::map<std::string, Tensor*>& getSessionOutputAll(const Session* session) const;
    
    /**
     * @brief 获取所有输入张量
     * @param session   给定的会话
     * @return 以名称映射的所有输入张量
     */
    const std::map<std::string, Tensor*>& getSessionInputAll(const Session* session) const;

public:
    /**
     * @brief 调整给定张量的大小
     * @param tensor    给定的张量
     * @param dims      新的维度，最多6个维度
     */
    void resizeTensor(Tensor* tensor, const std::vector<int>& dims);

    /**
     * @brief 按NCHW格式调整给定张量的大小
     * @param batch     批次大小/N
     * @param channel   通道数/C
     * @param height    高度/H
     * @param width     宽度/W
     */
    void resizeTensor(Tensor* tensor, int batch, int channel, int height, int width);

    /**
     * @brief 获取用于创建给定张量的后端
     * @param session   给定的会话
     * @param tensor    给定的张量
     * @return 用于创建给定张量的后端，可能为NULL
     */
    const Backend* getBackend(const Session* session, const Tensor* tensor) const;

    /**
     * @brief 获取业务代码（模型标识符）
     * @return 业务代码
     */
    const char* bizCode() const;

    /**
     * @brief 获取模型UUID
     * @return 模型UUID
     */
    const char* uuid() const;

private:
    /**
     * @brief 从缓冲区内部创建Interpreter实例
     * @param net 网络内容
     * @param enforceAuth 是否强制认证
     * @return Interpreter实例
     */
    static Interpreter* createFromBufferInternal(Content* net, bool enforceAuth);

    Content* mNet = nullptr;  ///< 网络内容指针
    Interpreter(Content* net);  ///< 构造函数

    /**
     * @brief 禁用拷贝构造函数
     */
    Interpreter(const Interpreter&)  = delete;
    
    /**
     * @brief 禁用移动构造函数
     */
    Interpreter(const Interpreter&&) = delete;
    
    /**
     * @brief 禁用拷贝赋值运算符
     */
    Interpreter& operator=(const Interpreter&) = delete;
    
    /**
     * @brief 禁用移动赋值运算符
     */
    Interpreter& operator=(const Interpreter&&) = delete;
    
    /**
     * @brief 等待会话完成
     * @param session 给定的会话
     */
    void waitSessionFinish(const Session* session) const;
    
#ifdef MNN_INTERNAL_ENABLED
    /**
     * @brief 记录会话运行日志
     * @param session 给定的会话
     * @param time 运行时间
     * @param api API名称
     */
    void logForRunSession(const Session* session, float time, const char* api) const;
#endif
};
} // namespace MNN

#endif /* Interpreter_hpp */

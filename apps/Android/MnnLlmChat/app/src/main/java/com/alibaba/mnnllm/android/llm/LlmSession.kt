// Created by ruoyi.sjd on 2025/5/7.
// Copyright (c) 2024 Alibaba Group Holding Limited All rights reserved.

package com.alibaba.mnnllm.android.llm;

import android.util.Log
import com.alibaba.mls.api.ApplicationProvider
import com.alibaba.mnnllm.android.llm.ChatService.Companion.provide
import com.alibaba.mnnllm.android.chat.model.ChatDataItem
import com.alibaba.mnnllm.android.modelsettings.ModelConfig
import com.alibaba.mnnllm.android.utils.FileUtils
import com.alibaba.mnnllm.android.utils.ModelPreferences
import com.alibaba.mnnllm.android.model.ModelUtils
import com.alibaba.mnnllm.android.modelsettings.ModelConfig.Companion.getExtraConfigFile
import com.google.gson.Gson
import timber.log.Timber
import java.io.File
import java.util.stream.Collectors
import kotlin.concurrent.Volatile
import android.util.Pair

/**
 * LLM会话类，用于管理与大语言模型的交互
 *
 * @param modelId 模型ID
 * @param sessionId 会话ID
 * @param configPath 配置文件路径
 * @param savedHistory 保存的历史聊天记录
 */
class LlmSession(
    private val modelId: String,
    override var sessionId: String,
    private val configPath: String,
    val savedHistory: List<ChatDataItem>?,
) : ChatSession {
    // 额外的助手提示模板
    private var extraAssistantPrompt: String? = null

    // 是否支持Omni模式
    override var supportOmni: Boolean = false

    // 本地指针，用于调用native方法
    private var nativePtr: Long = 0

    // 模型是否正在加载
    @Volatile
    private var modelLoading = false

    // 是否正在生成响应
    @Volatile
    private var generating = false

    // 是否请求释放资源
    @Volatile
    private var releaseRequested = false

    // 是否保持历史记录
    private var keepHistory = false

    /**
     * 加载模型
     * 初始化native指针并加载模型配置
     */
    override fun load() {
        Log.d(TAG, "MNN_DEBUG load begin")
        modelLoading = true
        var historyStringList: List<String>? = null

        // 处理保存的历史记录，转换为字符串列表
        if (!this.savedHistory.isNullOrEmpty()) {
            historyStringList =
                savedHistory.stream()
                    .map { obj: ChatDataItem -> obj.text }
                    .filter { obj: String? -> obj != null }
                    .map { obj: String? -> obj!! }
                    .collect(Collectors.toList())
        }

        // 加载合并配置
        val config = ModelConfig.loadMergedConfig(configPath, getExtraConfigFile(modelId))!!
        var rootCacheDir: String? = ""

        // 如果使用mmap，则创建缓存目录
        if (config.useMmap == true) {
            rootCacheDir = FileUtils.getMmapDir(modelId, configPath.contains("modelscope"))
            File(rootCacheDir).mkdirs()
        }

        val backend = config.backendType
        val configMap = HashMap<String, Any>().apply {
            put("is_r1", ModelUtils.isR1Model(modelId))
            put("mmap_dir", rootCacheDir ?: "")
            put("keep_history", keepHistory)
        }

        // 加载额外配置
        val extraConfig =
            ModelConfig.loadMergedConfig(configPath, getExtraConfigFile(modelId))?.apply {
                this.assistantPromptTemplate = extraAssistantPrompt
                this.backendType = backend
            }

        Log.d(TAG, "MNN_DEBUG load initNative")

        // 初始化native指针
        nativePtr = initNative(
            configPath, historyStringList,
            if (extraConfig != null) {
                Gson().toJson(extraConfig)
            } else {
                "{}"
            },
            Gson().toJson(configMap)
        )

        Log.d(TAG, "MNN_DEBUG load initNative end")
        modelLoading = false

        // 如果已请求释放资源，则执行释放操作
        if (releaseRequested) {
            release()
        }
    }

    /**
     * 生成新的会话ID
     * @return 新的会话ID
     */
    private fun generateNewSessionId(): String {
        this.sessionId = System.currentTimeMillis().toString()
        return this.sessionId
    }

    /**
     * 生成响应
     *
     * @param prompt 用户输入的提示
     * @param params 生成参数
     * @param progressListener 进度监听器
     * @return 包含生成结果的HashMap
     */
    override fun generate(
        prompt: String,
        params: Map<String, Any>,
        progressListener: GenerateProgressListener
    ): HashMap<String, Any> {
        synchronized(this) {
            Log.d(TAG, "MNN_DEBUG submit$prompt")
            generating = true
            // 调用native方法生成响应
            val result = submitNative(nativePtr, prompt, keepHistory, progressListener)
            generating = false

            // 如果已请求释放资源，则执行释放操作
            if (releaseRequested) {
                release()
            }
            return result
        }
    }

    /**
     * 重置会话
     * @return 新的会话ID
     */
    override fun reset(): String {
        synchronized(this) {
            // 调用native方法重置会话
            resetNative(nativePtr)
        }
        return generateNewSessionId()
    }

    /**
     * 释放资源
     */
    override fun release() {
        synchronized(this) {
            Log.d(
                TAG,
                "MNN_DEBUG release nativePtr: $nativePtr mGenerating: $generating"
            )
            // 如果没有在生成或加载模型，则直接释放资源
            if (!generating && !modelLoading) {
                releaseInner()
            } else {
                // 否则标记为请求释放，并等待当前操作完成后再释放
                releaseRequested = true
                while (generating || modelLoading) {
                    try {
                        (this as Object).wait()
                    } catch (e: InterruptedException) {
                        Thread.currentThread().interrupt()
                        Log.e(TAG, "Thread interrupted while waiting for release", e)
                    }
                }
                releaseInner()
            }
        }
    }

    /**
     * 内部释放资源方法
     */
    private fun releaseInner() {
        if (nativePtr != 0L) {
            // 调用native方法释放资源
            releaseNative(nativePtr)
            nativePtr = 0
            // 从服务中移除会话
            provide().removeSession(sessionId)
            (this as Object).notifyAll()
        }
    }

    /**
     * 初始化native方法
     *
     * @param configPath 配置文件路径
     * @param history 历史记录
     * @param mergedConfigStr 合并配置字符串
     * @param configJsonStr 配置JSON字符串
     * @return native指针
     */
    private external fun initNative(
        configPath: String?,
        history: List<String>?,
        mergedConfigStr: String?,
        configJsonStr: String?
    ): Long

    /**
     * 提交生成请求到native方法
     *
     * @param instanceId 实例ID
     * @param input 输入文本
     * @param keepHistory 是否保持历史记录
     * @param listener 进度监听器
     * @return 包含生成结果的HashMap
     */
    private external fun submitNative(
        instanceId: Long,
        input: String,
        keepHistory: Boolean,
        listener: GenerateProgressListener
    ): HashMap<String, Any>

    /**
     * 重置native方法
     * @param instanceId 实例ID
     */
    private external fun resetNative(instanceId: Long)

    /**
     * 获取调试信息的native方法
     * @param instanceId 实例ID
     * @return 调试信息字符串
     */
    private external fun getDebugInfoNative(instanceId: Long): String

    /**
     * 释放资源的native方法
     * @param instanceId 实例ID
     */
    private external fun releaseNative(instanceId: Long)

    /**
     * 设置波形回调的native方法
     *
     * @param instanceId 实例ID
     * @param listener 音频数据监听器
     * @return 是否设置成功
     */
    private external fun setWavformCallbackNative(
        instanceId: Long,
        listener: AudioDataListener?
    ): Boolean

    /**
     * 设置是否保持历史记录
     * @param keepHistory 是否保持历史记录
     */
    override fun setKeepHistory(keepHistory: Boolean) {
        this.keepHistory = keepHistory
    }

    /**
     * 设置是否启用音频输出
     * @param enable 是否启用音频输出
     */
    override fun setEnableAudioOutput(enable: Boolean) {
        updateEnableAudioOutputNative(nativePtr, enable)
    }

    /**
     * 获取调试信息
     */
    override val debugInfo
        get() = getDebugInfoNative(nativePtr) + "\n"

    /**
     * 设置音频数据监听器
     * @param listener 音频数据监听器
     */
    fun setAudioDataListener(listener: AudioDataListener?) {
        synchronized(this) {
            if (nativePtr != 0L) {
                setWavformCallbackNative(nativePtr, listener)
            } else {
                Log.e(TAG, "nativePtr null")
            }
        }
    }

    /**
     * 更新最大新token数
     * @param maxNewTokens 最大新token数
     */
    fun updateMaxNewTokens(maxNewTokens: Int) {
        updateMaxNewTokensNative(nativePtr, maxNewTokens)
    }

    /**
     * 更新系统提示
     * @param systemPrompt 系统提示
     */
    fun updateSystemPrompt(systemPrompt: String) {
        updateSystemPromptNative(nativePtr, systemPrompt)
    }

    /**
     * 更新助手提示
     * @param assistantPrompt 助手提示
     */
    fun updateAssistantPrompt(assistantPrompt: String) {
        extraAssistantPrompt = assistantPrompt
        updateAssistantPromptNative(nativePtr, assistantPrompt)
    }

    /**
     * 更新启用音频输出的native方法
     * @param llmPtr LLM指针
     * @param enable 是否启用
     */
    private external fun updateEnableAudioOutputNative(llmPtr: Long, enable: Boolean)

    /**
     * 更新最大新token数的native方法
     * @param llmPtr LLM指针
     * @param maxNewTokens 最大新token数
     */
    private external fun updateMaxNewTokensNative(llmPtr: Long, maxNewTokens: Int)

    /**
     * 更新系统提示的native方法
     * @param llmPtr LLM指针
     * @param systemPrompt 系统提示
     */
    private external fun updateSystemPromptNative(llmPtr: Long, systemPrompt: String)

    /**
     * 更新助手提示的native方法
     * @param llmPtr LLM指针
     * @param assistantPrompt 助手提示
     */
    private external fun updateAssistantPromptNative(llmPtr: Long, assistantPrompt: String)

    companion object {
        const val TAG: String = "LlmSession"

        // 加载native库
        init {
            System.loadLibrary("mnnllmapp")
        }
    }

    /**
     * 提交完整历史记录
     *
     * @param history 完整的历史记录列表
     * @param progressListener 进度监听器
     * @return 包含生成结果的HashMap
     */
    fun submitFullHistory(
        history: List<Pair<String, String>>,
        progressListener: GenerateProgressListener
    ): HashMap<String, Any> {
        synchronized(this) {
            // 使用 Timber 替代 Log
            Timber.d("MNN_DEBUG submitFullHistory with ${history.size} messages")
            // 转换类型：kotlin.Pair -> android.util.Pair
            val androidHistory = history.map { android.util.Pair(it.first, it.second) }
            // 调用JNI方法，移除不必要的类型转换
            val result = submitFullHistoryNative(nativePtr, androidHistory, progressListener)
            generating = false
            return result
        }
    }

    /**
     * 提交完整历史记录的native方法
     *
     * @param nativePtr native指针
     * @param history 历史记录
     * @param progressListener 进度监听器
     * @return 包含生成结果的HashMap
     */
    private external fun submitFullHistoryNative(
        nativePtr: Long,
        history: List<android.util.Pair<String, String>>,
        progressListener: GenerateProgressListener
    ): HashMap<String, Any>

    /**
     * 获取模型ID
     * @return 模型ID
     */
    fun modelId(): String {
        //创建一个临时变量，避免修改原始的modelId
        return modelId
    }

    /**
     * 获取系统提示
     * @return 系统提示字符串
     */
    fun getSystemPrompt(): String? {
        return getSystemPromptNative(nativePtr)
    }

    /**
     * 获取系统提示的native方法
     * @param llmPtr LLM指针
     * @return 系统提示字符串
     */
    private external fun getSystemPromptNative(llmPtr: Long): String?
}
// Created by ruoyi.sjd on 2025/5/6.
// Copyright (c) 2024 Alibaba Group Holding Limited All rights reserved.

package com.alibaba.mnnllm.android.chat

import android.text.TextUtils
import android.util.Log
import androidx.lifecycle.lifecycleScope
import com.alibaba.mnnllm.android.llm.ChatService
import com.alibaba.mnnllm.android.llm.ChatSession
import com.alibaba.mnnllm.android.chat.ChatActivity.Companion.TAG
import com.alibaba.mnnllm.android.chat.model.ChatDataItem
import com.alibaba.mnnllm.android.chat.model.ChatDataManager
import com.alibaba.mnnllm.android.llm.GenerateProgressListener
import com.alibaba.mnnllm.android.utils.FileUtils
import com.alibaba.mnnllm.android.model.ModelUtils
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.async
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import java.util.Random
import java.util.concurrent.Executors
import java.util.concurrent.ScheduledExecutorService

/**
 * 聊天界面的主导器类，负责处理聊天逻辑
 * 
 * @param chatActivity 关联的聊天Activity
 * @param modelName 模型名称
 * @param modelId 模型ID
 */
class ChatPresenter(
    private val chatActivity: ChatActivity,
    private val modelName: String,
    private val modelId: String
) {
    // 停止生成标志位
    var stopGenerating = false
    
    // 会话ID
    private var sessionId: String? = null
    
    // 会话名称
    private var sessionName: String? = null
    
    // 聊天数据管理器
    private var chatDataManager: ChatDataManager? = null
    
    // 聊天会话实例
    private lateinit var chatSession: ChatSession
    
    // 协程作用域，用于处理异步任务
    private val presenterScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    
    // 生成监听器
    private var generateListener: GenerateListener? = null
    
    /**
     * 获取LLM会话实例
     * 为api.openai模块提供安全的访问方式
     * @return LlmSession实例，如果chatSession未初始化或不是LlmSession类型则返回null
     */
    fun getLlmSession(): com.alibaba.mnnllm.android.llm.LlmSession? {
        return if (::chatSession.isInitialized && chatSession is com.alibaba.mnnllm.android.llm.LlmSession) {
            chatSession as com.alibaba.mnnllm.android.llm.LlmSession
        } else {
            null
        }
    }
    
    /**
     * 获取当前会话ID
     * @return 会话ID，如果未设置则返回null
     */
    fun getSessionId(): String? {
        return sessionId
    }

    /**
     * 初始化构造函数
     * 初始化聊天数据管理器
     */
    init {
        chatDataManager = ChatDataManager.getInstance(chatActivity)
    }

    /**
     * 创建聊天会话
     * 根据模型类型（扩散模型或LLM模型）创建相应的会话实例
     * 
     * @return 创建的聊天会话实例
     */
    fun createSession(): ChatSession {
        val intent = chatActivity.intent
        val chatService = ChatService.provide()
        sessionId = chatActivity.intent.getStringExtra("chatSessionId")
        val chatDataItemList: List<ChatDataItem>?
        
        // 如果存在会话ID，则从数据库加载聊天数据
        if (!TextUtils.isEmpty(sessionId)) {
            chatDataItemList = chatDataManager!!.getChatDataBySession(sessionId!!)
            if (chatDataItemList.isNotEmpty()) {
                sessionName = chatDataItemList[0].text
            }
        } else {
            chatDataItemList = null
        }
        
        // 根据模型类型创建不同类型的会话
        if (ModelUtils.isDiffusionModel(modelName)) {
            // 扩散模型会话
            val diffusionDir = intent.getStringExtra("diffusionDir")
            chatSession = chatService.createDiffusionSession(
                modelId, diffusionDir,
                sessionId, chatDataItemList
            )
        } else {
            // LLM模型会话
            val configFilePath = intent.getStringExtra("configFilePath")
            chatSession = chatService.createLlmSession(
                modelId, configFilePath,
                sessionId, chatDataItemList,
                ModelUtils.isOmni(modelName)
            )
        }
        
        sessionId = chatSession.sessionId
        // 设置保持历史记录
        chatSession.setKeepHistory(true)
        return chatSession
    }

    /**
     * 加载聊天会话
     * 在协程中执行加载操作，并在加载前后通知Activity更新UI状态
     */
    fun load() {
        Log.d(TAG, "current SessionId: $sessionId")
        presenterScope.launch {
            Log.d(TAG, "chatSession loading")
            // 通知Activity开始加载
            chatActivity.lifecycleScope.launch {
                chatActivity.onLoadingChanged(true)
            }
            // 执行加载操作
            chatSession.load()

            // 通知Activity加载完成
            chatActivity.lifecycleScope.launch {
                chatActivity.onLoadingChanged(false)
            }
            Log.d(TAG, "chatSession loaded")
        }
    }

    /**
     * 重置聊天会话
     * 删除当前会话的所有聊天数据并创建新的会话
     * 
     * @param onResetSuccess 重置成功后的回调函数，接收新的会话ID作为参数
     */
    fun reset(onResetSuccess: (newSessionId: String) -> Unit) {
        presenterScope.launch {
            // 删除当前会话的所有聊天数据
            chatDataManager!!.deleteAllChatData(sessionId!!)
            // 重置会话并获取新的会话ID
            sessionId = chatSession.reset()
            // 在主线程中调用成功回调
            chatActivity.lifecycleScope.launch {
                onResetSuccess(sessionId!!)
            }
        }
    }

    /**
     * 提交扩散模型请求
     * 
     * @param prompt 用户输入的提示文本
     * @return 包含生成结果的HashMap
     */
    private fun submitDiffusionRequest(prompt: String): HashMap<String, Any> {
        // 生成扩散图像的保存路径
        val diffusionDestPath = FileUtils.generateDestDiffusionFilePath(
            chatActivity,
            sessionId!!
        )
        
        // 调用会话的generate方法生成扩散图像
        return chatSession.generate(
            prompt,
            mapOf(
                "output" to diffusionDestPath,
                "iterNum" to 20, // 迭代次数
                "randomSeed" to Random(System.currentTimeMillis()).nextInt() // 随机种子
            ),
            object : GenerateProgressListener {
                // 处理生成进度更新
                override fun onProgress(progress: String?): Boolean {
                    chatActivity.lifecycleScope.launch {
                        this@ChatPresenter.generateListener?.onDiffusionGenerateProgress(progress, diffusionDestPath)
                    }
                    return false
                }
            }
        )
    }

    /**
     * 提交LLM模型请求
     * 
     * @param prompt 用户输入的提示文本
     * @return 包含生成结果的HashMap
     */
    private fun submitLlmRequest(prompt: String): HashMap<String, Any> {
        // 创建结果处理器
        val generateResultProcessor = GenerateResultProcessor()
        // 开始生成
        generateResultProcessor.generateBegin()
        
        // 调用会话的generate方法生成文本
        val result = chatSession.generate(prompt, mapOf(), object : GenerateProgressListener {
            // 处理生成进度更新
            override fun onProgress(progress: String?): Boolean {
                generateResultProcessor.process(progress)
                chatActivity.lifecycleScope.launch {
                    this@ChatPresenter.generateListener?.onLlmGenerateProgress(progress, generateResultProcessor)
                }
                
                // 检查是否需要停止生成
                if (stopGenerating) {
                    Log.d(TAG, "stopGenerating requested")
                }
                return stopGenerating
            }
        })
        
        // 将原始结果添加到返回结果中
        result["response"] = generateResultProcessor.getRawResult()
        return result
    }

    /**
     * 提交请求（根据模型类型选择相应的处理方法）
     * 
     * @param input 用户输入
     * @param userData 用户聊天数据项
     * @return 包含生成结果的HashMap
     */
    private fun submitRequest(input: String, userData: ChatDataItem): HashMap<String, Any> {
        // 重置停止生成标志
        stopGenerating = false
        
        // 根据模型类型选择相应的处理方法
        val benchMarkResult = if (ModelUtils.isDiffusionModel(this.modelName)) {
            submitDiffusionRequest(input)
        } else {
            submitLlmRequest(input)
        }
        
        // 通知监听器生成完成
        chatActivity.lifecycleScope.launch {
            this@ChatPresenter.generateListener?.onGenerateFinished(benchMarkResult)
        }
        return benchMarkResult
    }

    /**
     * 更新会话信息
     * 
     * @param sessionId 会话ID
     * @param modelId 模型ID
     * @param sessionName 会话名称
     */
    private fun updateSession(sessionId: String, modelId: String?, sessionName: String) {
        // 添加或更新会话信息
        chatDataManager!!.addOrUpdateSession(sessionId, modelId)
        // 更新会话名称
        chatDataManager!!.updateSessionName(this.sessionId!!, this.sessionName)
    }

    /**
     * 请求生成响应
     * 
     * @param userData 用户聊天数据项
     * @param generateListener 生成监听器
     * @return 包含生成结果的HashMap
     */
    suspend fun requestGenerate(userData: ChatDataItem, generateListener: GenerateListener): HashMap<String, Any> {
        // 设置生成监听器
        this.generateListener = generateListener
        
        // 生成用户提示
        val prompt = PromptUtils.generateUserPrompt(userData)
        
        // 如果会话名称为空，则生成会话名称
        if (this.sessionName.isNullOrEmpty()) {
            this.sessionName = SessionUtils.generateSessionName(userData)
            updateSession(sessionId!!, modelId, sessionName!!)
        }
        
        // 将用户数据添加到数据库
        chatDataManager!!.addChatData(sessionId, userData)
        
        // 通知监听器开始生成
        this.generateListener?.onGenerateStart()
        
        // 在协程中提交请求并等待结果
        val result = presenterScope.async {
            return@async submitRequest(prompt, userData)
        }.await()
        
        return result
    }

    /**
     * 停止生成
     */
    fun stopGenerate() {
        stopGenerating = true
    }

    /**
     * 销毁Presenter
     * 停止生成、取消协程并释放会话资源
     */
    fun destroy() {
        // 停止生成
        stopGenerate()
        
        // 取消协程作用域
        presenterScope.cancel("ChatPresenter destroy")
        
        // 在新协程中重置并释放会话资源
        presenterScope.launch {
            chatSession.reset()
            chatSession.release()
        }
        
        // 在IO线程中进行最终清理
        CoroutineScope(Dispatchers.IO + SupervisorJob()).launch {
            try {
                if (::chatSession.isInitialized) {
                    Log.d(TAG, "Final cleanup: Resetting and releasing chat session.")
                    chatSession.reset()
                    chatSession.release()
                    Log.d(TAG, "Chat session reset and released during destroy.")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error during final chat session cleanup", e)
            }
        }
    }

    /**
     * 将响应保存到数据库
     * 
     * @param recentItem 最近的聊天数据项
     */
    fun saveResponseToDatabase(recentItem: ChatDataItem) {
        this.chatDataManager?.addChatData(sessionId, recentItem)
    }

    /**
     * 设置是否启用音频输出
     * 
     * @param enable 是否启用音频输出
     */
    fun setEnableAudioOutput(enable: Boolean) {
        this.chatSession.setEnableAudioOutput(enable)
    }

    /**
     * 生成监听器接口
     * 定义生成过程中的各种回调方法
     */
    interface GenerateListener {
        /**
         * 扩散模型生成进度回调
         * 
         * @param progress 生成进度信息
         * @param diffusionDestPath 扩散图像保存路径
         */
        fun onDiffusionGenerateProgress(progress: String?, diffusionDestPath: String?)
        
        /**
         * 开始生成回调
         */
        fun onGenerateStart()
        
        /**
         * 生成完成回调
         * 
         * @param benchMarkResult 包含生成结果的HashMap
         */
        fun onGenerateFinished(benchMarkResult: HashMap<String, Any>)
        
        /**
         * LLM模型生成进度回调
         * 
         * @param progress 生成进度信息
         * @param generateResultProcessor 结果处理器
         */
        fun onLlmGenerateProgress(progress: String?, generateResultProcessor: GenerateResultProcessor)
    }
}
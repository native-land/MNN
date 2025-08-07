#include <android/asset_manager_jni.h>
#include <android/bitmap.h>
#include <android/log.h>
#include <jni.h>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <mutex>
#include <ostream>
#include <sstream>
#include <mutex>
#include <ostream>
#include <sstream>
#include <mutex>
#include <string>
#include <chrono>
#include "mls_log.h"
#include "MNN/expr/ExecutorScope.hpp"
#include "nlohmann/json.hpp"
#include "llm_stream_buffer.hpp"
#include "utf8_stream_processor.hpp"
#include "llm_session.h"

using MNN::Transformer::Llm;
using json = nlohmann::json;

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    __android_log_print(ANDROID_LOG_DEBUG, "MNN_DEBUG", "JNI_OnLoad");
    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    __android_log_print(ANDROID_LOG_DEBUG, "MNN_DEBUG", "JNI_OnUnload");
}

JNIEXPORT jlong JNICALL Java_com_alibaba_mnnllm_android_llm_LlmSession_initNative(JNIEnv *env,
                                                                                  jobject thiz,
                                                                                  jstring modelDir,
                                                                                  jobject chat_history,
                                                                                  jstring mergeConfigStr,
                                                                                  jstring configJsonStr) {
    const char *model_dir = env->GetStringUTFChars(modelDir, nullptr);
    auto model_dir_str = std::string(model_dir);
    const char *config_json_cstr = env->GetStringUTFChars(configJsonStr, nullptr);
    const char *merged_config_cstr = env->GetStringUTFChars(mergeConfigStr, nullptr);
    json merged_config = json::parse(merged_config_cstr);
    json extra_json_config = json::parse(config_json_cstr);
    env->ReleaseStringUTFChars(modelDir, model_dir);
    env->ReleaseStringUTFChars(configJsonStr, config_json_cstr);
    env->ReleaseStringUTFChars(mergeConfigStr, merged_config_cstr);
    MNN_DEBUG("createLLM BeginLoad %s", model_dir);
    std::vector<std::string> history;
    history.clear();
    if (chat_history != nullptr) {
        jclass listClass = env->GetObjectClass(chat_history);
        jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
        jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
        jint listSize = env->CallIntMethod(chat_history, sizeMethod);
        for (jint i = 0; i < listSize; i++) {
            jobject element = env->CallObjectMethod(chat_history, getMethod, i);
            const char *elementCStr = env->GetStringUTFChars((jstring) element, nullptr);
            history.emplace_back(elementCStr);
            env->ReleaseStringUTFChars((jstring) element, elementCStr);
            env->DeleteLocalRef(element);
        }
    }
    auto llm_session = new mls::LlmSession(model_dir_str, merged_config, extra_json_config,
                                           history);
    llm_session->Load();
    MNN_DEBUG("createLLM EndLoad %ld ", reinterpret_cast<jlong>(llm_session));
    return reinterpret_cast<jlong>(llm_session);
}


/*
 * JNI方法：Java_com_alibaba_mnnllm_android_llm_LlmSession_submitNative
 * 功能：提交用户输入给LLM模型进行推理，并返回性能统计数据
 * 
 * 参数说明：
 * - env: JNI环境指针，用于与Java层交互
 * - thiz: Java层调用此方法的对象实例（LlmSession）
 * - llmPtr: LLM会话对象的指针，由initNative方法创建
 * - inputStr: 用户输入的文本内容
 * - keepHistory: 是否保留历史对话记录（在当前实现中未使用）
 * - progressListener: 进度监听器，用于接收模型生成的文本片段
 * 
 * 返回值：
 * - HashMap<String, Long>: 包含性能统计数据的Java HashMap对象
 */
JNIEXPORT jobject JNICALL Java_com_alibaba_mnnllm_android_llm_LlmSession_submitNative(JNIEnv *env,
                                                                                      jobject thiz,
                                                                                      jlong llmPtr,
                                                                                      jstring inputStr,
                                                                                      jboolean keepHistory,
                                                                                      jobject
                                                                                      progressListener) {
    // 将Java传递的long型指针转换为C++ LlmSession对象指针
    auto *llm = reinterpret_cast<mls::LlmSession *>(llmPtr);
    
    // 检查LLM对象是否有效
    if (!llm) {
        return env->NewStringUTF("Failed, Chat is not ready!");
    }
    
    // 获取输入字符串的C风格字符指针
    const char *input_str = env->GetStringUTFChars(inputStr, nullptr);
    
    // 获取进度监听器的类信息
    jclass progressListenerClass = env->GetObjectClass(progressListener);
    
    // 获取进度监听器的onProgress方法ID，该方法用于接收生成的文本片段
    jmethodID onProgressMethod = env->GetMethodID(progressListenerClass, "onProgress",
                                                  "(Ljava/lang/String;)Z");
    if (!onProgressMethod) {
        MNN_DEBUG("ProgressListener onProgress method not found.");
    }
    
    // 调用LLM的Response方法进行推理，传入输入文本和回调函数
    // 回调函数会在每次生成新文本时被调用，用于将结果返回给Java层
    auto *context = llm->Response(input_str, [&, progressListener, onProgressMethod](
            const std::string &response, bool is_eop) {
        // 如果进度监听器和方法都有效
        if (progressListener && onProgressMethod) {
            // 如果是结束标记，则传入null，否则创建Java字符串
            jstring javaString = is_eop ? nullptr : env->NewStringUTF(response.c_str());
            
            // 调用Java层的onProgress方法，传递生成的文本片段
            jboolean user_stop_requested = env->CallBooleanMethod(progressListener,
                                                                  onProgressMethod, javaString);
            
            // 释放局部引用
            env->DeleteLocalRef(javaString);
            
            // 返回用户是否请求停止生成
            return (bool) user_stop_requested;
        } else {
            // 如果没有监听器，继续生成
            return true;
        }
    });
    
    // 初始化性能统计变量
    int64_t prompt_len = 0;      // 提示词长度
    int64_t decode_len = 0;      // 解码长度
    int64_t vision_time = 0;     // 视觉处理时间
    int64_t audio_time = 0;      // 音频处理时间
    int64_t prefill_time = 0;    // 预填充时间
    int64_t decode_time = 0;     // 解码时间
    
    // 累加本次推理的性能统计数据
    prompt_len += context->prompt_len;
    decode_len += context->gen_seq_len;
    vision_time += context->vision_us;
    audio_time += context->audio_us;
    prefill_time += context->prefill_us;
    decode_time += context->decode_us;
    
    // 创建HashMap用于返回性能统计数据
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID hashMapInit = env->GetMethodID(hashMapClass, "<init>", "()V");
    jmethodID putMethod = env->GetMethodID(hashMapClass, "put",
                                           "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    jobject hashMap = env->NewObject(hashMapClass, hashMapInit);

    // 将各项性能统计数据添加到HashMap中
    env->CallObjectMethod(hashMap, putMethod, env->NewStringUTF("prompt_len"),
                          env->NewObject(env->FindClass("java/lang/Long"),
                                         env->GetMethodID(env->FindClass("java/lang/Long"),
                                                          "<init>", "(J)V"), prompt_len));
    env->CallObjectMethod(hashMap, putMethod, env->NewStringUTF("decode_len"),
                          env->NewObject(env->FindClass("java/lang/Long"),
                                         env->GetMethodID(env->FindClass("java/lang/Long"),
                                                          "<init>", "(J)V"), decode_len));
    env->CallObjectMethod(hashMap, putMethod, env->NewStringUTF("vision_time"),
                          env->NewObject(env->FindClass("java/lang/Long"),
                                         env->GetMethodID(env->FindClass("java/lang/Long"),
                                                          "<init>", "(J)V"), vision_time));
    env->CallObjectMethod(hashMap, putMethod, env->NewStringUTF("audio_time"),
                          env->NewObject(env->FindClass("java/lang/Long"),
                                         env->GetMethodID(env->FindClass("java/lang/Long"),
                                                          "<init>", "(J)V"), audio_time));
    env->CallObjectMethod(hashMap, putMethod, env->NewStringUTF("prefill_time"),
                          env->NewObject(env->FindClass("java/lang/Long"),
                                         env->GetMethodID(env->FindClass("java/lang/Long"),
                                                          "<init>", "(J)V"), prefill_time));
    env->CallObjectMethod(hashMap, putMethod, env->NewStringUTF("decode_time"),
                          env->NewObject(env->FindClass("java/lang/Long"),
                                         env->GetMethodID(env->FindClass("java/lang/Long"),
                                                          "<init>", "(J)V"), decode_time));
    
    // 释放输入字符串的资源
    env->ReleaseStringUTFChars(inputStr, input_str);
    
    // 返回包含性能统计数据的HashMap
    return hashMap;
}



// 新增：支持完整历史消息的JNI方法
JNIEXPORT jobject JNICALL Java_com_alibaba_mnnllm_android_llm_LlmSession_submitFullHistoryNative(
        JNIEnv *env,
        jobject thiz,
        jlong llmPtr,
        jobject historyList,  // List<Pair<String, String>>
        jobject progressListener
) {
    auto *llm = reinterpret_cast<mls::LlmSession *>(llmPtr);
    if (!llm) {
        return env->NewStringUTF("Failed, Chat is not ready!");
    }

    // 解析 Java List<Pair<String, String>> 到 C++ vector
    std::vector<mls::PromptItem> history;

    // 获取List类和相关方法
    jclass listClass = env->GetObjectClass(historyList);
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");

    jint listSize = env->CallIntMethod(historyList, sizeMethod);

    // 获取 Pair 类和相关方法，修改为 android.util.Pair
    jclass pairClass = env->FindClass("android/util/Pair");
    if (pairClass == nullptr) {
        MNN_DEBUG("Failed to find android.util.Pair class");
        return env->NewStringUTF("Failed to find android.util.Pair class");
    }
    // 使用 GetFieldID 访问 first 字段
    jfieldID firstField = env->GetFieldID(pairClass, "first", "Ljava/lang/Object;");
    // 使用 GetFieldID 访问 second 字段
    jfieldID secondField = env->GetFieldID(pairClass, "second", "Ljava/lang/Object;");

    // 遍历List，提取每个Pair
    for (jint i = 0; i < listSize; i++) {
        jobject pairObj = env->CallObjectMethod(historyList, getMethod, i);
        if (pairObj == nullptr) {
            continue;
        }
        // 使用 GetObjectField 访问 first 字段
        jobject roleObj = env->GetObjectField(pairObj, firstField);
        // 使用 GetObjectField 访问 second 字段
        jobject contentObj = env->GetObjectField(pairObj, secondField);

        const char *role = nullptr;
        const char *content = nullptr;
        if (roleObj != nullptr) {
            role = env->GetStringUTFChars((jstring) roleObj, nullptr);
        }
        if (contentObj != nullptr) {
            content = env->GetStringUTFChars((jstring) contentObj, nullptr);
        }

        if (role && content) {
            history.emplace_back(std::string(role), std::string(content));
        }

        if (role) {
            env->ReleaseStringUTFChars((jstring) roleObj, role);
        }
        if (content) {
            env->ReleaseStringUTFChars((jstring) contentObj, content);
        }
        env->DeleteLocalRef(pairObj);
        if (roleObj) {
            env->DeleteLocalRef(roleObj);
        }
        if (contentObj) {
            env->DeleteLocalRef(contentObj);
        }
    }

    // 设置进度回调
    jclass progressListenerClass = env->GetObjectClass(progressListener);
    jmethodID onProgressMethod = env->GetMethodID(progressListenerClass, "onProgress","(Ljava/lang/String;)Z");

    if (!onProgressMethod) {
        MNN_DEBUG("ProgressListener onProgress method not found.");
    }

    // 调用API服务推理方法
    auto *context = llm->ResponseWithHistory(history, [&, progressListener, onProgressMethod](
            const std::string &response, bool is_eop) {
        if (progressListener && onProgressMethod) {
            jstring javaString = is_eop ? nullptr : env->NewStringUTF(response.c_str());
            jboolean user_stop_requested = env->CallBooleanMethod(progressListener,onProgressMethod, javaString);
            if (javaString) {
                env->DeleteLocalRef(javaString);
            }
            return (bool) user_stop_requested;
        } else {
            return true;
        }
    });

    // 构建返回结果
    int64_t prompt_len = 0;
    int64_t decode_len = 0;
    int64_t vision_time = 0;
    int64_t audio_time = 0;
    int64_t prefill_time = 0;
    int64_t decode_time = 0;

    if (context) {
        prompt_len += context->prompt_len;
        decode_len += context->gen_seq_len;
        vision_time += context->vision_us;
        audio_time += context->audio_us;
        prefill_time += context->prefill_us;
        decode_time += context->decode_us;
    }

    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID hashMapInit = env->GetMethodID(hashMapClass, "<init>", "()V");
    jmethodID hashMapPut = env->GetMethodID(hashMapClass, "put",
                                            "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    jobject hashMap = env->NewObject(hashMapClass, hashMapInit);

    // 添加统计信息
    jclass longClass = env->FindClass("java/lang/Long");
    jmethodID longInit = env->GetMethodID(longClass, "<init>", "(J)V");

    env->CallObjectMethod(hashMap, hashMapPut, env->NewStringUTF("prompt_len"),
                          env->NewObject(longClass, longInit, prompt_len));
    env->CallObjectMethod(hashMap, hashMapPut, env->NewStringUTF("decode_len"),
                          env->NewObject(longClass, longInit, decode_len));
    env->CallObjectMethod(hashMap, hashMapPut, env->NewStringUTF("vision_time"),
                          env->NewObject(longClass, longInit, vision_time));
    env->CallObjectMethod(hashMap, hashMapPut, env->NewStringUTF("audio_time"),
                          env->NewObject(longClass, longInit, audio_time));
    env->CallObjectMethod(hashMap, hashMapPut, env->NewStringUTF("prefill_time"),
                          env->NewObject(longClass, longInit, prefill_time));
    env->CallObjectMethod(hashMap, hashMapPut, env->NewStringUTF("decode_time"),
                          env->NewObject(longClass, longInit, decode_time));

    return hashMap;
}



JNIEXPORT void JNICALL
Java_com_alibaba_mnnllm_android_llm_LlmSession_resetNative(JNIEnv *env, jobject thiz,
                                                           jlong object_ptr) {
    auto *llm = reinterpret_cast<mls::LlmSession *>(object_ptr);
    if (llm) {
        MNN_DEBUG("RESET");
        llm->Reset();
    }
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_alibaba_mnnllm_android_llm_LlmSession_setWavformCallbackNative(
        JNIEnv *env, jobject thiz, jlong instance_id, jobject listener) {

    if (instance_id == 0 || !listener) {
        return JNI_FALSE;
    }
    auto *session = reinterpret_cast<mls::LlmSession *>(instance_id);
    jobject global_ref = env->NewGlobalRef(listener);
    JavaVM *jvm;
    env->GetJavaVM(&jvm);
    session->SetWavformCallback(
            [jvm, global_ref](const float *data, size_t size, bool is_end) -> bool {
                bool needDetach = false;
                JNIEnv *env;
                if (jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
                    jvm->AttachCurrentThread(&env, NULL);
                    needDetach = true;
                }
                jvm->AttachCurrentThread(&env, NULL);
                jclass listenerClass = env->GetObjectClass(global_ref);
                jmethodID onAudioDataMethod = env->GetMethodID(listenerClass, "onAudioData",
                                                               "([FZ)Z");
                jfloatArray audioDataArray = env->NewFloatArray(size);
                env->SetFloatArrayRegion(audioDataArray, 0, size, data);
                jboolean result = env->CallBooleanMethod(global_ref, onAudioDataMethod,
                                                         audioDataArray, is_end);
                env->DeleteLocalRef(audioDataArray);
                env->DeleteLocalRef(listenerClass);
                if (needDetach) {
                    jvm->DetachCurrentThread();
                }
                return result == JNI_TRUE;
            });

    return JNI_TRUE;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_alibaba_mnnllm_android_llm_LlmSession_getDebugInfoNative(JNIEnv *env, jobject thiz,
                                                                  jlong objecPtr) {
    auto *llm = reinterpret_cast<mls::LlmSession *>(objecPtr);
    if (llm == nullptr) {
        return env->NewStringUTF("");
    }
    return env->NewStringUTF(llm->getDebugInfo().c_str());
}

JNIEXPORT void JNICALL Java_com_alibaba_mnnllm_android_llm_LlmSession_releaseNative(JNIEnv *env,
                                                                                    jobject thiz,
                                                                                    jlong objecPtr) {
    MNN_DEBUG("Java_com_alibaba_mnnllm_android_llm_LlmSession_releaseNative\n");
    auto *llm = reinterpret_cast<mls::LlmSession *>(objecPtr);
    delete llm;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_alibaba_mnnllm_android_llm_LlmSession_updateMaxNewTokensNative(JNIEnv *env, jobject thiz,
                                                                        jlong llm_ptr,
                                                                        jint max_new_tokens) {
    auto *llm = reinterpret_cast<mls::LlmSession *>(llm_ptr);
    if (llm) {
        llm->SetMaxNewTokens(max_new_tokens);
    }

}
extern "C"
JNIEXPORT void JNICALL
Java_com_alibaba_mnnllm_android_llm_LlmSession_updateSystemPromptNative(JNIEnv *env, jobject thiz,
                                                                        jlong llm_ptr,
                                                                        jstring system_promp_j) {
    auto *llm = reinterpret_cast<mls::LlmSession *>(llm_ptr);
    const char *system_prompt_cstr = env->GetStringUTFChars(system_promp_j, nullptr);
    if (llm) {
        llm->setSystemPrompt(system_prompt_cstr);
    }
    env->ReleaseStringUTFChars(system_promp_j, system_prompt_cstr);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_alibaba_mnnllm_android_llm_LlmSession_updateAssistantPromptNative(JNIEnv *env,
                                                                           jobject thiz,
                                                                           jlong llm_ptr,
                                                                           jstring assistant_prompt_j) {
    auto *llm = reinterpret_cast<mls::LlmSession *>(llm_ptr);
    const char *assistant_prompt_cstr = env->GetStringUTFChars(assistant_prompt_j, nullptr);
    if (llm) {
        llm->SetAssistantPrompt(assistant_prompt_cstr);
    }
    env->ReleaseStringUTFChars(assistant_prompt_j, assistant_prompt_cstr);
}
}
extern "C"
JNIEXPORT void JNICALL
Java_com_alibaba_mnnllm_android_llm_LlmSession_updateEnableAudioOutputNative(JNIEnv *env,jobject thiz, jlong llm_ptr, jboolean enable) {
    auto *llm = reinterpret_cast<mls::LlmSession *>(llm_ptr);
    if (llm) {
        llm->enableAudioOutput((bool)enable);
    }
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_alibaba_mnnllm_android_llm_LlmSession_getSystemPromptNative(JNIEnv *env, jobject thiz,
                                                                     jlong llm_ptr) {
    auto *llm = reinterpret_cast<mls::LlmSession *>(llm_ptr);
    if (llm) {
        std::string system_prompt = llm->getSystemPrompt();
        return env->NewStringUTF(system_prompt.c_str());
    }
    return nullptr;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_alibaba_mnnllm_android_llm_LlmSession_clearHistoryNative(JNIEnv *env, jobject thiz,
                                                                  jlong llm_ptr) {
    auto *llm = reinterpret_cast<mls::LlmSession *>(llm_ptr);
    if (llm) {
        llm->clearHistory();
    }
}
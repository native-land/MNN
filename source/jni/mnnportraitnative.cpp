//
//  mnnportraitnative.cpp
//  MNN
//
//  Created by MNN on 2019/01/29.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#include <android/bitmap.h>
#include <jni.h>
#include <string.h>
#include <MNN/ImageProcess.hpp>
#include <MNN/Interpreter.hpp>
#include <MNN/Tensor.hpp>
#include <memory>

/**
 * 将分割掩码转换为像素数组的 JNI 接口函数
 * 
 * 该函数的主要作用是将神经网络输出的浮点型分割掩码数据转换为 Android 可以直接使用的
 * 整型像素数组，用于人像分割功能。
 * 
 * @param env JNI环境指针
 * @param jclazz Java类引用
 * @param jmaskarray 包含分割掩码数据的浮点数组
 * @param length 掩码数据的长度
 * @return 转换后的整型像素数组
 */
extern "C" JNIEXPORT jintArray JNICALL
Java_com_taobao_android_mnn_MNNPortraitNative_nativeConvertMaskToPixelsMultiChannels(JNIEnv *env, jclass jclazz,
                                                                                     jfloatArray jmaskarray,
                                                                                     jint length) {
    // 获取 Java 浮点数组的指针
    float *scores = (float *)env->GetFloatArrayElements(jmaskarray, 0);

    // 创建用于存储结果的整型数组
    int dst32[length];

#if 0
    // 被注释掉的代码段：使用指数函数进行像素值映射的方法
    for (int l = 0; l < length; l++) {
        int* dst = dst32 + l;
        float* src = scores + l;
        float max = scores[l];
        float min = scores[l];
        // 遍历21个通道，找到最大值和最小值
        for(int c = 0; c < 21; c++){
            float data = src[c*length];
            if(max < data){
                max = data;
            }
            if(min > data){
                min = data;
            }
        }

        // 获取第15个通道的数据
        unsigned data = src[15*length];
        // 计算映射范围
        float range = 255.0f / (exp(max) - exp(min));
        // 使用指数函数映射到0-255范围
        float result = (exp(data) - exp(min)) * range;

        // 确保结果在0-255范围内
        unsigned result_uint8 = result > 255.0f ? 255 : result;

        // 设置ARGB值（灰度图）
        unsigned a = result_uint8;
        unsigned r = a;
        unsigned g = a;
        unsigned b = a;
        // 组合成ARGB格式的整数
        dst[0] = a << 24 | r << 16 | g << 8 | b;
    }
#else
    // 当前使用的代码段：基于最大值比较的二值化方法
    for (int l = 0; l < length; l++) {
        // 指向当前处理位置的指针
        int *dst   = dst32 + l;
        float *src = scores + l;
        
        // 初始化最大值为第一个通道的值
        float max  = scores[l];
        
        // 遍历21个通道，找到最大值
        for (int c = 0; c < 21; c++) {
            if (max < src[c * length]) {
                max = src[c * length];
            }
        }
        
        // 判断第15个通道的值是否为最大值
        // 如果是最大值，则设置为透明（0），否则设置为不透明（255）
        unsigned a = src[15 * length] == max ? 0 : 255;
        unsigned r = a;
        unsigned g = a;
        unsigned b = a;
        
        // 将ARGB值组合成一个整数（ARGB格式）
        // A(Alpha)占最高8位，R(Green)次之，G(Blue)再次，B(Red)最低
        dst[0] = a << 24 | r << 16 | g << 8 | b;
    }
#endif
    
    // 创建新的 Java 整型数组
    jintArray arr = env->NewIntArray(length);
    // 将 C++ 数组内容复制到 Java 数组中
    env->SetIntArrayRegion(arr, 0, length, dst32);

    // 释放之前获取的 Java 数组资源
    env->ReleaseFloatArrayElements(jmaskarray, scores, 0);

    // 返回转换后的数组
    return arr;
}

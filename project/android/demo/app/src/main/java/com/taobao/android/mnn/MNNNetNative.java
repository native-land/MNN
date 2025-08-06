package com.taobao.android.mnn;

import android.graphics.Bitmap;
import android.util.Log;

import com.taobao.android.utils.Common;

/**
 * MNN原生接口封装类
 * 提供了MNN神经网络推理引擎的底层JNI接口封装
 */
public class MNNNetNative {
    /**
     * 加载GPU相关库文件
     * @param name 库名
     */
    static void loadGpuLibrary(String name) {
        try {
            System.loadLibrary(name);
        } catch (Throwable ce) {
            Log.w(Common.TAG, "加载MNN " + name + " GPU so库异常=%s", ce);
        }
    }
    
    // 静态初始化块，用于加载MNN相关库文件
    static {
        System.loadLibrary("MNN");           // 加载MNN核心库
        loadGpuLibrary("MNN_Vulkan");        // 加载Vulkan GPU支持库
        loadGpuLibrary("MNN_CL");            // 加载OpenCL GPU支持库
        loadGpuLibrary("MNN_GL");            // 加载OpenGL GPU支持库
        System.loadLibrary("mnncore");       // 加载MNN核心功能库
    }

    //==================== 网络模型相关接口 ====================
    /**
     * 从文件创建神经网络
     * @param modelName 模型文件路径
     * @return 网络指针
     */
    protected static native long nativeCreateNetFromFile(String modelName);

    /**
     * 从缓冲区数据创建神经网络
     * @param buffer 模型数据缓冲区
     * @return 网络指针
     */
    protected static native long nativeCreateNetFromBuffer(byte[] buffer);

    /**
     * 释放神经网络资源
     * @param netPtr 网络指针
     * @return 释放结果
     */
    protected static native long nativeReleaseNet(long netPtr);


    //==================== 会话相关接口 ====================
    /**
     * 创建推理会话
     * @param netPtr 网络指针
     * @param forwardType 前向推理类型
     * @param numThread 线程数
     * @param saveTensors 需要保存的张量名称数组
     * @param outputTensors 输出张量名称数组
     * @return 会话指针
     */
    protected static native long nativeCreateSession(long netPtr, int forwardType, int numThread, String[] saveTensors, String[] outputTensors);

    /**
     * 释放推理会话资源
     * @param netPtr 网络指针
     * @param sessionPtr 会话指针
     */
    protected static native void nativeReleaseSession(long netPtr, long sessionPtr);

    /**
     * 运行推理会话
     * @param netPtr 网络指针
     * @param sessionPtr 会话指针
     * @return 运行结果状态码
     */
    protected static native int nativeRunSession(long netPtr, long sessionPtr);

    /**
     * 运行带回调的推理会话
     * @param netPtr 网络指针
     * @param sessionPtr 会话指针
     * @param nameArray 张量名称数组
     * @param tensorAddr 张量地址数组
     * @return 运行结果状态码
     */
    protected static native int nativeRunSessionWithCallback(long netPtr, long sessionPtr, String[] nameArray, long[] tensorAddr);

    /**
     * 重新调整会话形状
     * @param netPtr 网络指针
     * @param sessionPtr 会话指针
     * @return 调整结果状态码
     */
    protected static native int nativeReshapeSession(long netPtr, long sessionPtr);

    /**
     * 获取会话输入张量
     * @param netPtr 网络指针
     * @param sessionPtr 会话指针
     * @param name 输入张量名称
     * @return 输入张量指针
     */
    protected static native long nativeGetSessionInput(long netPtr, long sessionPtr, String name);

    /**
     * 获取会话输出张量
     * @param netPtr 网络指针
     * @param sessionPtr 会话指针
     * @param name 输出张量名称
     * @return 输出张量指针
     */
    protected static native long nativeGetSessionOutput(long netPtr, long sessionPtr, String name);


    //==================== 张量相关接口 ====================
    /**
     * 重新调整张量形状
     * @param netPtr 网络指针
     * @param tensorPtr 张量指针
     * @param dims 新的维度数组
     */
    protected static native void nativeReshapeTensor(long netPtr, long tensorPtr, int[] dims);

    /**
     * 获取张量维度信息
     * @param tensorPtr 张量指针
     * @return 维度数组
     */
    protected static native int[] nativeTensorGetDimensions(long tensorPtr);

    /**
     * 设置输入整型数据
     * @param netPtr 网络指针
     * @param tensorPtr 张量指针
     * @param data 整型数据数组
     */
    protected static native void nativeSetInputIntData(long netPtr, long tensorPtr, int[] data);

    /**
     * 设置输入浮点型数据
     * @param netPtr 网络指针
     * @param tensorPtr 张量指针
     * @param data 浮点型数据数组
     */
    protected static native void nativeSetInputFloatData(long netPtr, long tensorPtr, float[] data);


    /**
     * 获取张量浮点型数据
     * @param tensorPtr 张量指针
     * @param dest 目标浮点型数组，如果为null则返回数据长度
     * @return 数据长度或操作结果
     */
    protected static native int nativeTensorGetData(long tensorPtr, float[] dest);

    /**
     * 获取张量整型数据
     * @param tensorPtr 张量指针
     * @param dest 目标整型数组
     * @return 数据长度或操作结果
     */
    protected static native int nativeTensorGetIntData(long tensorPtr, int[] dest);

    /**
     * 获取张量UINT8数据
     * @param tensorPtr 张量指针
     * @param dest 目标字节数组
     * @return 数据长度或操作结果
     */
    protected static native int nativeTensorGetUINT8Data(long tensorPtr, byte[] dest);


    //==================== 图像处理相关接口 ====================
    /**
     * 将Bitmap转换为张量
     * @param srcBitmap 源Bitmap图像
     * @param tensorPtr 目标张量指针
     * @param destFormat 目标格式
     * @param filterType 滤镜类型
     * @param wrap 包装模式
     * @param matrixValue 变换矩阵值
     * @param mean 均值数组
     * @param normal 归一化数组
     * @return 转换是否成功
     */
    protected static native boolean nativeConvertBitmapToTensor(Bitmap srcBitmap, long tensorPtr, int destFormat, int filterType, int wrap, float[] matrixValue, float[] mean, float[] normal);

    /**
     * 将缓冲区数据转换为张量
     * @param bufferData 源数据缓冲区
     * @param width 图像宽度
     * @param height 图像高度
     * @param tensorPtr 目标张量指针
     * @param srcFormat 源格式
     * @param destFormat 目标格式
     * @param filterType 滤镜类型
     * @param wrap 包装模式
     * @param matrixValue 变换矩阵值
     * @param mean 均值数组
     * @param normal 归一化数组
     * @return 转换是否成功
     */
    protected static native boolean nativeConvertBufferToTensor(byte[] bufferData, int width, int height, long tensorPtr,
                                                                int srcFormat, int destFormat, int filterType, int wrap, float[] matrixValue, float[] mean, float[] normal);

}

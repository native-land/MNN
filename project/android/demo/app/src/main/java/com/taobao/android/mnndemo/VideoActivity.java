package com.taobao.android.mnndemo;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Matrix;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.ViewStub;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.taobao.android.mnn.MNNForwardType;
import com.taobao.android.mnn.MNNImageProcess;
import com.taobao.android.mnn.MNNNetInstance;
import com.taobao.android.utils.Common;
import com.taobao.android.utils.TxtFileReader;

import java.text.DecimalFormat;
import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * VideoActivity类用于实现基于摄像头预览的实时图像分类功能
 * 支持多种模型（MobileNet和SqueezeNet）和多种推理后端（CPU、OpenCL、OpenGL、Vulkan）
 * 通过MNN框架进行神经网络推理，实现实时物体识别
 */
public class VideoActivity extends AppCompatActivity implements AdapterView.OnItemSelectedListener {

    // 日志标签
    private final String TAG = "VideoActivity";
    // 最大分类数量限制
    private final int MAX_CLZ_SIZE = 1000;

    // MobileNet模型文件名和标签文件名
    private final String MobileModelFileName = "MobileNet/v2/mobilenet_v2.caffe.mnn";
    private final String MobileWordsFileName = "MobileNet/synset_words.txt";

    // SqueezeNet模型文件名和标签文件名
    private final String SqueezeModelFileName = "SqueezeNet/v1.1/squeezenet_v1.1.caffe.mnn";
    private final String SqueezeWordsFileName = "SqueezeNet/squeezenet.txt";

    // MobileNet模型路径和标签列表
    private String mMobileModelPath;
    private List<String> mMobileTaiWords;
    // SqueezeNet模型路径和标签列表
    private String mSqueezeModelPath;
    private List<String> mSqueezeTaiWords;

    // 当前选中的模型索引（0: MobileNet, 1: SqueezeNet）
    private int mSelectedModelIndex;
    // MNN网络配置（线程数、推理后端等）
    private final MNNNetInstance.Config mConfig = new MNNNetInstance.Config();

    // 摄像头预览视图
    private CameraView mCameraView;
    // 推理后端选择下拉框（CPU/OpenCL/OpenGL/Vulkan）
    private Spinner mForwardTypeSpinner;
    // 线程数选择下拉框
    private Spinner mThreadNumSpinner;
    // 模型选择下拉框（MobileNet/SqueezeNet）
    private Spinner mModelSpinner;
    // 更多示例选择下拉框
    private Spinner mMoreDemoSpinner;

    // 显示前三个识别结果的文本视图
    private TextView mFirstResult;
    private TextView mSecondResult;
    private TextView mThirdResult;
    // 显示推理耗时的文本视图
    private TextView mTimeTextView;

    // MobileNet模型输入尺寸（宽x高）
    private final int MobileInputWidth = 224;
    private final int MobileInputHeight = 224;

    // SqueezeNet模型输入尺寸（宽x高）
    private final int SqueezeInputWidth = 227;
    private final int SqueezeInputHeight = 227;

    // 用于执行MNN推理的后台线程和处理器
    HandlerThread mThread;
    Handler mHandle;

    // UI渲染锁定标志，防止在模型加载过程中更新UI
    private AtomicBoolean mLockUIRender = new AtomicBoolean(false);
    // 帧丢弃标志，防止处理队列过长导致延迟
    private AtomicBoolean mDrop = new AtomicBoolean(false);

    // MNN网络实例、会话和输入张量
    private MNNNetInstance mNetInstance;
    private MNNNetInstance.Session mSession;
    private MNNNetInstance.Session.Tensor mInputTensor;

    // 屏幕旋转角度（0/90/180/270度）
    private int mRotateDegree;

    /**
     * 监听屏幕旋转
     * 通过OrientationEventListener监听设备方向变化，用于调整摄像头预览方向
     */
    void detectScreenRotate() {
        // 创建方向监听器，监听设备方向变化
        OrientationEventListener orientationListener = new OrientationEventListener(this,
                SensorManager.SENSOR_DELAY_NORMAL) {
            @Override
            public void onOrientationChanged(int orientation) {
                // 当设备平放时，无法获取有效角度，直接返回
                if (orientation == OrientationEventListener.ORIENTATION_UNKNOWN) {
                    return;
                }

                // 将角度标准化到0/90/180/270度四个方向
                // 通过(orientation + 45) / 90 * 90的方式实现四舍五入到最近的90度倍数
                orientation = (orientation + 45) / 90 * 90;
                mRotateDegree = orientation % 360;
            }
        };

        // 检查设备是否支持方向检测，如果支持则启用监听器，否则禁用
        if (orientationListener.canDetectOrientation()) {
            orientationListener.enable();
        } else {
            orientationListener.disable();
        }
    }

    /**
     * 准备模型文件
     * 将assets目录中的模型文件和标签文件复制到缓存目录，以便后续加载
     */
    private void prepareModels() {
        // 准备MobileNet模型文件
        mMobileModelPath = getCacheDir() + "mobilenet_v1.caffe.mnn";
        try {
            // 从assets目录复制MobileNet模型文件到缓存目录
            Common.copyAssetResource2File(getBaseContext(), MobileModelFileName, mMobileModelPath);
            // 加载MobileNet模型的标签文件
            mMobileTaiWords = TxtFileReader.getUniqueUrls(getBaseContext(), MobileWordsFileName, Integer.MAX_VALUE);
        } catch (Throwable e) {
            throw new RuntimeException(e);
        }

        // 准备SqueezeNet模型文件
        mSqueezeModelPath = getCacheDir() + "squeezenet_v1.1.caffe.mnn";
        try {
            // 从assets目录复制SqueezeNet模型文件到缓存目录
            Common.copyAssetResource2File(getBaseContext(), SqueezeModelFileName, mSqueezeModelPath);
            // 加载SqueezeNet模型的标签文件
            mSqueezeTaiWords = TxtFileReader.getUniqueUrls(getBaseContext(), SqueezeWordsFileName, Integer.MAX_VALUE);
        } catch (Throwable e) {
            throw new RuntimeException(e);
        }
    }


    /**
     * 准备神经网络
     * 根据当前选择的模型索引加载对应的MNN模型，并创建会话和输入张量
     */
    private void prepareNet() {
        // 释放之前的会话资源
        if (null != mSession) {
            mSession.release();
            mSession = null;
        }
        // 释放之前的网络实例资源
        if (mNetInstance != null) {
            mNetInstance.release();
            mNetInstance = null;
        }

        // 根据选中的模型索引选择对应的模型路径
        String modelPath = mMobileModelPath;
        if (mSelectedModelIndex == 0) {
            modelPath = mMobileModelPath;  // MobileNet模型
        } else if (mSelectedModelIndex == 1) {
            modelPath = mSqueezeModelPath; // SqueezeNet模型
        }

        // 从文件创建MNN网络实例
        mNetInstance = MNNNetInstance.createFromFile(modelPath);

        // 根据配置创建会话
        mSession = mNetInstance.createSession(mConfig);

        // 获取输入张量
        mInputTensor = mSession.getInput(null);

        // 设置输入张量的维度，强制batch size为1
        int[] dimensions = mInputTensor.getDimensions();
        dimensions[0] = 1; // force batch = 1  NCHW  [batch, channels, height, width]
        mInputTensor.reshape(dimensions);
        mSession.reshape();

        // 解锁UI渲染，允许更新界面
        mLockUIRender.set(false);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 保持屏幕常亮
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        // 设置全屏模式
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_main);

        // 监听屏幕旋转
        detectScreenRotate();

        // 初始化默认配置
        mSelectedModelIndex = 0;  // 默认使用MobileNet模型
        mConfig.numThread = 4;    // 默认使用4个线程
        mConfig.forwardType = MNNForwardType.FORWARD_CPU.type;  // 默认使用CPU推理

        // 准备MNN模型文件
        prepareModels();

        // 初始化界面控件
        mForwardTypeSpinner = findViewById(R.id.forwardTypeSpinner);
        mThreadNumSpinner = findViewById(R.id.threadsSpinner);
        mThreadNumSpinner.setSelection(2);  // 默认选择第3项（索引为2）
        mModelSpinner = findViewById(R.id.modelTypeSpinner);
        mMoreDemoSpinner = findViewById(R.id.MoreDemo);

        // 初始化结果显示控件
        mFirstResult = findViewById(R.id.firstResult);
        mSecondResult = findViewById(R.id.secondResult);
        mThirdResult = findViewById(R.id.thirdResult);
        mTimeTextView = findViewById(R.id.timeTextView);

        // 设置下拉框选项改变监听器
        mForwardTypeSpinner.setOnItemSelectedListener(VideoActivity.this);
        mThreadNumSpinner.setOnItemSelectedListener(VideoActivity.this);
        mModelSpinner.setOnItemSelectedListener(VideoActivity.this);
        mMoreDemoSpinner.setOnItemSelectedListener(VideoActivity.this);

        // 初始化子线程处理器，锁定UI渲染以准备网络
        mLockUIRender.set(true);
        clearUIForPrepareNet();

        // 检查并请求摄像头权限
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                // 请求摄像头权限
                requestPermissions(new String[]{Manifest.permission.CAMERA}, 10);
            } else {
                // 已有权限，处理预览回调
                handlePreViewCallBack();
            }
        } else {
            // Android 6.0以下版本直接处理预览回调
            handlePreViewCallBack();
        }

        // 创建并启动后台线程用于执行MNN推理
        mThread = new HandlerThread("MNNNet");
        mThread.start();
        mHandle = new Handler(mThread.getLooper());

        // 在后台线程中准备神经网络
        mHandle.post(new Runnable() {
            @Override
            public void run() {
                prepareNet();
            }
        });

    }


    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        // 处理权限请求结果
        if (10 == requestCode) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // 权限获取成功，处理预览回调
                handlePreViewCallBack();
            } else {
                // 权限获取失败，提示用户
                Toast.makeText(this, "没有获得必要的权限", Toast.LENGTH_SHORT).show();
            }
        }

    }

    /**
     * 处理摄像头预览回调
     * 初始化摄像头视图并设置预览回调，处理预览帧数据进行神经网络推理
     */
    private void handlePreViewCallBack() {
        // 加载摄像头视图
        ViewStub stub = (ViewStub) findViewById(R.id.stub);
        stub.inflate();

        mCameraView = (CameraView) findViewById(R.id.camera_view);

        // 设置摄像头预览回调
        mCameraView.setPreviewCallback(new CameraView.PreviewCallback() {
            @Override
            public void onGetPreviewOptimalSize(int optimalWidth, int optimalHeight) {
                // 根据屏幕尺寸调整视频预览大小
                DisplayMetrics metric = new DisplayMetrics();
                getWindowManager().getDefaultDisplay().getMetrics(metric);
                int fixedVideoHeight = metric.widthPixels * optimalWidth / optimalHeight;

                FrameLayout layoutVideo = findViewById(R.id.videoLayout);
                RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) layoutVideo.getLayoutParams();
                params.height = fixedVideoHeight;
                layoutVideo.setLayoutParams(params);
            }

            @Override
            public void onPreviewFrame(final byte[] data, final int imageWidth, final int imageHeight, final int angle) {
                // 如果UI渲染被锁定，直接返回
                if (mLockUIRender.get()) {
                    return;
                }

                // 处理帧丢弃逻辑，防止处理队列过长
                if (mDrop.get()) {
                    Log.w(TAG, "drop frame , net running too slow !!");
                } else {
                    mDrop.set(true);
                    // 在后台线程中处理预览帧数据
                    mHandle.post(new Runnable() {
                        @Override
                        public void run() {
                            mDrop.set(false);
                            // 如果UI渲染被锁定，直接返回
                            if (mLockUIRender.get()) {
                                return;
                            }

                            // 计算校正角度，基于摄像头方向和设备旋转角度
                            int needRotateAngle = (angle + mRotateDegree) % 360;

                            // 创建图像处理配置
                            final MNNImageProcess.Config config = new MNNImageProcess.Config();
                            
                            // 根据选中的模型索引设置不同的图像处理参数
                            if (mSelectedModelIndex == 0) {
                                // MobileNet模型的归一化参数
                                config.mean = new float[]{103.94f, 116.78f, 123.68f};
                                config.normal = new float[]{0.017f, 0.017f, 0.017f};
                                config.source = MNNImageProcess.Format.YUV_NV21; // 输入源格式
                                config.dest = MNNImageProcess.Format.BGR;        // 输入数据格式

                                // 矩阵变换：目标到源
                                Matrix matrix = new Matrix();
                                matrix.postScale(MobileInputWidth / (float) imageWidth, MobileInputHeight / (float) imageHeight);
                                matrix.postRotate(needRotateAngle, MobileInputWidth / 2, MobileInputHeight / 2);
                                matrix.invert(matrix);

                                // 将图像数据转换为输入张量
                                MNNImageProcess.convertBuffer(data, imageWidth, imageHeight, mInputTensor, config, matrix);

                            } else if (mSelectedModelIndex == 1) {
                                // SqueezeNet模型的图像处理参数
                                config.source = MNNImageProcess.Format.YUV_NV21; // 输入源格式
                                config.dest = MNNImageProcess.Format.BGR;        // 输入数据格式

                                // 矩阵变换：目标到源
                                final Matrix matrix = new Matrix();
                                matrix.postScale(SqueezeInputWidth / (float) (float) imageWidth, SqueezeInputHeight / (float) imageHeight);
                                matrix.postRotate(needRotateAngle, SqueezeInputWidth / 2, SqueezeInputWidth / 2);
                                matrix.invert(matrix);

                                // 将图像数据转换为输入张量
                                MNNImageProcess.convertBuffer(data, imageWidth, imageHeight, mInputTensor, config, matrix);
                            }

                            // 记录推理开始时间
                            final long startTimestamp = System.nanoTime();
                            
                            // 执行神经网络推理
                            mSession.run();

                            // 获取输出张量
                            MNNNetInstance.Session.Tensor output = mSession.getOutput(null);

                            // 获取浮点型推理结果
                            float[] result = output.getFloatData();
                            // 计算推理耗时
                            final long endTimestamp = System.nanoTime();
                            final float inferenceTimeCost = (endTimestamp - startTimestamp) / 1000000.0f;

                            // 检查结果大小是否超出限制
                            if (result.length > MAX_CLZ_SIZE) {
                                Log.w(TAG, "session result too big (" + result.length + "), model incorrect ?");
                            }

                            // 筛选置信度大于0.01的结果
                            final List<Map.Entry<Integer, Float>> maybes = new ArrayList<>();
                            for (int i = 0; i < result.length; i++) {
                                float confidence = result[i];
                                if (confidence > 0.01) {
                                    maybes.add(new AbstractMap.SimpleEntry<Integer, Float>(i, confidence));
                                }
                            }

                            // 按置信度降序排序
                            Collections.sort(maybes, new Comparator<Map.Entry<Integer, Float>>() {
                                @Override
                                public int compare(Map.Entry<Integer, Float> o1, Map.Entry<Integer, Float> o2) {
                                    if (Math.abs(o1.getValue() - o2.getValue()) <= Float.MIN_NORMAL) {
                                        return 0;
                                    }
                                    return o1.getValue() > o2.getValue() ? -1 : 1;
                                }
                            });

                            // 在UI线程中更新识别结果
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    // 如果没有识别结果，显示"no data"
                                    if (maybes.size() == 0) {
                                        mFirstResult.setText("no data");
                                        mSecondResult.setText("");
                                        mThirdResult.setText("");
                                    }
                                    
                                    // 显示第一个识别结果
                                    if (maybes.size() > 0) {
                                        // 根据置信度设置文字颜色
                                        mFirstResult.setTextColor(maybes.get(0).getValue() > 0.2 ? Color.BLACK : Color.parseColor("#a4a4a4"));
                                        final Integer iKey = maybes.get(0).getKey();
                                        final Float fValue = maybes.get(0).getValue();
                                        String strWord = "unknown";
                                        
                                        // 根据模型索引获取对应的标签
                                        if (0 == mSelectedModelIndex) {
                                            if (iKey < mMobileTaiWords.size()) {
                                                strWord = mMobileTaiWords.get(iKey);
                                            }
                                        } else {
                                            if (iKey < mSqueezeTaiWords.size()) {
                                                strWord = mSqueezeTaiWords.get(iKey);
                                            }
                                        }
                                        
                                        // 处理标签显示格式
                                        final String resKey = mSelectedModelIndex == 1 ? strWord.length() >= 10 ? strWord.substring(10) : strWord : strWord;
                                        mFirstResult.setText(resKey + "：" + new DecimalFormat("0.00").format(fValue));
                                    }
                                    
                                    // 显示第二个识别结果
                                    if (maybes.size() > 1) {
                                        final Integer iKey = maybes.get(1).getKey();
                                        final Float fValue = maybes.get(1).getValue();
                                        String strWord = "unknown";
                                        
                                        // 根据模型索引获取对应的标签
                                        if (0 == mSelectedModelIndex) {
                                            if (iKey < mMobileTaiWords.size()) {
                                                strWord = mMobileTaiWords.get(iKey);
                                            }
                                        } else {
                                            if (iKey < mSqueezeTaiWords.size()) {
                                                strWord = mSqueezeTaiWords.get(iKey);
                                            }
                                        }
                                        
                                        // 处理标签显示格式
                                        final String resKey = mSelectedModelIndex == 1 ? strWord.length() >= 10 ? strWord.substring(10) : strWord : strWord;
                                        mSecondResult.setText(resKey + "：" + new DecimalFormat("0.00").format(fValue));
                                    }
                                    
                                    // 显示第三个识别结果
                                    if (maybes.size() > 2) {
                                        final Integer iKey = maybes.get(2).getKey();
                                        final Float fValue = maybes.get(2).getValue();
                                        String strWord = "unknown";
                                        
                                        // 根据模型索引获取对应的标签
                                        if (0 == mSelectedModelIndex) {
                                            if (iKey < mMobileTaiWords.size()) {
                                                strWord = mMobileTaiWords.get(iKey);
                                            }
                                        } else {
                                            if (iKey < mSqueezeTaiWords.size()) {
                                                strWord = mSqueezeTaiWords.get(iKey);
                                            }
                                        }
                                        
                                        // 处理标签显示格式
                                        final String resKey = mSelectedModelIndex == 1 ? strWord.length() >= 10 ? strWord.substring(10) : strWord : strWord;
                                        mThirdResult.setText(resKey + "：" + new DecimalFormat("0.00").format(fValue));
                                    }

                                    // 显示推理耗时
                                    mTimeTextView.setText("cost time：" + inferenceTimeCost + "ms");
                                }
                            });
                        }
                    });
                }
            }
        });
    }


    @Override
    public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
        // 根据不同的下拉框处理选项改变事件

        // 处理推理后端类型选择
        if (mForwardTypeSpinner.getId() == adapterView.getId()) {
            // 根据选中的索引设置对应的推理后端
            if (i == 0) {
                mConfig.forwardType = MNNForwardType.FORWARD_CPU.type;      // CPU
            } else if (i == 1) {
                mConfig.forwardType = MNNForwardType.FORWARD_OPENCL.type;   // OpenCL
            } else if (i == 2) {
                mConfig.forwardType = MNNForwardType.FORWARD_OPENGL.type;   // OpenGL
            } else if (i == 3) {
                mConfig.forwardType = MNNForwardType.FORWARD_VULKAN.type;   // Vulkan
            }
        }
        // 处理线程数选择
        else if (mThreadNumSpinner.getId() == adapterView.getId()) {
            // 根据选中的索引设置对应的线程数
            String[] threadList = getResources().getStringArray(R.array.thread_list);
            mConfig.numThread = Integer.parseInt(threadList[i].split(" ")[1]);
        }
        // 处理模型选择
        else if (mModelSpinner.getId() == adapterView.getId()) {
            // 设置选中的模型索引
            mSelectedModelIndex = i;
        } 
        // 处理更多示例选择
        else if (mMoreDemoSpinner.getId() == adapterView.getId()) {
            // 根据选中的索引跳转到对应的Activity
            if (i == 1) {
                Intent intent = new Intent(VideoActivity.this, ImageActivity.class);
                startActivity(intent);
            } else if (i == 2) {
                Intent intent = new Intent(VideoActivity.this, PortraitActivity.class);
                startActivity(intent);
            } else if (i == 3) {
                Intent intent = new Intent(VideoActivity.this, OpenGLTestActivity.class);
                startActivity(intent);
            }
        }

        // 锁定UI渲染，准备重新加载网络
        mLockUIRender.set(true);
        clearUIForPrepareNet();

        // 在后台线程中重新准备神经网络
        mHandle.post(new Runnable() {
            @Override
            public void run() {
                prepareNet();
            }
        });

    }

    /**
     * 清理UI界面，为准备网络显示提示信息
     * 在重新加载模型时显示"prepare net ..."提示
     */
    private void clearUIForPrepareNet() {
        mFirstResult.setText("prepare net ...");
        mSecondResult.setText("");
        mThirdResult.setText("");
        mTimeTextView.setText("");
    }


    @Override
    public void onNothingSelected(AdapterView<?> adapterView) {
        // 当下拉框没有选择任何项时的处理逻辑（当前为空实现）
    }

    @Override
    protected void onPause() {
        // 暂停摄像头预览
        mCameraView.onPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        // 恢复摄像头预览
        mCameraView.onResume();
    }


    @Override
    protected void onDestroy() {
        // 中断后台线程
        mThread.interrupt();

        // 释放MNN网络实例资源
        mHandle.post(new Runnable() {
            @Override
            public void run() {
                if (mNetInstance != null) {
                    mNetInstance.release();
                }
            }
        });

        super.onDestroy();
    }
}
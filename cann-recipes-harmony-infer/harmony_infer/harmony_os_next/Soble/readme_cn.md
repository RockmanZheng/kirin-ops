## HDC_Sobel_Demo  -- 模型推理

## 介绍

本示例展示了使用CANN API提供的模型推理能力。

本示例展示了对图片中的物体进行滤波识别的能力展示。

需要编译依赖CANN的动态库libhiai_foundation.so和AI领域公共动态库libneural_network_core.so。

## 效果预览

|             **主界面**             |              **结果**              | **下一张图片**                    |   |
|:-------------------------------:|:--------------------------------:|------------------------------|---|
| ![](screenshots/sobel_pre.jpeg) | ![](screenshots/sobel_finished.jpeg) |![](screenshots/sobel_next.jpeg) |   |

使用说明：
1. 使用DevEco构建应用前请将ATC转换后的omc模型文件放置到应用entry/src/main/resources/rawfile目录下，再进行应用构建和安装
2. 在手机的主屏幕，点击“npu推理”，启动应用，自动加载模型，进行模型处理，将处理后的图片和模型处理时间展示出来
3. 点击“cpu推理”，将图片交由CPU处理，将处理后的图片和cpu处理时间展示出来
4. 点击“Click for next image”，展示下一张图片。
5. 退出应用时，自动卸载模型。

## 工程目录
```
└── entry/src/main                            // 代码区
    ├── cpp
    │    ├── types/libentry
    │    │     └── Index.d.ts                 // native层接口注册文件
    │    ├── SobelCustom.cpp                  // native api层接口的具体实现函数
    │    ├── CMakeLists.txt                   // native层编译配置
    │    ├── HIAIModelManager.cpp             // 模型管理类的实现
    │    ├── HIAIModelManager.h               // 模型管理类的定义
    ├── ets
    │    ├── entryability 
    │    │     └── EntryAbility.ets           // 程序入口类
    │    ├── entrybackupability
    │    │     └── EntryBackupAbility.ets     // 备份文件
    │    ├── pages 
    │    │     └── Index.ets                  // 主界面展示类
    └── resources
    │    ├── base/media                       // 图片资源
    │    │   ├── cup.jpg
    │    │   └── guitar.jpg
    │    ├── rawfile 
    │    │    ├── SobelCustom.omc             // 模型文件
```

## 具体实现

本示例展示的功能使用了在HiAI Foundation和NNCore中定义的以下API：

* OH_NN_ReturnCode HMS_HiAIOptions_SetBandMode(OH_NNCompilation* compilation, HiAI_BandMode bandMode)
* HiAI_BandMode HMS_HiAIOptions_GetBandMode(const OH_NNCompilation* compilation)
* OH_NN_ReturnCode HMS_HiAIOptions_SetModelDeviceOrder(OH_NNCompilation* compilation, HiAI_ExecuteDevice* executeDevices, size_t deviceCount);
* HiAI_Compatibility HMS_HiAICompatibility_CheckFromBuffer(const void* data, size_t size);
* OH_NN_ReturnCode OH_NNDevice_GetAllDevicesID(const size_t **allDevicesID, uint32_t *deviceCount);
* OH_NN_ReturnCode OH_NNDevice_GetName(size_t deviceID, const char **name);
* void *OH_NNTensor_GetDataBuffer(const NN_Tensor *tensor);
* OH_NN_ReturnCode OH_NNTensor_GetSize(const NN_Tensor *tensor, size_t *size);
* OH_NN_ReturnCode OH_NNTensor_Destroy(NN_Tensor **tensor);
* OH_NNCompilation *OH_NNCompilation_ConstructWithOfflineModelBuffer(const void *modelBuffer, size_t modelSize);
* void OH_NNCompilation_Destroy(OH_NNCompilation **compilation);
* OH_NN_ReturnCode OH_NNCompilation_SetDevice(OH_NNCompilation *compilation, size_t deviceID);
* void OH_NNCompilation_Destroy(OH_NNCompilation **compilation);
* OH_NN_ReturnCode OH_NNCompilation_Build(OH_NNCompilation *compilation);
* OH_NNExecutor *OH_NNExecutor_Construct(OH_NNCompilation *compilation);
* OH_NN_ReturnCode OH_NNExecutor_GetInputCount(const OH_NNExecutor *executor, size_t *inputCount);
* NN_TensorDesc *OH_NNExecutor_CreateInputTensorDesc(const OH_NNExecutor *executor, size_t index);
* NN_Tensor *OH_NNTensor_Create(size_t deviceID, NN_TensorDesc *tensorDesc);
* OH_NN_ReturnCode OH_NNTensorDesc_Destroy(NN_TensorDesc **tensorDesc);
* OH_NN_ReturnCode OH_NNExecutor_GetOutputCount(const OH_NNExecutor *executor, size_t *outputCount);
* NN_TensorDesc *OH_NNExecutor_CreateOutputTensorDesc(const OH_NNExecutor *executor, size_t index);
* OH_NN_ReturnCode OH_NNExecutor_RunSync(OH_NNExecutor *executor, NN_Tensor *inputTensor[], size_t inputCount, NN_Tensor *outputTensor[], size_t outputCount);
* void OH_NNExecutor_Destroy(OH_NNExecutor **executor);

调用OH_NNExecutor_RunSync等接口，传入想要分类的图片，得到识别结果，观察日志等。参考entry/src/main/ets/pages/Index.ets.


## 相关权限

不涉及。

## 依赖

不涉及。

## 约束与限制

1. 本示例仅支持标准系统上运行，支持设备：华为手机、平板和2in1。
2. HarmonyOS系统：HarmonyOS 5.0.1 Release及以上。
3. DevEco Studio版本：DevEco Studio 6.0.0 Release及以上。
4. HarmonyOS SDK版本：HarmonyOS 6.0.0 Release SDK及以上。

# 基本接口设计
## 1、 初始化/去初始化

| 函数原型 | aclError aclInit(const char *configPath) |
|---------| ------------------------------------------|
| 函数功能 | 初始化接口， 获取配置信息 |
| 输入说明 | configPath配置文件路径（端侧只支持空）|
| 输出说明 | NA |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | 使用Ascendcl接口开发时， 必须先调用aclInit接口 |
| 注意事项 | 端侧configPath只支持空一个进程只能调用一次 |

| 函数原型 | aclError aclFinalize() |
|---------| ------------------------------------------|
| 函数功能 | 去初始化接口， 释放资源 |
| 输入说明 | NA |
| 输出说明 | NA |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | 应用程序退出前， 应确保AscendCL已经去初始化 |
| 注意事项 | NA |

## 2、 模型编译（模型保存，可选； 删除模型）

| 函数原型 | graphStatus aclgrphBuildModelFromFile(const char *graph_file, const std::map<AscendString, AscendString> &build_options, aclmdlModel** model） |
|---------| ------------------------------------------|
| 函数功能 | 编译模型文件， 获取编译后的对象指针 |
| 输入说明 | const char_t *graph_file模型文件，const std::map<AscendString, AscendString> &build_options编译参数）|
| 输出说明 | aclmdlModel **model编译后的对象 |
| 返回值说明 | graphStatus成功返回GRAPH_SUCCESS，失败返回GRAPH_FAILED |
| 使用说明 | 获取的编译后对象可以在加载时直接使用 |
| 注意事项 | 入参为om模型 |


| 函数原型 | graphStatus aclgrphBuildModelFromData(const void *graph_data, size_t graph_size, const std::map<AscendString, AscendString> &build_options, aclmdlModel **model) |
|---------| ------------------------------------------|
| 函数功能 | 编译模型缓存， 获取编译后的对象指针 |
| 输入说明 | const char_t *graph_data模型缓存，const std::map<AscendString, AscendString> &build_options编译参数）|
| 输出说明 | aclmdlModel **model编译后的对象 |
| 返回值说明 | graphStatus成功返回GRAPH_SUCCESS，失败返回GRAPH_FAILED |
| 使用说明 | 获取的编译后对象可以在加载时直接使用 |
| 注意事项 | 入参为om模型 |

| 函数原型 | graphStatus aclgrphSaveModel(const char_t *output_file, const aclmdlModel* model) |
|---------| ------------------------------------------|
| 函数功能 | 保存编译后的模型 |
| 输入说明 | const char_t *output_file保存的模型文件，aclmdlModel *model 编译后的对象 |
| 输出说明 | NA |
| 返回值说明 | graphStatus, 成功返回GRAPH_SUCCESS，失败返回GRAPH_FAILED|
| 使用说明 | 将获取的编译后对象保存到模型文件内 |
| 注意事项 | NA |

| 函数原型 | graphStatus aclgrphSaveModelToBuffer(ModelBufferData &model_buffer, const aclmdlModel *model) |
|---------| ------------------------------------------|
| 函数功能 | 保存编译后的模型 |
| 输入说明 | ModelBufferData &model_buffer 外部用户提供的内存块, aclmdlModel *model 编译后的对象 |
| 输出说明 | NA |
| 返回值说明 | graphStatus, 成功返回GRAPH_SUCCESS，失败返回GRAPH_FAILED |
| 使用说明 | 将获取的编译后对象保存到模型缓存内 |
| 注意事项 | NA |


build option支持的参数

| 参数 | 值 |
|---------| -------------|
| INPUT_SHAPE | 静态shape，示例：{ge::ir_option::INPUT_SHAPE, "input_0_0:16,32,208,208;input_1_0:16,64,208,208"}；动态shape暂不支持 |
| PRECISION_MODE_V2 | 精度模式，示例：PRECISION_MODE_V2 {ge::ir_option::PRECISION_MODE_V2, "fp16"} 端侧只支持fp16（默认值）与origin |
| EXEC_DISABLE_REUSED_MEMORY | 内存复用模式，示例：EXEC_DISABLE_REUSED_MEMORY {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, "0" } |

## 3、 模型加载

| 函数原型 | aclError aclmdlLoadFromModel(const aclmdlModel *model, const aclmdlConfigHandle *handle, uint32_t *modelId) |
|---------| ------------------------------------------|
| 函数功能 | 模型加载 |
| 输入说明 | const aclmdlModel *model编译后的对象，const aclmdlConfigHandle *handle配置参数 |
| 输出说明 | unint32_t *modelld模型标识id |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | 用编译后的模型，不序列化直接加载 |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlLoadFromFile(const char *modelPath, uint32_t *modelId) |
|---------| ------------------------------------------|
| 函数功能 | 模型加载 |
| 输入说明 | const char *modelPath模型路径 |
| 输出说明 | unint32_t *modelld模型标识id |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM  |
| 使用说明 | 从文件加载模型 |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlLoadFromMem(const void *model, size_t modelSize, uint32_t *modelId) |
|---------| ------------------------------------------|
| 函数功能 | 模型加载 |
| 输入说明 | const void *model模型数据内存地址，size_t modelSize模型数据内存大小 |
| 输出说明 | unint32_t *modelld模型标识id |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM  |
| 使用说明 | 用编译后的模型，不序列化直接加载 |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlLoadWithConfig(const aclmdlConfigHandle *handle, uint32_t *modelId)) |
|---------| ------------------------------------------|
| 函数功能 | 模型加载 |
| 输入说明 | const aclmdlConfigHandle *handle配置参数 |
| 输出说明 | unint32_t *modelld模型标识id |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM  |
| 使用说明 | 根据配置信息执行加载 |
| 注意事项 | NA |

## 4、 动态配置

| 函数原型 | aclmdlConfigHandle *aclmdlCreateConfigHandle()  |
|---------| ------------------------------------------|
| 函数功能 | 创建配置信息 |
| 输入说明 | NA |
| 输出说明 | NA |
| 返回值说明 | aclmdlConfigHandle *handle配置参数  |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlSetConfigOpt(aclmdlConfigHandle *handle, aclmdlConfigAttr attr, const void *attrValue, size_t valueSize)  |
|---------| ------------------------------------------|
| 函数功能 | 动态设置配置信息 |
| 输入说明 | const aclmdlConfigHandle *handle配置参数， aclmdlConfigAttr attr 需要设置的属性， conat void *attrValue指向属性的指针， size_t valueSize value部分数据长度 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM  |
| 使用说明 | 将配置信息添加到对象内 |
| 注意事项 | NA |

| 函数原型 | void aclmdlSetConfigs (uint32_t modelld, const aclmdlConfigHandle *handle)   |
|---------| ------------------------------------------|
| 函数功能 | 动态设置配置信息 |
| 输入说明 | unint32_t *modelld模型标识id, const aclmdlConfigHandle *handle配置参数 |
| 输出说明 | NA  |
| 返回值说明 | NA   |
| 使用说明 | 指明模型使用的配置信息 |
| 注意事项 | NA |

aclmdlConfigHandles端侧仅支持这些配置：

| 参数 | 值 |
|---------| ------------------------------------------|
| ACL_MDL_PRIORITY_INT32  | 模型执行的优先级，数字越小优先级越高，取值[0,7], 可选项。 默认值为0 |
| ACL_MDL_LOAD_TYPE_SIZET | 模型加载方式，端侧支持ACL_MDL_ LOAD _FROM _Fl LE （从文件加载）和ACL_MDL_LOAD_FROM_MEM（从内存加载） |
| ACL_MDL_PATH_PTR | 离线模型文件路径的指针，如果选择从文件加载模型，则该选项必选 |
| ACL_MDL_MEM_ADDR_PTR | 模型在内存中的起始地址，如果选择从内存加载模型，则该选项必选 |
| ACL_MDL_MEM_SIZET | 模型在内存中的大小，如果选择从内存加载模型，则该选项必选，与ACL_MDL_MEM_ADDR_PTR选项配合使用 |

## 5、 输入输出内存申请和构造

| 函数原型 | aclmdlDesc* aclmdlCreateDesc()  |
|---------| ------------------------------------------|
| 函数功能 | 创建aclmdlDesc类型的数据表示模型描述信息 |
| 输入说明 | NA  |
| 输出说明 | NA  |
| 返回值说明 | 返回aclmdlDesc类型的指针 |
| 使用说明 | NA |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlGetDesc(aclmdlDesc *modelDesc, uint32_t modelId)   |
|---------| ------------------------------------------|
| 函数功能 | 获取aclmdlDesc类型的数据表示模型描述信息 |
| 输入说明 | aclmdlDesc类型的指针  |
| 输出说明 | unint32 t *modelld模型标识id  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | size_t aclmdlGetNumlnputs(aclmdlDesc *modelDesc)   |
|---------| ------------------------------------------|
| 函数功能 | 根据描述信息获取输入个数 |
| 输入说明 | aclmdlDesc类型的指针  |
| 输出说明 | NA  |
| 返回值说明 | 模型的输入个数 |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | size_t aclmdlGetNumOutputs(aclmdlDesc *modelDesc)   |
|---------| ------------------------------------------|
| 函数功能 | 根据描述信息获取输出个数 |
| 输入说明 | aclmdlDesc类型的指针  |
| 输出说明 | NA  |
| 返回值说明 | 模型的输出个数 |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | size_t aclmdlGetlnputSizeBylndex(aclmdlDesc *modelDesc, size_t index)    |
|---------| ------------------------------------------|
| 函数功能 | 根据模型描述信息获取指定输入的大小 |
| 输入说明 | aclmdlDesc *modelDesc adlmdlDesc类型的指针, size_t index指定获取第几个输入的大小，index值从0开始  |
| 输出说明 | NA  |
| 返回值说明 | 返回输入的大小 |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | size_t aclmdlGetOutputSizeBylndex(aclmdlDesc *modelDesc, size_t index)    |
|---------| ------------------------------------------|
| 函数功能 | 根据模型描述信息获取指定输出的大小 |
| 输入说明 | aclmdlDesc *modelDesc adlmdlDesc类型的指针, size_t index 指定获取第几个输出的大小 ，index值从0开始 |
| 输出说明 | NA  |
| 返回值说明 | 返回输出的大小 |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclrtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count)    |
|---------| ------------------------------------------|
| 函数功能 | 初始化内存将内存中的内容设置为指定的值 |
| 输入说明 | void *devPtr目标内存地址，size_t maxCount最大设置字节数，int32_t value设置为该值，size_t count实际设计字节数 |
| 输出说明 |  NA |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM  |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclrtfree(void *devPtr)  |
|---------| ------------------------------------------|
| 函数功能 | 释放 Device上的内存 |
| 输入说明 | void *devPtr待释放内存的指针 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM  |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclDataBuffer* aclCreateDataBuffer(void *data, size_t size)  |
|---------| ------------------------------------------|
| 函数功能 | 创建aclDataBuffer类型的数据，该数据类型用于描述内存地址、大小等内存信息 |
| 输入说明 | void *data存放数据内存地址的指针。 该内存需调用aclrtMalloc申请，端侧不支持data为空, size_t size内存大小单位Byte |
| 输出说明 | NA  |
| 返回值说明 | 返回aclDataBuffer类型的指针  |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclDestroyDataBuffer(const aclDataBuffer* dataBuffer) |
|---------| ------------------------------------------|
| 函数功能 | 销毁通过aclCreateDataBuffer接口创建的aclDataBuffer类型的数据 |
| 输入说明 | const aclDataBuffer *dataBuffer待销毁的aclDataBuffer类型的指针 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM  |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclmdlDataset *aclmdlCreateDataset()   |
|---------| ------------------------------------------|
| 函数功能 | 创建的adlmdlDataset类型的数据 |
| 输入说明 | NA |
| 输出说明 | NA  |
| 返回值说明 | 返回aclmdlDataset类型的指针 |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlDestroyDataset(const aclmdlDataset *dataset)   |
|---------| ------------------------------------------|
| 函数功能 | 销毁通过aclmdlCreateDataset接口创建的adlmdlDataset类型的数据 |
| 输入说明 | const aclmdlDataset *dataset待销毁的aclmdlDataset类型的指针 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM  |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlAddDatasetBuffer(aclmdlDataset *dataset, aclDataBuffer *dataBuffer)   |
|---------| ------------------------------------------|
| 函数功能 | 向aclmdlDataset中增加aclDataBuffer |
| 输入说明 | aclmdlDataset *dataset待增加dataset容器，aclDataBuffer *dataBuffer待增加的dataBuffer地址指针 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |

## 6、 同步推理， 模型卸载

| 函数原型 | aclError aclmdlExecute(uint32_t modelId, const aclmdlDataset *input, aclmdlDataset *output)  |
|---------| ------------------------------------------|
| 函数功能 | 同步推理 |
| 输入说明 | uint32_t modelld模型标识，const aclmdlDataset *input 模型推理的输入数据的指针，aclmdlDataset *output模型推理输出数据的指针。|
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM   |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlSetInputDynamicDims(uint32_t modelId, aclmdlDataset *dataset, size_t index, const aclmdlIODims *dims)   |
|---------| ------------------------------------------|
| 函数功能 | 动态修改推理时的输入数据 |
| 输入说明 | uint32_t modelld模型标识，aclmdlDataset *dataset输入数据的指针，size_t index标识动态维度的输入index，const aclmdllODims *dims具体某一档上的所有维度信息的指针。 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


## 7、 aipp图像预处理
详细参数规格参考 https://developer.huawei.com/consumer/cn/doc/hiai-Guides/aipp-parameters-0000001053445646

| 函数原型 | aclmdlAIPP* aclmdlCreateAIPP(uint64_t batchSize)  |
|---------| ------------------------------------------|
| 函数功能 | 创建aipp参数 |
| 输入说明 | batchSize aipp批量个数 |
| 输出说明 | NA  |
| 返回值说明 | 创建成功返回参数地址，失败返回空指针  |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlDestroyAIPP(const aclmdlAIPP *aippParmsSet)   |
|---------| ------------------------------------------|
| 函数功能 | 销毁aipp参数 |
| 输入说明 | aippParmsSet 待销毁aipp参数地址 |
| 输出说明 | NA  |
| 返回值说明 | 成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlGetAippDataSize(uint64_t batchSize, size_t *size)   |
|---------| ------------------------------------------|
| 函数功能 | 获取aipp参数空间大小 |
| 输入说明 | batchSize 批量大小输入, size 此批量大小对应的aipp参数数据大小 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlSetAIPPInputFormat(aclmdlAIPP *aippParmsSet, aclAippInputFormat inputFormat)   |
|---------| ------------------------------------------|
| 函数功能 | 设置输入图片格式的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，inputFormat 输入图片格式|
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlSetAIPPSrcImageSize(aclmdlAIPP *aippParmsSet, int32_t srcimageSizeW, int32_t srcimageSizeH)   |
|---------| ------------------------------------------|
| 函数功能 | 设置输入源图片大小的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，srcimageSizeW 图片宽度， srcimageSizeH 图片高度 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlSetAIPPCscParams(aclmdlAIPP *aippParmsSet, int8_t cscSwitch, int16_t cscMatrixROCO, int16_t cscMatrixROC1, int16_t cscMatrixR0C2, int16_t cscMatrixR1CO, int16_t cscMatrixR1C1, int16_t cscMatrixR1C2, int16_t cscMatrixR2C0, int16_t cscMatrixR2C1, int16_t cscMatrixR2C2, uint8_t cscOutputBiasRO, uint8_t cscOutputBiasR1, uint8_t cscOutputBiasR2, uint8_t cscinputBiasRO, uint8_t cscInputBiasR1, uint8_t cscInputBiasR2)   |
|---------| ------------------------------------------|
| 函数功能 | 设置颜色空间转换的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，cscSwitch 转换开关，cscMatrix等参数参考上面链接 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlSetAIPPRbuvSwapSwitch(aclmdlAIPP *aippParmsSet, int8_t rbuvSwapSwitch)   |
|---------| ------------------------------------------|
| 函数功能 | 设置RB/UV通道交换的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，rbuvSwapSwitch 转换开关 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlSetAIPPAxSwapSwitch(aclmdlAIPP *aippParmsSet, int8_t axSwapSwitch)   |
|---------| ------------------------------------------|
| 函数功能 | 设置AX通道交换的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，axSwapSwitch 转换开关 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |

| 函数原型 | aclError aclmdlSetAIPPScfParams(aclmdlAIPP *aippParmsSet, int8_t scfSwitch, int32_t scfInputSizeW, int32_t scfInputSizeH, int32_t scfOutputSizeW, int32_t scfOutputSizeH, uint64_t batchIndex)   |
|---------| ------------------------------------------|
| 函数功能 | 设置图片缩放的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，scfSwitch 功能开关，batchIndex aipp输出索引，其他参数参考链接 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlSetAIPPCropParams(aclmdlAIPP *aippParmsSet, int8_t cropSwitch, int32_t cropStartPosW, int32_t cropStartPosH, int32_t cropSizeW, int32_t cropSizeH, uint64_t batchIndex)   |
|---------| ------------------------------------------|
| 函数功能 | 设置图片缩放的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，cropSwitch 功能开关，batchIndex aipp输出索引，其他参数参考链接 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlSetAIPPPaddingParams(aclmdlAIPP *aippParmsSet, int8_t paddingSwitch, int32_t paddingSizeTop, int32_t paddingSizeBottom, int32_t paddingSizeLeft, int32_t paddingSizeRight, uint64_t batchIndex)  |
|---------| ------------------------------------------|
| 函数功能 | 设置图片缩放的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，paddingSwitch 功能开关，batchIndex aipp输出索引，其他参数参考链接 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlSetAIPPDtcPixelMean(aclmdlAIPP *aippParmsSet, int16_t dtcPixelMeanChn0, int16_t dtcPixelMeanChn1, int16_t dtcPixelMeanChn2, int16_t dtcPixelMeanChn3, uint64_t batchIndex)  |
|---------| ------------------------------------------|
| 函数功能 | 设置图片数据类型转换的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，batchIndex aipp输出索引，其他参数参考链接 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlSetAIPPDtcPixelMin(aclmdlAIPP *aippParmsSet, float dtcPixelMinChn0, float dtcPixelMinChn1, float dtcPixelMinChn2, float dtcPixelMinChn3, uint64_t batchIndex)  |
|---------| ------------------------------------------|
| 函数功能 | 设置图片数据类型转换的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，batchIndex aipp输出索引，其他参数参考链接 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlSetAIPPPixelVarReci(aclmdlAIPP *aippParmsSet, float dtcPixelVarReciChn0, float dtcPixelVarReciChn1, float dtcPixelVarReciChn2, float dtcPixelVarReciChn3, uint64_t batchIndex)  |
|---------| ------------------------------------------|
| 函数功能 | 设置图片数据类型转换的aipp参数 |
| 输入说明 | aippParmsSet 待设置aipp参数地址，batchIndex aipp输出索引，其他参数参考链接 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


| 函数原型 | aclError aclmdlSetAIPPByInputIndex(uint32_t modelId, aclmdlDataset *dataset, size_t index, const aclmdlAIPP *aippParmSet)  |
|---------| ------------------------------------------|
| 函数功能 | 设置aipp参数到指定模型输入 |
| 输入说明 | modelId 指定的模型，index 输入index，aippParmsSet 待设置aipp参数地址 |
| 输出说明 | NA  |
| 返回值说明 | aclError成功返回ACL_SUCCESS, 失败返回ACL_ERROR_INVALID_PARAM |
| 使用说明 | NA |
| 注意事项 | NA |


## 代码调用示例
```C++
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>

#include "neural_network_runtime/neural_network_runtime_type.h"
#include "acl_manager.h"
#include "utils/ir_option.h"
#include "graph/ascend_string.h"

struct AscendclParam {
    aclmdlDataset* inputDataSet_ {nullptr};
    aclmdlDataset* outputDataSet_ {nullptr};
    std::vector<std::string> inputDataFile_ {};
    std::vector<std::string> outputDataFile_ {};
    uint32_t modelId_ = 0;
    std::string modelPath_ = "";
};

FILE* OpenFile(const std::string& path)
{
    char pathTemp[PATH_MAX] = {0x00};
    if (realpath(path.c_str(), pathTemp) == nullptr) {
        printf("OpenFile error: realpath return nullptr path=%s.\n", path.c_str());
        return nullptr;
    }
    return fopen(pathTemp, "r+");
}

uint32_t GetFileLength(FILE* fp)
{
    if (fseek(fp, 0, SEEK_END) != 0) {
        printf("fseek SEEK_END error.\n");
        return 0;
    }

    long fileLength = ftell(fp);
    if (fileLength > UINT32_MAX || fileLength < 0) {
        printf("file length error.\n");
        return 0;
    }

    if (fseek(fp, 0, SEEK_SET)) {
        printf("fseek SEEK_SET error.\n");
        return 0;
    }

    return static_cast<uint32_t>(fileLength);
}

bool ReadFile(const std::string& path, void** buffer, size_t* size)
{
    FILE* fp = OpenFile(path);
    if (fp == nullptr) {
        printf("OpenFile failed\n");
        return false;
    }

    uint32_t fileLength = GetFileLength(fp);
    if (fileLength == 0) {
        printf("fileLength is 0\n");
        (void) fclose(fp);
        return false;
    }

    void* data = malloc(fileLength);
    if (data == nullptr) {
        printf("CreateFromPath ERROR: malloc fail!\n");
        (void) fclose(fp);
        return false;
    }

    *size = static_cast<size_t>(fread(data, 1, fileLength, fp));
    if (*size != fileLength) {
        printf("CreateFromPath ERROR: size(%zu) != fileLength(%u)\n", *size, fileLength);
        free(data);
        data = nullptr;
        (void) fclose(fp);
        return false;
    }

    *buffer = data;
    (void) fclose(fp);
    return true;
}

bool WriteFile(const std::string& path, const void* buffer, size_t size)
{
    std::ofstream ofs(path, std::ios::binary);
    if(!ofs) {
        printf("open file failed\n");
        return false;
    }

    ofs.write(reinterpret_cast<char*>(buffer), size);
    ofs.close();
    return true;
}

int main()
{
    AscendclParam ascendclParm;
    ascendclParm.modelPath_ = "./UdunA5.om";
    ascendclParm.inputDataFile_ = {"UdunA5_input.bin"};
    ascendclParm.outputDataFile_ = {"UdunA5_gold.bin"};
    ascendclParm.inputDataSet_ = aclmdlCreateDataset();
    ascendclParm.outputDataSet_ = aclmdlCreateDataset();
    ascendclParm.modelId_ = 0;
    std::string configPath = "";
    auto ret = aclInit(configPath.c_str());
    if (ret != ACL_SUCCESS) {
        printf("init failed.\n");
        return;
    }
    printf("aclInit success.\n");
    for(size_t i = 0; i < ascendclParm.inputDataFile_.size(); i++) {
        void* data = nullptr;
        size_t inputDataSize = 0;
        if (!ReadFile(ascendclParm.inputDataFile_[i].c_str(), &data, &inputDataSize)) {
            return;
        }
        printf("readfile end\n");
        aclDataBuffer* dataBuffer = aclCreateDataBuffer(data, inputDataSize);
        auto ret = aclmdlAddDatasetBuffer(ascendclParm.inputDataSet_, dataBuffer);
        if (ret != ACL_SUCCESS) {
            printf("CreateInputData failed.\n");
            return;
        }
    }
    for(size_t i = 0; i < ascendclParm.outputDataFile_.size(); i++) {
        void* data = nullptr;
        size_t outputDataSize = 0;
        if (!ReadFile(ascendclParm.outputDataFile_[i].c_str(), &data, &outputDataSize)) {
            return;
        }
        printf("readfile end\n");
        aclDataBuffer* dataBuffer = aclCreateDataBuffer(data, outputDataSize);
        auto ret = aclmdlAddDatasetBuffer(ascendclParm.outputDataSet_, dataBuffer);
        if (ret != ACL_SUCCESS) {
            printf("CreateOutputData failed.\n");
            return;
        }
    }
    printf("CreateData success.\n");

    std::string value2 = "0";
    std::string value3 = "fp16";
    std::map<AscendString, AscendString> buildOptions {
        {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, value2.c_str()},
        {ge::ir_option::PRECISION_MODE_V2, value3.c_str()}
    };
    aclmdlModel model;
    aclmdlModel* p = &model;
    aclmdlModel** ptr = &p;

    ret = aclgrphBuildModel(ascendclParm.modelPath_.c_str(), buildOptions, ptr);
    if (ret != ACL_SUCCESS) {
        printf("build file failed.\n");
        return;
    }
    aclmdlConfigHandle* handle = aclmdlCreateConfigHandle();
    ret = aclmdlLoadFromModel(p, handle, &ascendclParm.modelId_);
    if (ret != ACL_SUCCESS) {
        printf("load from model failed.\n");
        return;
    }
    printf("LoadFromModel success.\n");
    printf("model id: %d\n", ascendclParm.modelId_);

    ret = aclmdlDestroyConfigHandle(handle);
    if (ret != ACL_SUCCESS) {
        printf("destroy config handle failed.\n");
        return;
    }
    ret = aclmdlExecute(ascendclParm.modelId_, ascendclParm.inputDataSet_, ascendclParm.outputDataSet_);
    if (ret != ACL_SUCCESS) {
        printf("Execute failed\n");
        return;
    }
    printf("Execute success\n");

    if ((*(ascendclParm.outputDataSet_)).size() != 1) {
        printf("outputDataSet_ vector size error\n");
        return;
    }
 
    void* outputTensorData = (*(ascendclParm.outputDataSet_))[0]->data;
    size_t outputTensorSize =  (*(ascendclParm.outputDataSet_))[0]->size;
    if (!WriteFile("UdunA5_output.bin", outputTensorData, outputTensorSize)) {
        return;
    }

    auto desc = aclmdlCreateDesc();
    ret = aclmdlGetDesc(desc, ascendclParm.modelId_);
    if (ret != ACL_SUCCESS) {
        printf("CreateDesc failed.\n");
        return;
    }
    printf("CreateDesc success.\n");
    ret = aclmdlDestroyDesc(desc);
    if (ret != ACL_SUCCESS) {
        printf("DestroyDesc failed.\n");
        return;
    }
    printf("DestroyDesc success.\n");
    ret = aclmdlUnload(ascendclParm.modelId_);
    if (ret != ACL_SUCCESS) {
        printf("Unload failed.\n");
        return;
    }
    printf("Unload success.\n");
    ret = aclFinalize();
    if (ret != ACL_SUCCESS) {
        printf("aclFinalize failed.\n");
    }
    printf("aclFinalize success.\n");
}
```

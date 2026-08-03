# AscendC自定义算子开发指南

[toc]

# AscendC简介

## 概述

Ascend C是CANN针对算子开发场景推出的编程语言，遵循C和C++标准规范，匹配开发者开发习惯；通过多层接口抽象、自动并行计算、孪生调试等关键技术，提高算子开发效率，助力AI开发者低成本完成算子开发和模型调优部署。

![(zh-cn_image)](https://alliance-communityfile-drcn.dbankcdn.com/FileServer/getFile/cmtyPub/011/111/111/0000000000011111111.20251215183850.71020205189028050355819483523473:50001231000000:2800:432F5A5CCC99834545DCA96EFDB558262A0EC3089D478D90FC6A71E3C3DA852B.png)

使用AscendC进行自定义算子开发的突出优势有：

- 遵循C/C++编程规范，匹配开发者开发习惯

- 自动并行调度，获得最优执行性能

- 结构化核函数编程，简化算子开发逻辑

- NPU仿真调试，提升算子调试效率

开发者可以通过[Ascend C主页](https://www.hiascend.com/zh/ascend-c)了解更详细的内容。


## 使用须知

当前AscendC支持的产品型号为：KirinX90/Kirin9030系列处理器产品。


# 环境准备

进行环境准备前，你需要了解如下基本概念，以便更好的理解后续操作。


- 开发环境：指编译开发代码的环境。

- 运行环境：指运行算子、推理程序等的Linux环境。运行环境必须连接上带有Kirin AI处理器的设备，如手机、平板等。

- 开发环境与运行环境合设场景：开发环境和运行环境在同一台机器上，开发者使用连接上Kirin AI处理器的机器作为运行环境，同时在该环境上进行代码开发与编译。

- 开发环境与运行环境分设场景：开发环境和运行环境不在同一台机器上，开发者使用连接上Kirin AI处理器的机器作为运行环境；使用其他独立机器进行代码开发与编译，作为开发环境。 


进行自定义算子开发前，需要完成CANN软件包的安装，请参考[环境准备](./quick_install.md)。



# 快速入门

本节以一个简单算子为例，带开发者体验从算子工程创建、代码编写、编译部署到运行验证的开发全流程，让开发者对算子开发工程有个宏观的认识，此处我们以输入是动态shape的Add算子实现为例，为了与内置Add算子区分，定义算子类型为AddCustom。


## 工程创建

直接参考 ops/ascendc/src/add_custom工程

```
│──build.sh
│──CMakeLists.txt
│──CMakePresets.json
│──run.sh
│
├─framework
│  │──CMakeLists.txt
│  │
│  └─onnx_plugin
│          │──CMakeLists.txt
│          │──add_custom_plugin.cc
│
├─op_host
│      │──CMakeLists.txt
│      │──add_custom.cpp
│      │──add_custom_tiling.h
│
├─op_kernel
│      │──CMakeLists.txt
│      │──add_custom.cpp
│
└─test
    │──create_model.py
    │──gen_data.py

```

## 算子核函数实现

在工程存储目录的"AddCustom/op_kernel/add_custom.cpp"文件中实现算子的核函数，完整的样例代码开发者可以在[add_custom.cpp](../ops/ascendc/src/add_custom/op_kernel/add_custom.cpp)中查看，下面介绍关键实现代码。

算子核函数实现代码的内部调用关系示意图如下。

**图1** 核函数调用关系图  
![](https://alliance-communityfile-drcn.dbankcdn.com/FileServer/getFile/cmtyPub/011/111/111/0000000000011111111.20251215183857.20013491577714889222610497743724:50001231000000:2800:16CAFA7B2C254E688B316520F2E79EC6B8A2CD3E8D46CF4E3165A1A28842DDE4.png)

由此可见除了Init函数完成初始化外，Process中完成了对流水任务 **：** 搬入、计算、搬出的调用，开发者可以重点关注三个流水任务的实现。

1. 进行**核函数的定义，** 并在核函数中调用算子类的Init和Process函数。
    ```cpp
    extern "C" __global__ __aicore__ void add_custom(GM_ADDR x, GM_ADDR y, GM_ADDR z, GM_ADDR workspace, GM_ADDR tiling) 
    { 
        // 获取Host侧传入的Tiling参数 
        GET_TILING_DATA(tiling_data, tiling); 
        // 初始化算子类 
        KernelAdd op; 
        // 算子类的初始化函数，完成内存初始化相关工作 
        op.Init(x, y, z, tiling_data.totalLength, tiling_data.tileNum); 
        // 完成算子实现的核心逻辑 
        op.Process(); 
    }
    ```

2. 定义KernelAdd算子类，其具体成员及成员函数实现如下。
    ```cpp
    #include "kernel_operator.h" 
    constexpr int32_t BUFFER_NUM = 2; 
    class KernelAdd { 
    public: 
        __aicore__ inline KernelAdd() {} 
        // 初始化函数，完成内存初始化相关操作 
        __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, GM_ADDR z, uint32_t totalLength, uint32_t tileNum) 
        { 
            // 使用获取到的TilingData计算得到singleCoreSize(每个核上总计算数据大小)、tileNum（每个核上分块个数）、singleTileLength（每个分块大小）等变量 
            this->blockLength = totalLength / AscendC::GetBlockNum(); 
            this->tileNum = tileNum; 
            this->tileLength = this->blockLength / tileNum / BUFFER_NUM; 
            
            // 获取当前核的起始索引 
            xGm.SetGlobalBuffer((__gm__ DTYPE_X*)x + this->blockLength * AscendC::GetBlockIdx(), this->blockLength); 
            yGm.SetGlobalBuffer((__gm__ DTYPE_Y*)y + this->blockLength * AscendC::GetBlockIdx(), this->blockLength); 
            zGm.SetGlobalBuffer((__gm__ DTYPE_Z*)z + this->blockLength * AscendC::GetBlockIdx(), this->blockLength); 
            // 通过Pipe内存管理对象为输入输出Queue分配内存 
            pipe.InitBuffer(inQueueX, BUFFER_NUM, this->tileLength * sizeof(DTYPE_X)); 
            pipe.InitBuffer(inQueueY, BUFFER_NUM, this->tileLength * sizeof(DTYPE_Y)); 
            pipe.InitBuffer(outQueueZ, BUFFER_NUM, this->tileLength * sizeof(DTYPE_Z)); 
        } 
        // 核心处理函数，实现算子逻辑，调用私有成员函数CopyIn、Compute、CopyOut完成矢量算子的三级流水操作 
        __aicore__ inline void Process() 
        { 
            int32_t loopCount = this->tileNum * BUFFER_NUM; 
            for (int32_t i = 0; i < loopCount; i++) { 
                CopyIn(i); 
                Compute(i); 
                CopyOut(i); 
            } 
        } 
    
    
    private: 
        // 搬入函数，完成CopyIn阶段的处理，被核心Process函数调用 
        __aicore__ inline void CopyIn(int32_t progress) 
        { 
            // 从Queue中分配输入Tensor 
            AscendC::LocalTensor<DTYPE_X> xLocal = inQueueX.AllocTensor<DTYPE_X>(); 
            AscendC::LocalTensor<DTYPE_Y> yLocal = inQueueY.AllocTensor<DTYPE_Y>(); 
            // 将GlobalTensor数据拷贝到LocalTensor 
            AscendC::DataCopy(xLocal, xGm[progress * this->tileLength], this->tileLength); 
            AscendC::DataCopy(yLocal, yGm[progress * this->tileLength], this->tileLength); 
            // 将LocalTensor放入VECIN（代表矢量编程中搬入数据的逻辑存放位置）的Queue中 
            inQueueX.EnQue(xLocal); 
            inQueueY.EnQue(yLocal); 
        } 
        // 计算函数，完成Compute阶段的处理，被核心Process函数调用 
        __aicore__ inline void Compute(int32_t progress) 
        { 
            // 将Tensor从队列中取出，用于后续计算 
            AscendC::LocalTensor<DTYPE_X> xLocal = inQueueX.DeQue<DTYPE_X>(); 
            AscendC::LocalTensor<DTYPE_Y> yLocal = inQueueY.DeQue<DTYPE_Y>(); 
            // 从Queue中分配输出Tensor 
            AscendC::LocalTensor<DTYPE_Z> zLocal = outQueueZ.AllocTensor<DTYPE_Z>(); 
            // 调用Add接口进行计算 
            AscendC::Add(zLocal, xLocal, yLocal, this->tileLength); 
            // 将计算结果LocalTensor放入到VecOut的Queue中 
            outQueueZ.EnQue<DTYPE_Z>(zLocal); 
            // 释放输入Tensor 
            inQueueX.FreeTensor(xLocal); 
            inQueueY.FreeTensor(yLocal); 
        } 
        // 搬出函数，完成CopyOut阶段的处理，被核心Process函数调用 
        __aicore__ inline void CopyOut(int32_t progress) 
        { 
            // 从VecOut的Queue中取出输出Tensor 
            AscendC::LocalTensor<DTYPE_Z> zLocal = outQueueZ.DeQue<DTYPE_Z>(); 
            // 将输出Tensor拷贝到GlobalTensor中 
            AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, this->tileLength); 
            // 将不再使用的LocalTensor释放 
            outQueueZ.FreeTensor(zLocal); 
        } 
    
    
    private: 
        //Pipe内存管理对象 
        AscendC::TPipe pipe; 
        //输入数据Queue队列管理对象，QuePosition为VECIN 
        AscendC::TQue<AscendC::QuePosition::VECIN, 1> inQueueX, inQueueY;  
        //输出数据Queue队列管理对象，QuePosition为VECOUT 
        AscendC::TQue<AscendC::QuePosition::VECOUT, 1> outQueueZ; 
        //管理输入输出Global Memory内存地址的对象，其中xGm, yGm为输入，zGm为输出 
        AscendC::GlobalTensor<DTYPE_X> xGm; 
        AscendC::GlobalTensor<DTYPE_Y> yGm; 
        AscendC::GlobalTensor<DTYPE_Z> zGm; 
        // 每个核上总计算数据大小 
        uint32_t blockLength; 
        // 每个核上总计算数据分块个数 
        uint32_t tileNum; 
        // 每个分块大小 
        uint32_t tileLength; 
    };
    ```


## Host侧算子实现

核函数开发并验证完成后，下一步就是进行Host侧的实现，对应“AddCustom/op_host”目录下的add_custom_tiling.h文件与add_custom.cpp文件。下面简要介绍下两个文件的关键实现，完整的样例代码可参见[add_custom_tiling.h](https://gitcode.com/HarmonyOS_Samples/cannkit_samplecode_add_custom_cpp/blob/master/FrameworkLaunch/AddCustom/op_host/add_custom_tiling.h)与[add_custom.cpp](https://gitcode.com/HarmonyOS_Samples/cannkit_samplecode_add_custom_cpp/blob/master/FrameworkLaunch/AddCustom/op_host/add_custom.cpp)。

1. 修改“add_custom_tiling.h”文件，在此文件中增加粗体部分的代码，进行Tiling参数的定义。
    ```cpp
    #ifndef ADD_CUSTOM_TILING_H 
    #define ADD_CUSTOM_TILING_H 
    #include "register/tilingdata_base.h" 
    namespace optiling { 
    BEGIN_TILING_DATA_DEF(AddCustomTilingData) 
    // AddCustom算子使用了2个tiling参数：totalLength与tileNum 
    TILING_DATA_FIELD_DEF(uint32_t, totalLength);     // 总计算数据量 
    TILING_DATA_FIELD_DEF(uint32_t, tileNum);         // 每个核上总计算数据分块个数 
    END_TILING_DATA_DEF; 
    
    // 注册tiling数据到对应的算子 
    REGISTER_TILING_DATA_CLASS(AddCustom, AddCustomTilingData) 
    } 
    #endif // ADD_CUSTOM_TILING_H
    ```

2. 修改“add_custom.cpp”文件，进行Tiling的实现。
   修改“TilingFunc”函数，实现Tiling上下文的获取，并通过上下文获取输入输出shape信息，并根据shape信息设置TilingData，序列化保存TilingData，并设置TilingKey。

    ```cpp
    namespace optiling { 
    const uint32_t BLOCK_DIM = 1; 
    const uint32_t TILE_NUM = 8; 
    static ge::graphStatus TilingFunc(gert::TilingContext* context) 
    { 
        AddCustomTilingData tiling; 
        uint32_t totalLength = context->GetInputShape(0)->GetOriginShape().GetShapeSize(); 
        context->SetBlockDim(BLOCK_DIM); 
        tiling.set_totalLength(totalLength); 
        tiling.set_tileNum(TILE_NUM); 
        tiling.SaveToBuffer(context->GetRawTilingData()->GetData(), context->GetRawTilingData()->GetCapacity()); 
        context->GetRawTilingData()->SetDataSize(tiling.GetDataSize()); 
        size_t *currentWorkspace = context->GetWorkspaceSizes(1); 
        currentWorkspace[0] = 0; 
        return ge::GRAPH_SUCCESS; 
    } 
    } // namespace optiling
    ```
    
3. 在“add_custom.cpp”文件中实现AddCustom算子的shape推导。
   Add算子的输出shape等于输入shape，所以直接将输入shape赋给输出shape
    
4. 修改“add_custom.cpp”文件中的算子原型注册，此函数为入口函数。
    ```cpp
    namespace ops { 
    class AddCustom : public OpDef { 
    public: 
        explicit AddCustom(const char* name) : OpDef(name) 
        {  
            // Add算子的第一个输入 
            this->Input("x") 
                .ParamType(REQUIRED)    // 代表输入必选 
                .DataType({ ge::DT_FLOAT16, ge::DT_FLOAT, ge::DT_INT32 })   // 输入支持的数据类型 
                .Format({ ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND });   // 输入支持的数据格式 
            // Add算子的第二个输入 
            this->Input("y") 
                .ParamType(REQUIRED) 
                .DataType({ ge::DT_FLOAT16, ge::DT_FLOAT, ge::DT_INT32 }) 
                .Format({ ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND }); 
            this->Output("z") 
                .ParamType(REQUIRED) 
                .DataType({ ge::DT_FLOAT16, ge::DT_FLOAT, ge::DT_INT32 }) 
                .Format({ ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND }); 
            // 关联InferShape函数 
            this->SetInferShape(ge::InferShape); 
            // 关联Tiling函数 
            this->AICore() 
                .SetTiling(optiling::TilingFunc); 
            // 注册算子支持的AI处理器型号，请替换为实际支持的AI处理器型号,如kirinx90 
            this->AICore().AddConfig("kirinxxx"); 
        } 
    }; 
    // 结束算子注册 
    OP_ADD(AddCustom); 
    } // namespace ops
    ```

## 算子入图（GE图）开发
### 算子入图概述
图模式是神经网络模型的一种运行模式，在图模式下开发者首先将模型的计算过程构造成一张图，然后通过GE将图下发到Kirin硬件执行。相对于单个算子依次下发的方式，图模式下，GE可以通过计算图优化、多流并行、内存复用、模型下沉等技术手段，加速模型执行效率，减少模型内存占用。

算子入图的开发流程如下图所示，算子工程创建完成后，基于工程代码框架完成算子原型定义、Kernel侧算子实现、Host侧Tiling实现并完成算子入图开发，通过工程编译脚本完成算子的编译部署，之后即可基于图IR执行算子，比如单算子模型执行或者IR构图的方式调用自定义算子。该开发流程以工程化算子开发为基础，除了需要提供算子实现中的算子实现文件外，还需要额外交付算子入图的代码文件。

![算子入图流程](https://alliance-communityfile-drcn.dbankcdn.com/FileServer/getFile/cmtyPub/011/111/111/0000000000011111111.20251216162819.62019094373656597380607052696226:50001231000000:2800:433BE5510DC2A8ADABC6096E85BE450A11D902F335E7B2ACAFCB6056EC2F543D.png)

### 开发流程
假设下图是我们需要使用的网络模型，开发者可能会想直接逐个算子调用，根据输入tensor得到输出tensor就可以完成网络的运行，但在图模式场景下，实际的网络模型生成过程中，会先进行tensor shape以及datatype的推导。这样可以让我们在图执行之前，就知道各tensor的数据类型和形状，提前校验其正确性；同时提前推理出算子的输出张量描述，包括张量的形状、数据类型及数据排布格式等信息，算子构图准备阶段就可以为所有的张量静态分配内存，避免动态内存分配带来的开销。

下面的网络模型经过shape和datatype推导之后，可以得到灰色底纹框中的推导信息。

图1 shape与datatype推导示意图

![shape与datatype推导示意图](https://alliance-communityfile-drcn.dbankcdn.com/FileServer/getFile/cmtyPub/011/111/111/0000000000011111111.20251216162821.86298633453980218262584402283711:50001231000000:2800:644E52123009928645EE2D191D3E6EDDA4518A4EA98C023878FA6A89030CCBBB.png)

除了tiling实现外，算子入图时需要额外提供的实现代码有以下几种：

datatype推导：根据算子的输入datatype、算子逻辑及算子属性等信息，推理出算子的输出张量datatype。
shape推导：根据算子的输入shape、算子逻辑及算子属性等信息，推理出算子的输出张量shape。
声明数据依赖：部分算子在InferShape时，需要依赖某个输入的具体值才可以进行，这类算子被称为“数据依赖算子”，对应的输入被称为“数据依赖输入”。该类算子在注册时，需要声明其数据依赖输入。
下表列出了不同类型的算子对上述实现代码的要求。

表1 不同的类型的算子对入图实现代码的要求

| 分类 | 对入图实现代码的要求 |
| ---- | ---- |
| 根据输入shape可以推导出输出shape。 | - shape推导 <br> - datatype推导 |
| 依赖输入的value才能推导出输出shape，即数据依赖算子。 如Reshape算子，依赖shape输入的value才能推导出输出shape。| - shape推导 <br> - datatype推导 <br> - 声明数据依赖|


实际开发时通过固定的datatype和shape推导原型实现推导函数，然后再通过SetInferShape、SetInferDataType接口来关联对应的shape推导函数，样例如下。

```cpp
namespace ge { 
static graphStatus InferShape(gert::InferShapeContext *context) 
{ 
    // ... 
    return GRAPH_SUCCESS; 
} 
 
static graphStatus InferDataType(gert::InferDataTypeContext *context) 
{ 
    // ... 
    return ge::GRAPH_SUCCESS; 
} 
} // namespace ge 
 
 
namespace ops { 
class AddCustom : public OpDef { 
public: 
    AddCustom(const char* name) : OpDef(name) 
    { 
        this->Input("x") 
            .ParamType(REQUIRED) 
            .DataType({ge::DT_FLOAT16, ge::DT_FLOAT, ge::DT_INT32}) 
            .Format({ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND}); 
        this->Input("y") 
            .ParamType(REQUIRED) 
            .DataType({ge::DT_FLOAT16, ge::DT_FLOAT, ge::DT_INT32}) 
            .Format({ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND}); 
        this->Output("z") 
            .ParamType(REQUIRED) 
            .DataType({ge::DT_FLOAT16, ge::DT_FLOAT, ge::DT_INT32}) 
            .Format({ge::FORMAT_ND, ge::FORMAT_ND, ge::FORMAT_ND}); 
        // 根据开发者的算子调用方式决定需不需要注册 图模式调用方式下需要 
        this->SetInferShape(ge::InferShape); 
       this->SetInferDataType(ge::InferDataType);   
        this->AICore() 
            .SetTiling(optiling::TilingFunc); 
        // 请替换为实际的Kirin AI处理器型号 
        this->AICore().AddConfig("kirinx90"); 
    } 
}; 
OP_ADD(AddCustom); 
} // namespace ops
```

#### datatype推导
以AddCustom算子为例，InferDataType的实现如下所示。该样例中输出tensor的数据类型与输入tensor的数据类型相同，所以直接将任意一个输入tensor的数据类型赋给输出tensor即可。

```cpp
namespace ge { 
static graphStatus InferDataType(gert::InferDataTypeContext* context) 
{ 
    const auto inputDataType = context->GetInputDataType(0); 
    context->SetOutputDataType(0, inputDataType); 
    return ge::GRAPH_SUCCESS; 
} 
} // namespace ge
```
如下示例则给出了更灵活的datatype推导样例，当输入的数据类型为DT_INT4时，其输出的数据类型为DT_INT32
```cpp
ge::graphStatus InferDataTypeForFoo(gert::InferDataTypeContext* context) { 
 
    if (context->GetInputDataType(0) == DT_INT4) { 
        context->SetOutputDataType(0, DT_INT32); 
    } 
}
```

#### shape推导
简单的shape推导逻辑可以使用Follow接口来表达，比如输出shape和输入shape相同的情况。示例如下。

输出“y1”Follow输入“x1”场景，指定Follow模式为SHAPE，此时“y1”的shape将会和“x1”保持一致。

```cpp
this->Input("x1") 
    .ParamType(REQUIRED) 
    .DataType({ge::DT_FLOAT, ge::DT_FLOAT}) 
    .Format({ge::FORMAT_ND, ge::FORMAT_ND}); 
this->Input("x2") 
    .ParamType(REQUIRED) 
    .DataType({ge::DT_FLOAT, ge::DT_FLOAT}) 
    .Format({ge::FORMAT_ND, ge::FORMAT_ND}); 
this->Output("y1") 
    .ParamType(REQUIRED) 
    .DataType({ge::DT_FLOAT, ge::DT_FLOAT}) 
    .Format({ge::FORMAT_ND, ge::FORMAT_ND}) 
    .Follow("x1", FollowType::SHAPE);
```

无法在原型定义中通过Follow表达的情况需要开发者编写InferShape函数，其原型是确定的，接受一个InferShapeContext作为输入，从此context上可以获取到输入、输出的shape指针等内容。输入shape为const类型，因此InferShape时，输入shape是只读、不允许修改的。InferShape成功后，返回ge::GRAPH_SUCCESS，其他返回值被认为推导失败。推导失败后，执行过程结束退出。

以ReShape算子为例，InferShape的实现如下所示。根据第1个输入（shape输入）的值，Reshape算子将第0个输入（x输入）的shape做变换，并输出到其第0个输出（y输出）上。Reshape的InferShape实现为：
```cpp
ge::graphStatus InferShapeForReshape(InferShapeContext *context) { 
const gert::Shape *x_shape = context->GetInputShape(0);        // 获取第0个输入的shape 
const gert::Tensor *shape_tensor = context->GetInputTensor(1); // 获取第1个输入的tensor 
gert::Shape *output_shape = context->GetOutputShape(0); 
if (x_shape == nullptr || shape_tensor == nullptr || output_shape == nullptr) { 
    // 防御式编程，不应该出现的场景，打印错误并返回失败 
    return ge::GRAPH_FAILED; 
} 
 
auto reshape_size = static_cast<int32_t>(shape_tensor->GetShapeSize()); 
if (reshape_size < 1) { 
    // 防御式编程，不应该出现的场景，打印错误并返回失败 
    return ge::GRAPH_FAILED; 
} 
 
// 根据原型信息，Reshape的shape输入支持INT32与INT64两类，根据不同的类型进入对应的模板函数中做真正的shape变换操作 
if (shape_tensor->GetDataType() == ge::DT_INT32) { 
    int32_t *reshape_data = shape_tensor->GetData<int32_t>(); 
    return ReshapeInferShapeImpl<int32_t>(reshape_data, *x_shape, *output_shape, reshape_size); 
} else { 
    int64_t *reshape_data = shape_tensor->GetData<int64_t>(); 
    return ReshapeInferShapeImpl<int64_t>(reshape_data, *x_shape, *output_shape, reshape_size); 
} 
}
```
InferShapeContext public继承自ExtendedKernelContext，因此ExtendedKernelContext中提供的方法如获取算子type、name、属性等接口均可以在InferShapeContext实例中调用。

注意:
- InferShape推导函数和使用Follow接口去Follow shape不能混用，即不支持部分输出采用InferShape推导、部分输出采用Follow推导的情况。若开发者同时使用了InferShape函数和Follow接口，以开发者的InferShape函数为准，需要保证在InferShape函数中能够推导出所有的输出shape。
- 为了效率考虑，调用InferShape函数时，框架不会为输出shape做初始化，因此，在InferShape函数中，可以认为输出是未初始化的状态。如果在InferShape时，希望通过Append方式操作输出shape，需要先将输出shape的DimNum清零，以防止出现未定义行为。

#### InferShape时获取属性、输入
在InferShape、Tiling时，可以通过context实例获取算子IR属性值，所谓IR属性，是指在IR注册时定义的属性，以TransData算子为例：
```cpp
namespace ops { 
class TransData : public OpDef { 
public: 
    explicit TransData(const char *name) : OpDef(name) 
    { 
        this->Input("src") 
             // ... 
        this->Output("dst") 
             // ... 
        this->Attr("src_format") 
            .AttrType(REQUIRED) 
            .String(); 
        this->Attr("dst_format") 
            .AttrType(REQUIRED) 
            .String(); 
        this->Attr("group") 
            .AttrType(OPTIONAL)            
           .Int(1); 
        // ... 
    } 
}; 
OP_ADD(TransData); 
} // namespace ops
```
其原型定义中声明了src_format、dst_format、group三个属性，可以通过如下方式获取算子属性：
```cpp
ge::graphStatus ExampleGetTransDataAttr(TilingContext *context) { 
  // 获取所有属性 
  const RuntimeAttrs *attrs = context->GetAttrs(); 
  ASSERT_NOT_NULL(attrs); 
   
  // 按照在原型定义中的顺序，使用index获取属性，index从0开始计数 
  const char *src_format = attrs->GetAttrPointer<char>(0);  // 获取src_format，src_format是第一个属性，因此index为0 
  const char *dst_format = attrs->GetAttrPointer<char>(1);  // 获取dst_format，dst_format是第二个属性，因此index为1 
  const int64_t group = attrs->GetAttrPointer<int64_t>(2);  // 获取group，group是第三个属性，因此index为2 
   
  return ge::GRAPH_SUCCESS; 
}
```

#### 数据依赖
一般来说，具备输入shape后，算子可以通过InferShape推导出输出shape。然而部分算子在InferShape时，需要依赖某个输入的具体值才可以进行，这类算子被称为“数据依赖算子”，对应的输入被称为“数据依赖输入”。以Reshape算子为例，其依据shape输入的描述，对输入的shape做调整，因此Reshape算子依赖shape输入的值。这类算子需要在原型定义时通过ValueDepend接口声明对应的输入为数据依赖输入项。
```cpp
namespace ops { 
class Reshape : public OpDef { 
public: 
    explicit Reshape(const char *name) : OpDef(name) 
    { 
        // ... 
        this->Input("shape") 
             .ParamType(REQUIRED) 
             // ... 
             .ValueDepend(REQUIRED) // 声明 ReShape算子的shape输入为数据依赖输入 
        // ... 
    } 
}; 
OP_ADD(Reshape); 
} // namespace ops
```
根据第1个输入（shape输入）的值，Reshape算子将第0个输入（x输入）的shape做变换，并输出到其第0个输出（y输出）上。Reshape的InferShape实现为：
```cpp
ge::graphStatus InferShapeForReshape(InferShapeContext *context) { 
const gert::Shape *x_shape = context->GetInputShape(0);        // 获取第0个输入的shape 
const gert::Tensor *shape_tensor = context->GetInputTensor(1); // 获取第1个输入的tensor 
gert::Shape *output_shape = context->GetOutputShape(0); 
if (x_shape == nullptr || shape_tensor == nullptr || output_shape == nullptr) { 
    // 防御式编程，不应该出现的场景，打印错误并返回失败 
    return ge::GRAPH_FAILED; 
} 
 
auto reshape_size = static_cast<int32_t>(shape_tensor->GetShapeSize()); 
if (reshape_size < 1) { 
    // 防御式编程，不应该出现的场景，打印错误并返回失败 
    return ge::GRAPH_FAILED; 
} 
 
// 根据原型信息，Reshape的shape输入支持INT32与INT64两类，根据不同的类型进入对应的模板函数中做真正的shape变换操作 
if (shape_tensor->GetDataType() == ge::DT_INT32) { 
    int32_t *reshape_data = shape_tensor->GetData<int32_t>(); 
    return ReshapeInferShapeImpl<int32_t>(reshape_data, *x_shape, *output_shape, reshape_size); 
} else { 
    int64_t *reshape_data = shape_tensor->GetData<int64_t>(); 
    return ReshapeInferShapeImpl<int64_t>(reshape_data, *x_shape, *output_shape, reshape_size); 
} 
}
```
注意
- 只有声明过数据依赖的输入，才可以在InferShape时调用GetInputTensor等获取tensor的接口获取其对应的tensor数据。若对一个未声明数据依赖的输入调用GetInputTensor等获取tensor的接口，只能在tensor中获取到正确的shape、format、datatype信息，无法获取到真实的tensor数据地址（获取到的地址为nullptr）。
- 从tensor中获取tensor_data时(GetData<int32_t>或GetData<int64_t>)，使用者需要保证获取的数据类型是正确的，否则行为是未定义的。

## 算子工程编译部署

编译AddCustom工程，生成自定义算子安装包，并将其安装到算子库中。

1. 编译自定义算子工程，构建生成自定义算子包。
    在算子工程AddCustom目录下执行如下命令，进行算子工程编译。
    ```shell
    ./build.sh
    ```
    
    编译成功后，会在当前目录下创建build_out目录，在build_out/autogen目录下生成自定义算子交付件。
    
2. 自定义算子安装包部署。
   在执行编译的同时，会将交付件安装为如下目录结构

    
    查看部署后的目录结构，如下所示：
```
├── opp    //算子库目录
│   ├── vendors     //自定义算子所在目录
│       ├── config.ini
│       └── vendor_name1   // 存储对应厂商部署的自定义算子，此名字为编译自定义算子安装包时配置的vendor_name，若未配置，默认值为customize
│           ├── framework     //自定义算子插件库
│           ├── op_api
│           │   ├── include
│           │   │  └── aclnn_xx.h      //算子调用API声明文件
│           │   └── lib
│           │       └── libcust_opapi.so
│           ├── op_impl
│           │   └── ai_core
│           │       └── tbe
│           │           ├── config
│           │           ├── vendor_name1_impl    //自定义算子实现代码文件
│           │           │   └── dynamic
│           │           │       ├── xx.cpp
│           │           │       └── xx.py
│           │           ├── kernel     //自定义算子二进制文件
│           │           │   └── ${soc_version}     //昇腾AI处理器类型
│           │           │   └── config
│           │           └── op_tiling
│           │               ├── lib
│           │               └── liboptiling.so 
│           └── op_proto     //自定义算子原型库所在目录
│                ├── inc
│                │   └── op_proto.h
│                └── lib
│       ├── vendor_name2   // 存储厂商vendor_name2部署的自定义算子
```
# 算子调试调优

## 算子调试概述

### 工具介绍


#### 使用说明

介绍AscendC算子调测工具的调测流程和调测功能等，并提供命令行方式，以指导开发者在不同场景下对AscendC算子发起Tiling、精度、性能调测。

**表1** 工具指引

| 使用场景 | 操作指引 | 说明 |
| -------- | -------- | -------- |
| AscendC算子调测工具是什么 | [调测工具简介](#NPU仿真模式下的精度校验与性能采集分析) | 阐述了工具的基本概念、关键调测功能、调测方式和流程等，帮助开发者快速了解和使用工具。 |
| 算子调测过程中涉及哪些配置和调测功能 | [数据准备与配置说明](#数据准备和配置说明) | 阐述命令行方式下算子调测需要的**输入/标杆数据**、**算子信息json文件配置**等。 |


#### 使用约束

仅支持对静态Shape算子调测，暂不支持动态Shape算子调测。


### 基本概念

**表2** 基本概念

| 概念 | 描述 |
| -------- | -------- |
| 标准自定义算子工程场景(customize) | 该场景下，开发者需按照“工程创建-&gt;算子实现-&gt;编译部署-&gt;算子调用”的流程完成算子开发和调用。基于工程代码框架完成算子核函数的开发和Tiling实现，通过工程编译脚本完成算子编译部署。Tiling开发基于CANN提供的编程框架进行，后续更易于借助框架功能实现单算子API调用、图模式算子调用等。 |
| 输入数据和标杆数据 | 使用AscendC调测工具时，需要提供算子输入数据及标杆数据（Golden数据）。<br/>- 输入数据：固定shape算子运行时的输入数据，bin格式。<br/>- 标杆数据：根据输入数据计算出来的真值数据，用于与输出数据进行精度比对，bin格式。 |
| 算子json配置文件 | 用于描述算子的输入、输出及属性信息，以json格式存储。该文件中的参数个数、顺序、数据类型等信息，必须与算子原型相匹配。<br/>工具支持以下配置格式：<br/>[固定输入/输出顺序的算子json配置](#固定输入输出顺序的算子json配置)：其输入/输出参数信息单独在inputs、outputs、attrs配置项中设置，不支持输入/输出参数交叉设置。 |
| Tiling | （Kirin当前版本暂不支持Tiling调试）描述Kirin AI处理器上算子的输入/输出数据切分、分块计算、多核并行等策略，主要是为了满足片上存储限制和计算pipeline的需求，最大化计算并行性和数据局部性（data locality 或 data reuse），从而发挥硬件的极致性能。 |
| CAModel | CAModel(Cycle Accurate Model)仿真器主要用于生成算子性能仿真流水数据。 |
| CCEC | CCEC(Cube-based Computing Engine Compiler)工具基于毕昇编译器，通过自动拼接编译选项构建的一种编译方式。其中，毕昇编译器是一款专为达芬奇SOC设计的编译器，支持异构编程扩展，可将开发者编写的昇腾算子代码编译成二进制可执行文件和动态库等形式，详细介绍请参考《[毕昇编译器使用指南](https://www.hiascend.com/document/detail/zh/canncommercial/82RC1/index/index.html)》。 |
| CCEprint/Intrilog | intrilog：落盘算子运行中的CCE指令流。 |
| 原地算子(in-place op) | 一般指原地更新操作类算子，即算子的输入和输出为同一地址，算子在计算完成后，把原有的输出结果直接覆盖在输入的地址上，以减少不必要的内存占用。 |

输入数据和标杆数据可通过numpy生成，样例如下。
```python
import numpy as np
def gen_data_simple(): 
    input_x = np.random.uniform(-100, 100, [8, 2048]).astype(np.float16)
    input_y = np.random.uniform(-100, 100, [8, 2048]).astype(np.float16)
    golden = (input_x + input_y).astype(np.float16)
    input_x.tofile("./input/input_x.bin")     # 通过numpy生成的随机输入数据x
    input_y.tofile("./input/input_y.bin")     # 通过numpy生成的随机输入数据y
    golden.tofile("./output/golden.bin")      # 根据输入计算出来的真值数据，即标杆数据
```


### 运行流程

若采用工具命令行方式对单算子调测，其运行流程如图1所示。

**图1** 基于命令行的工具运行流程
![alt text](https://alliance-communityfile-drcn.dbankcdn.com/FileServer/getFile/cmtyPub/011/111/111/0000000000011111111.20251215183900.85214262072511646947492832791648:50001231000000:2800:F636DDAA4EA2BED5A4C6C9FBA236953F8EAA2AF4681797F38A0113B32B51417C.png)

1. 准备环境：使用本工具进行算子调测前，请先完成基础环境搭建，详细操作参见[环境准备](#环境准备)。

2. 准备数据：准备好bin格式的输入数据和标杆数据文件，该数据是调测工具的必要输入。


3. 通过命令行方式发起算子调测。
    目前工具支持的算子调测能力如下，请按需选择：

    - Tiling调测：对Tiling函数调测，生成Tiling结果，验证Tiling函数功能。（Kirin暂不支持）
    - 精度调试：通过NPU仿真进行核函数精度调试，支持自动精度比对、printf/PRINTF/DumpTensor/assert打印等功能。
    - 性能调优：通过NPU仿真获取算子执行circle数，评估算子性能。

## 数据准备和配置说明

### 输入数据和标杆数据准备

使用AscendC调测工具进行算子调测前，必须提供算子的[输入数据和标杆数据](#输入数据和标杆数据准备)。


- 输入数据（input数据）：固定shape算子运行时的输入数据，bin格式。

- 标杆数据（golden数据）：根据输入数据计算出来的真值数据，用于与输出数据进行精度比对，bin格式。


本章节提供多种方式来准备数据，开发者可以根据实际情况选择合适的方式。


- **方式：采用脚本生成input/golden数据。**
    开发者自行准备数据生成脚本（一般是.py文件）
    可以参考ops/ascendc/AddKernelInvocation/scripts/gen_data.py


## NPU仿真模式下的精度校验与性能采集分析
基于NPU域算子的调用接口编写的算子程序，通过毕昇编译器编译后生成可执行程序，运行可执行程序，可以完成算子NPU域的运行验证。使用算子调优工具运行NPU模式下生成的可执行文件从而采集Ascend C算子在AI处理器上执行的性能数据，进行性能精细调优。
当前，Kirin支持的芯片型号为KirinX90, Kirin9030，支持分析的类型为为仿真器，支持的场景为Kernel直调场景，关于Kernel直调场景的详情请参考 [Kernel直调说明](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0056.htmld)。
### 精度校验
样例参考本仓 ops/ascendc/AddKernelInvocation/ 目录
该工程的编译、执行参考 [样例工程下的README](../ops/ascendc/AddKernelInvocation/README.md)

样例工程的run脚本会自动编译算子直调程序的二进制，并结合ops/ascendc/AddKernelInvocation/scripts目录下的gen_data.py、verify_result.py进行精度校验

### 仿真性能流水图
在上述步骤编译获得 add_sim二进制之后，执行以下命令，可以使用msprof工具生成仿真模式下的算子性能流水图
```shell
msprof op simulator --soc-version=KirinX90 --output=XXX ./add_sim
```
msprof op simulator可选参数说明
|字段名|字段含义 |是否必选   |
|------|------|------|
|--application|建议使用msprof op simulator --soc-version=Kirinxxyy [msprof op simulator参数] ./app进行拉取，其中app为用户指定的可执行文件，如果app未指定路径，默认为使用当前路径。|是，--application 和 --export二选一|
|--export|(不推荐使用)指定包含单算子仿真性能结果文件夹，直接对该仿真结果进行解析，并生成单算子单核或多核的指令流水图。；与./app不共存，更推荐使用 ./app|是，--application 和 --export二选一|
|--soc-version|KirinX90 或者 Kirin9030|是|
|--output|性能分析结果的输出目录，该目录及其父目录的写权限不能开放给其他用户|否，如果不填，就输出在当前工作目录|
|--launch-count|设置可以采集算子的最大数量，默认值为1，取值范围为1~100之间的整数。| 用户无需关注，不涉及|
|--aic-metrics|使能算子性能指标采集。| 用户无需关注，不涉及|
|--core-id|该参数适用于算子分布均匀的情况时，可使用--core-id参数指定部分逻辑核的id，解析部分核的仿真数据。| 用户无需关注，不涉及|
|--timeout|该参数适用于数据量大且计算重复的算子，完整运行该类算子将会耗时很长，部分流水图即可获取必要信息。可通过设置--timeout参数缩短算子运行时长并获取必要流水信息| 用户无需关注，不涉及|
|--mstx|该参数决定算子调优工具是否使能用户代码程序中使用的mstx API。| 用户无需关注，不涉及|
|--mstx-include|该参数支持在msProf工具使能用户指定mstx API。|用户无需关注，不涉及|
|--dump|控制仿真器dump文件是否生成。<br>选项包括开启（on）和关闭（off），默认情况下设置为关闭（off），即不生成仿真器dump文件。|用户无需关注，不涉及|
|-h，--help|帮助信息|否|

**备注1**:如果要使用--export选项，请删除ops/ascendc/AddKernelInvocation/run.sh最后一行清理的步骤，使用生成的./sim_log文件夹



msprof命令执行完毕后会有 **Profiling results saved in XXXX**的日志，切换到提示的目录，该目录下 **simulator/trace.json**即指令流水图

在Chrome浏览器中输入chrome://tracing，将生成的trace.json文件拖到空白处打开，界面会呈现trace图。通过键盘上的快捷键（w：放大，s：缩小，a：左移，d：右移）查看各字段含义如下：



|字段名|字段含义|
|------|------|
|SCALAR|标量运算单元|
|VECTOR|向量运算单元|
|CUBE|矩阵乘运算单元|
|MTE1|数据搬运流水，数据搬运方向为：L1 ->{L0A/L0B, UBUF}|
|MTE2|数据搬运流水，数据搬运方向为：{GM} ->{L1, L0A/B, UBUF}|
|MTE3|数据搬运流水，数据搬运方向为：UBUF -> {GM, L1}|
|FIXP|数据搬运流水，数据搬运方向为：FIXPIPE L0C -> OUT/L1|
|FLOWCTRL|控制流指令|
|CACHEMISS|未命中icache|
|PUSHQ|向量运算单元相关指令|
|RVECEX|向量运算单元相关指令|
|RVECLD|向量运算单元相关指令|
|RVECST|向量运算单元相关指令|

# 兼容性指南
兼容性是确保开发者能够高效迁移算子的关键。为了保证在不同AI处理器之间迁移时，Ascend C算子的语义和功能保持一致，开发者需要根据目标平台的架构进行代码适配和优化。如果开发者希望在新平台上运行其他平台开发的Ascend C算子程序，需要在新平台上重新编译并运行。请参考[迁移指南](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/cannkit-ascend-kirin-compatibility)，了解如何将昇腾平台上开发的算子迁移到麒麟平台。

说明
- 为了保证兼容性，建议开发者在开发过程中尽量避免使用如下接口或编程方式，否则开发者需自行保证兼容性。
    - 非Ascend C公开接口、结构体，例如impl目录下的接口、结构体等；
    - 对芯片规格硬编码; 例如：
      ```cpp
      uint32_t MAX_UB_SIZE = 256 * 1024;
      // 建议获取方式：ascendcPlatform.GetCoreMemSize(platform_ascendc::CoreMemType::UB, MAX_UB_SIZE);
      // ...
      ```
    - 编译器BuiltIn API; 例如：
      ```cpp
      copy_gm_to_ubuf(input_1_local_ub, input_1, 0, 1, 8, 0, 0);
      ```
    - TilingKey编程。

- 建议开发者使用如下接口或编程方式。
    - 兼容API；
    - 若涉及特定领域扩展特性，在Device侧用[NPU_ARCH编译宏](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/cannkit-ascend910b-kirinx90)进行隔离，在Host侧用[SocVersion](https://gitcode.com/cann/asc-devkit/blob/master/docs/api/context/GetSocVersion.md)进行隔离；
    - 使用[Tiling模板编程](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_00025.html)。

# 附录: Ascend C API列表
详见[《Ascend C API列表》](https://gitcode.com/cann/asc-devkit/blob/master/docs/api/README.md)。
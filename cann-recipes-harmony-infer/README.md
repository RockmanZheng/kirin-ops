# cann-recipes-harmony-infer

## 🔥Latest News
- [2025/12] cann-recipes-harmony-infer项目首次上线。包含Ascend C自定义算子在支付宝端侧大模型、QQ音乐声伴分离、智谱GLM-Edge-1.5b-Chat、悟空图像本地闪绘等项目中的实践。

## 🚀概述
cann-recipes-harmony-infer 是面向鸿蒙开发者的技术实践库，基于CANN 平台提供典型业务实践案例，包括Ascend C 自定义算子开发实践，助力开发者高效实现端云协同迁移、经验复用与端侧推理部署。

## ✨样例列表
| 实践                                                                          |简介|
|-----------------------------------------------------------------------------|-----|
| [QuantMatmul](ops/ascendc/docs/custom-npu_quantMatmul.md)在支付宝端侧大模型实践        |基于Ascend C实现QuantMatmul自定义量化算子，在Kirin9030设备上高性能部署。
| [BandNorm](ops/ascendc/docs/custom-npu_bandNorm.md)在QQ音乐声伴分离实践              |实现QQ音乐声伴分离业务模型中的BandNorm等算子在鸿蒙设备上的高性能推理部署。|
| [RmsNorm](ops/ascendc/docs/custom-npu_rmsNorm.md)在智谱GLM-Edge-1.5b-Chat大模型实践 |基于Ascend C实现RmsNorm算子，在KirinX90设备上高性能部署。|
| [SliceGelu](ops/ascendc/docs/custom-npu_slice_gelu.md)在悟空图像SDXL大模型实践        |基于Ascend C实现SliceGelu融合算子，在KirinX90设备上高性能部署。|
| [SobelCustom](ops/ascendc/docs/custom-npu_sobel.md)在边缘检测CV处理实践              |基于Ascend C实现Sobel自定义算子，在KirinX90设备上高性能部署。|

## 📖目录结构说明
```
├── docs                                        # 文档目录
├── harmony_infer                               # 鸿蒙设备执行
|  ├── harmony_os_next                          # 鸿蒙next应用工程
|  ├── README.md
├── ops                                         # 算子目录
|  ├── ascendc
|  |  ├── AddKernelInvocation                   # 算子Kernel执行样例
|  |  ├── docs                                  # 自定义算子文档目录
|  |  |   ├── custom-xxx.md                     # xxx自定义算子文档
|  |  |   ├── ...
|  |  ├── src                                   # 自定义算子实现目录
|  |  |   ├── xxx_custom                        # xxx自定义算子实现
|  |  |   ├── ...
└── CONTRIBUTION.md                             # 本仓贡献指南
└── README.md                                   # 本仓公共README
└── ...
```

## 📝相关信息

- [贡献指南](./CONTRIBUTION.md)
- [许可证](./LICENSE)

    cann-recipes-harmony-infer仓涉及的模型，如模型目录下存在License的以该License为准。如模型目录下不存在License的，遵循Apache 2.0许可证，对应协议文本可查阅[LICENSE](./LICENSE)
    
- [免责声明](./DISCLAIMER.md)

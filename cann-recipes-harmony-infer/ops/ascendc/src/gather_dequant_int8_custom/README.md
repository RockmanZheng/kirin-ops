# GatherDequantInt8 自定义算子

## 简述

`GatherDequantInt8` 是一个基于 Ascend C 实现的 **per-row（per-token）非对称 INT8 embedding 查表 + 反量化** 融合算子，目标设备为 Kirin 9020 处理器。

在端侧部署带 embedding 的模型（如标点 / ASR 等）时，希望把 embedding 权重以 INT8 形式内置进图以压缩模型体积。但 9020 工具链的框架 Gather（`GatherV2D`）的 `data` 输入仅支持 fp32/fp16/int32，**不接受 uint8/int8**，导致"图内 INT8 embedding 查表"无法用标准算子表达。本算子在 AI Core 上直接以 uint8 读表、Cast 到 half、逐行反量化，使图内 embedding 可压到 INT8（约为 fp32 的 1/4），与外置反量化 bin 的体积对齐，且数值与 fp16 反量化路径一致。

数学表达式：

```
y[i, :] = (half(table[indices[i], :]) - zero_point[indices[i]]) * scale[indices[i]]
```

## 算子规格

| 名称        | 角色 | 数据类型 | 形状      | 说明 |
| :---------- | :--- | :------- | :-------- | :--- |
| table       | 输入 | uint8    | [V, E]    | 逐行非对称量化的 embedding 表 |
| indices     | 输入 | int32    | [...]     | token id（按元素查表，支持任意 shape） |
| scale       | 输入 | fp16     | [V]       | 逐行 scale |
| zero_point  | 输入 | fp16     | [V]       | 逐行 zero_point |
| y           | 输出 | fp16     | [..., E]  | 反量化后的 embedding |

- V：词表大小；E：embedding 维度。
- 输出 shape = `indices.shape ++ [E]`，dtype 固定 fp16。

## 支持的产品型号

- Kirin 9020 处理器系列产品

> 如需适配 Kirin X90 / 9030，请同步修改 `CMakePresets.json` 的 `ASCEND_COMPUTE_UNIT` 与 `op_host/gather_dequant_int8.cpp` 中 `AICore().AddConfig(...)`。

## 目录结构

```text
gather_dequant_int8_custom
├── build_and_install.sh                                # 编译安装脚本
├── CMakeLists.txt
├── CMakePresets.json                                   # 编译配置（ASCEND_CANN_PACKAGE_PATH / 算力型号）
├── framework
│   ├── CMakeLists.txt
│   └── onnx_plugin
│       ├── CMakeLists.txt
│       └── gather_dequant_int8_plugin.cpp              # ONNX 前端适配插件
├── op_host
│   ├── CMakeLists.txt
│   ├── gather_dequant_int8.cpp                         # 原型注册 / InferShape / InferDataType / Tiling
│   └── gather_dequant_int8_tiling.h                    # TilingData 定义
├── op_kernel
│   ├── CMakeLists.txt
│   └── gather_dequant_int8.cpp                         # 核函数实现
└── test
    ├── create_onnx.py                                  # 生成单算子 ONNX 测试模型
    └── gen_data.py                                     # 生成输入与 golden 数据
```

## 操作步骤

### 1. 环境准备

参考 [环境准备](../../../../docs/quick_install.md) 完成环境搭建，确保 Ascend-cann-toolkit 安装完成。编译前确认 `CMakePresets.json` 中 `ASCEND_CANN_PACKAGE_PATH` 指向正确的 toolkit 安装路径（一般为 `${install_path}/cann`）。

### 2. 编译安装

```bash
chmod +x build_and_install.sh
./build_and_install.sh
```

编译产物为自定义算子 run 包并自动安装到 `packages/vendors/customize/` 下。

### 3. 单算子测试

```bash
cd test
python3 create_onnx.py   # 生成 GatherDequantInt8.onnx
python3 gen_data.py      # 生成 table/indices/scale/zero_point.bin 与 golden output.bin
```

随后可通过 ATC 工具转换测试模型，调用鸿蒙维测接口完成单算子的精度与性能验证，详细流程参考 [ascendc 算子工程编译部署](../README.md)。

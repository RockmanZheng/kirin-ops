# 应用场景说明

在端侧部署带 embedding 的模型（如标点恢复、ASR 等）时，为压缩模型物理尺寸，常希望把 embedding 权重以 INT8 形式内置进计算图。然而 Kirin 9020 工具链的框架 Gather 算子（`GatherV2D`）的 `data` 输入仅支持 fp32/fp16/int32，**不支持 uint8/int8**，导致"图内 INT8 embedding 查表 + 反量化"无法用标准算子组合表达。

`GatherDequantInt8` 针对该场景，基于 Ascend C 在 NPU 上实现 uint8 读表 + 逐行（per-token）非对称反量化的融合算子，使图内 embedding 可压缩到 INT8（约为 fp32 的 1/4），在标点模型端到端部署中已验证与 fp32 embedding 的标点预测 argmax 100% 一致。

# GatherDequantInt8 自定义算子样例说明

本样例通过 Ascend C 编程语言实现了 `GatherDequantInt8` 算子，并提供 ONNX 框架适配插件，可通过 ATC/OMG 将含该自定义节点的 ONNX 模型转换为端侧离线模型。

在开发者自研 INT8 量化的 embedding 部署场景下，可使用本样例实现"图内 INT8 embedding 查表 + 反量化"，从而把 embedding 权重以 uint8 内置进图、压缩模型体积。

按照 工程创建 -> 算子实现 -> 编译部署 -> 算子调用 的流程完成算子开发，整个过程依赖算子工程：基于工程框架完成核函数与 Tiling 实现，通过编译脚本完成算子编译部署，继而实现单算子调用或第三方框架（ONNX）中的算子调用。

# 算子描述

`GatherDequantInt8` 算子对应的数学表达式为：

$$
y_{i} = (\mathrm{half}(table_{idx_i}) - zp_{idx_i}) \times scale_{idx_i}, \quad idx_i = indices_i
$$

- $table$ 为逐行非对称量化的 uint8 embedding 表
- $indices$ 为待查表的 token id
- $scale$ / $zp$ 为逐行（per-token）的缩放因子与零点
- $y$ 为反量化后的 fp16 embedding

逐行量化关系（与本样例 `test/gen_data.py` 一致）：

$$
scale_v = \frac{\max(W_v) - \min(W_v)}{255}, \quad zp_v = \frac{-\min(W_v)}{scale_v}, \quad q_{v} = \mathrm{round}(W_v / scale_v + zp_v)
$$

# 算子规格描述

| 名称        | 角色 | 数据类型 | 维度      | 说明 |
| :---------- | :--- | :------- | :-------- | :--- |
| table       | 输入 | uint8    | [V, E]    | 逐行非对称量化的 embedding 表 |
| indices     | 输入 | int32    | [...]     | token id，按元素查表，支持任意 shape |
| scale       | 输入 | fp16     | [V]       | 逐行 scale |
| zero_point  | 输入 | fp16     | [V]       | 逐行 zero_point |
| y           | 输出 | fp16     | [..., E]  | 反量化输出，shape = indices.shape ++ [E] |

# 支持的产品型号

本样例支持如下产品型号：
- Kirin 9020 处理器系列产品

# 编译安装执行

参考 [ascendc 算子工程编译部署](../src/README.md)，或本算子工程下的 [README](../src/gather_dequant_int8_custom/README.md)。

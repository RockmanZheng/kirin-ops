# CANN LLM Kirin Model Reproduction Notes

更新日期：2026-08-07

## 可转贴摘要

这次核对的是 HarmonyOS Samples 的 `cannkit_samplecode_lm_engine_cpp` 仓，当前本地克隆已同步到 `origin/master`，commit 为 `762941b62cc8273486a482fe15576de95df0e5f8`。

结论先行：

- 官方样例不是直接把 Hugging Face 原始权重放到麒麟芯片上跑，而是走“原始 PyTorch 权重 -> DOPT 三阶段量化 -> NPU 亲和 ONNX 导出 -> OMG/OMC 转换 -> 端侧 Demo 集成”的链路。
- 官方文档口径是“样例支持/DEMO 支持”这些模型；仓内没有提供已经跑完的实测日志、性能数据或可直接下载的量化后权重包。
- 按官方量化说明，端侧部署模型需要压缩/量化。核心精度口径是 `16-4 grouplinear`：decoder linear 权重 4 bit，activation/input 推荐 16 bit；embedding 默认分离并生成 int8 权重和反量化 scale；KV cache 在 OMC 转换命令中是 FP16，`lm_logits` 输出是 FP32。
- 因此我们现在能提前准备原始权重下载脚本、校准数据集、DOPT 配置模板、OMC 转换配置、端侧文件传输清单；真正“跑通”仍需要拿到真机、对应平台插件和设备侧验证日志。

## 官方样例模型和硬件矩阵

| 模型 | 官方支持硬件 | 是否需要量化/压缩 | 官方样例精度口径 | 需要准备的量化/转换动作 | 复现优先级 |
| --- | --- | --- | --- | --- | --- |
| Qwen2.5-1.5B | Kirin9020、Kirin x90 | 是 | W4A16 group-linear；embedding int8 分离；KV FP16；logits FP32 | DOPT 三阶段量化；`quant_param_2` 在 Kirin9020 用 true、Kirin x90 用 false；OMG 转 OMC 时带 `--compress_conf quant_params_file` | 最高，建议第一个跑 |
| DeepSeek-R1-Distill-Qwen-1.5B | Kirin9020、Kirin x90 | 是 | 同上，Qwen2 架构路径 | 同上，走 `Deepseek_1b5` OMC 分支 | 高，建议第二个跑 |
| Glm-1.5B / glm-edge-1.5b-chat | Kirin x90 | 是 | 同一套 16-4 量化口径 | `model_arch: zhipu`，使用 GLM 导出脚本和 `Glm_1b5` OMC 分支 | 中，只适合 x90 路径 |
| Qwen2.5-7B-Instruct | Kirin x90 | 是 | 同上 | Qwen2 导出路径，走 `Qwen25_7b` OMC 分支；权重和中间产物更大 | 低，等 1.5B 链路稳定后再跑 |
| Qwen3-8B | Kirin x90 | 是 | 同上 | `model_arch: qwen3`，使用 Qwen3 导出脚本和 `Qwen3_8b` OMC 分支 | 低，最后验证 |

## 压缩和量化口径

官方文档明确说明：大模型在资源受限的手机/PC 等设备部署时需要使能量化，CANN LLM 提供对应量化工具链。量化输入是原始 PyTorch 模型和参与量化的数据集，输出是量化后的模型与量化配置文件。

量化链路分三阶段：

1. 权重量化：生成并手工调整 `dopt_config.json`，再得到 `trained_quant_weight.pth`。
2. 激活量化：用校准样本生成 `trained.pth`。
3. 量化参数提取：生成 `fake_quant_weight.pth`、`quant_params_file`、embedding 权重文件和 embedding 反量化 scale 文件。

关键配置口径：

- decoder 层策略：`Quant_act_weight_eco`
- lm head 层策略：`Quant_lm_head`
- embedding 层策略：`Quant_Embed_MinMax`
- 权重量化位宽：官方推荐/支持口径为 4 bit，不建议尝试其他位宽
- 激活位宽：支持 8 bit 或 16 bit，推荐 16 bit
- group size：支持 64、128、256，文字推荐 128；样例 JSON 片段里使用 64，需要复现时统一配置并记录选择
- `quant_param_2`：Kirin x90 默认 false，Kirin9020 默认 true
- `embedding_separate`：默认 true，embedding 单独保存为 bin 文件

## 我们可提前准备的工作

| 准备项 | 当前状态 | 说明 |
| --- | --- | --- |
| 原始权重下载脚本 | 已准备 | `scripts/prepare-cann-llm-model-downloads.sh` 默认 dry-run，只生成/打印下载命令，不会实际下载 |
| 模型优先级 | 已确定 | 优先 Qwen2.5-1.5B，其次 DeepSeek-R1-Distill-Qwen-1.5B |
| 校准数据集 | 待准备 | DOPT 激活量化需要样本，先按官方配置 `num_samples: 256` 准备一版 JSON/text 校准集 |
| DOPT 配置模板 | 待准备 | 需要预生成每个模型的 `config.yaml` 和 `dopt_config.json` 修改模板 |
| DDK_tools 和平台插件 | 待获取 | 需要 `tools_dopt`、`tools_omg`，以及 `kirin9020` 或 `kirinx90` 平台插件；官方入口通常需要人工下载 |
| ONNX 导出配置 | 待准备 | 需要为每个模型填 `model_info_target.yaml`，并确认 `model_arch`、`hf_model_path`、`config_file`、`quant_pth`、输出目录 |
| OMC 转换脚本 | 待加固 | 样例 `to_omc.sh` 是模板，路径和 `--platform` 仍是占位符；条件判断写法也需要修正后再执行 |
| 端侧传输清单 | 待准备 | 最终需要传 `.omc`、`SubGraph_0.weight`、embedding 权重/scale、`context.json`、`executor.json`、`tokenizer.json` |
| 真机验收脚本 | 待真机接入后补齐 | 需要显式指定目标设备和设备内 `model_run_tool` 路径，不能在多设备环境里猜测 runner |

## 复现建议

第一阶段先只做 Qwen2.5-1.5B：

1. 只生成原始权重下载命令，不下载：`scripts/prepare-cann-llm-model-downloads.sh --model qwen25-1b5`
2. 准备 256 条左右校准样本。
3. 在 GPU Linux 环境跑 DOPT 三阶段量化，产出 `fake_quant_weight.pth`、`quant_params_file`、embedding 权重/scale。
4. 用 `export_model_single_qwen2.py` 导出 NPU 亲和 ONNX。
5. 用加固后的 OMG 命令生成 `.omc` 和外置 `SubGraph_0.weight`。
6. 拿到真机后，传输七类端侧文件，通过 CANN LLM Engine Demo 验证对话输出和日志。

第二阶段再复现 DeepSeek-R1-Distill-Qwen-1.5B。等 1.5B 链路稳定后，再决定是否推进 Kirin x90 上的 GLM 1.5B、Qwen2.5-7B、Qwen3-8B。

## 风险和注意事项

- 当前还没有真机，因此只能说“官方样例支持路径已梳理”，不能说“我们已在麒麟真机上跑通模型”。
- 官方样例仓没有提供实测性能表或可直接复用的量化权重，需要我们自己跑量化和转换。
- DOPT 三阶段量化文档写明只支持 GPU 环境，本机 macOS 主要适合做下载、配置和文档准备，不适合直接跑这一步。
- 官方 `executor.json` 示例里存在一个 embedding scale 路径字符串的明显引号问题，后续生成配置时不要原样复制。
- 样例 `to_omc.sh` 目前是模板性质，执行前需要修正路径、平台、模型分支判断和输出目录。

## 证据索引

- 官方样例仓：`https://gitcode.com/HarmonyOS_Samples/cannkit_samplecode_lm_engine_cpp`
- 本地样例说明：`cannkit_samplecode_lm_engine_cpp/CANN_LLM/CANN_LLM_Engine_Guide/CANN LLM 大语言模型解决方案.md`
- 导出配置模板：`cannkit_samplecode_lm_engine_cpp/CANN_LLM/CANN_LLM_Engine_Model/npu_tuned_export/model_info_target.yaml`
- embedding int8 处理：`cannkit_samplecode_lm_engine_cpp/CANN_LLM/CANN_LLM_Engine_Model/npu_tuned_export/do_opt.py`
- Qwen2 导出脚本：`cannkit_samplecode_lm_engine_cpp/CANN_LLM/CANN_LLM_Engine_Model/npu_tuned_export/export_model_single_qwen2.py`
- OMC 转换模板：`cannkit_samplecode_lm_engine_cpp/CANN_LLM/CANN_LLM_Engine_Model/scripts_for_omc/to_omc.sh`
- 权重下载脚本：`scripts/prepare-cann-llm-model-downloads.sh`
- Huawei CANN Kit 准备文档：`https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/cannkit-preparations`
- Huawei ONNX 转 CANN 文档：`https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/cannkit-llm-onnx2cann`

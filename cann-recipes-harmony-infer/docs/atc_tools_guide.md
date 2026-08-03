# ATC模型转换指南
ATC是异构计算架构CANN体系下的模型转换工具，它可以将开源框架的网络模型以及Ascend IR定义的单算子描述文件（JSON格式）转换为昇腾、麒麟AI处理器支持的离线模型格式。
##  环境准备
1. 参见[环境准备](./quick_install.md)进行环境搭建，并确保开发套件Ascend-cann-toolkit安装完成。该场景下ATC工具安装在`${install_path}/cann/bin`目录下；其中，`${install_path}`请替换为CANN软件安装目录，以root安装举例，安装目录为：`/usr/local/Ascend/ascend-toolkit`。

2. 安装CANN软件后，使用CANN运行用户进行编译、运行时，需要以CANN运行用户登录环境，执行如下环境变量：
```bash
# 默认路径安装，以root用户为例（非root用户，将/usr/local替换为${HOME}）
source /usr/local/Ascend/cann-${cann_version}/set_env.sh
# 指定路径安装
# source ${install_path}/cann-${cann_version}/set_env.sh
```
## 模型转换
### ONNX网络模型转换成离线模型
```bash
atc --model=$HOME/module/resnet50*.onnx --framework=5 --output=$HOME/module/out/onnx_resnet50 --soc_version=<soc_version>
```
   - --model：Resnet50网络模型文件所在路径。
   - --framework：原始框架类型，5表示ONNX。
   - --output：生成的离线模型路径。
   - --soc_version：Kirin AI处理器的型号，如KirinX90、Kirin9030等。
### *.air格式的模型文件转换成离线模型
```bash
atc --model=$HOME/module/ResNet50.air --framework=1 --output=$HOME/module/out/ResNet50_air --soc_version=<soc_version>
```
   - --model：*.air格式的模型文件所在路径。
   - --framework：原始框架类型，1表示*.air格式的模型文件。
   - --output：生成的离线模型路径。
   - --soc_version：Kirin AI处理器的型号，如KirinX90、Kirin9030等。
### 单算子描述文件转换成离线模型
单算子描述文件是基于Ascend IR定义的单个算子的定义文件，包括算子的输入、输出及属性等信息，借助该文件转换成适配昇腾、麒麟AI处理器的离线模型后，可以验证单算子的功能。详细配置请参考[单算子描述文件配置](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/devaids/atctool/atlasatc_16_0034.html)。
```bash
atc --singleop=$HOME/singleop/add.json --output=$HOME/singleop/out/op_model --soc_version=<soc_version>
```
   - --singleop：用于指定add.json单算子描述文件。
   - --output：转换后的离线模型存放路径。
   - --soc_version：Kirin AI处理器的型号，如KirinX90、Kirin9030等。

## 附录
|ATC参数名称   |参数简述     |是否必选   |默认值   |
| ------------ | ------------ | ------------ | ------------ |
|--help或--h   |显示帮助信息。   |否   |不涉及   |
|--model   |	原始模型文件路径与文件名。   |是   |不涉及   |
|--framework   |原始框架类型。   |是   |不涉及   |
|--input_format | 输入数据格式。 | 否 | Caffe、MindSpore、ONNX默认为NCHW；TensorFlow默认为NHWC |
|--input_shape| 模型输入数据的shape。 | 否 | 不涉及 |
|--singleop| 单算子定义文件，将单个算子JSON文件转换成适配昇腾AI处理器的离线模型。 | 否 | 不涉及 |
| --output | ● 如果是开源框架的网络模型，存放转换后的离线模型的路径以及文件名。<br>● 如果是单算子描述文件，存放转换后的单算子模型的路径。 | 是 | 不涉及 |
| --output_type | 指定网络输出数据类型或指定某个输出节点的输出类型。 | 否 | 不涉及 |
| --soc_version | 模型转换时指定芯片版本。 | 是 | 不涉及 |
| --core_type | 设置网络模型使用的Core类型，若网络模型中包括Cube算子，则只能使用 AiCore。 | 否 | AiCore |
| --out_nodes | 指定输出节点。 | 否 | 不涉及 |
| --external_weight | 生成om离线模型时，是否将原始网络中的Const/Constant节点的权重保存在单独的文件中，同时将节点类型转换为 FileConstant类型。 | 否 | 0 |
| --precision_mode | 设置网络模型的精度模式。 | 否 | force_fp16 |
| --precision_mode_v2 | 设置网络模型的精度模式。与--precision_mode不能同时使用，推荐使用--precision_mode_v2。 | 否 | fp16 |
| --log | 设置ATC模型转换过程中显示日志的级别。 | 否 | null |

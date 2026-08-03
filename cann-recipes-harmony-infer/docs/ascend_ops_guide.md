# 昇腾算子开发环境配置与编译指南



## 1.环境准备


### 1.1安装CANN包

CANN（Compute Architecture for Neural Networks）是昇腾AI计算引擎，提供算子开发所需的工具链和运行时支持。

#### 1.1.1获取安装包

从华为云镜像仓库下载对应版本的CANN包（可自行选择日期）：

```
https://ascend.devcloud.huaweicloud.com/artifactory/cann-run-mirror/software/legacy/
```

根据目标硬件平台下载相应版本：

| 参数                      |说明|
|--------------------------|-----|
| `${cann_version}`        |CANN包版本号（如`9.0.0`）|
| `${arch}`                |CPU架构（`aarch64`或`x86_64`）|
| `${soc_name}`            |NPU型号名称（如`910b`）|
| `${install_path}`        |指定安装路径，ops包需与toolkit包安装至相同路径；root用户默认安装至`/usr/local/Ascend`|


#### 1.1.2安装CANN Toolkit包

Toolkit包包含算子开发所需的编译器、调试器、性能分析器等开发工具。

```bash
# 赋予可执行权限
chmod +x Ascend-cann-toolkit_${cann_version}_linux-${arch}.run
# 执行安装
./Ascend-cann-toolkit_${cann_version}_linux-${arch}.run --install --install-path=${install_path}
```

#### 1.1.3安装CANN Ops包（运行态依赖）

Ops包提供算子运动时的动态库依赖。**注意**：若仅进行算子编译（不运行），可跳过此步骤。


```bash
# 赋予可执行权限
chmod +x Ascend-cann-${soc_name}-ops_${cann_version}_linux-${arch}.run
# 执行安装
./Ascend-cann-${soc_name}-ops_${cann_version}_linux-${arch}.run --install --install-path=${install_path}
```

#### 1.1.4配置环境变量

```bash
source ${install_path}/cann/set_env.sh
```


---


### 1.2下载源码

从官方仓库获取与CANN版本配套的源码分支：

```bash
# 克隆ops-math仓库
git clone https://gitcode.com/cann/ops-math.git
```


---


### 1.3安装依赖


#### 1.3.1安装系统依赖

项目根目录提供`install_deps.sh`脚本，可一键安装系统依赖：

```bash
bash install_deps.sh
```

>**说明**：若脚本不支持当前操作系统，请参考脚本内容自行适配。


#### 1.3.2安装Python依赖

```bash
pip3 install -r requirements.txt
```


---


## 2.编译KirinX90平台is_finite算子


在项目根目录下执行编译命令：

```bash
bash build.sh --pkg --soc=KirinX90 --ops=is_finite -j16
```

编译产物位于`build_out/`目录。

### 2.1已知问题与解决方案

**问题描述**：当前Kirin代码未适配图灵架构（Turing）,直接编译会遇到错误。

**解决方案**：需额外安装`kirin-bisheng`子包。

1.下载工具包：

下载对应`Ascend-cann-toolkit_${cann_version}_linux-${arch}-mobile-station.run`包，下载链接为[toolkit x86_64包](https://kiri-obs.obs.cn-north-4.myhuaweicloud.com/Cann%20Large%20Model%20Foundation%208.5.0.rc001/Ascend-cann-toolkit_9.0.0_linux-x86_64-mobile-station.run)。

2.执行安装：

```bash
# 解压安装包
./Ascend-cann-toolkit_9.0.0_linux-x86_64-mobile-station.run --tar xvf -C ./path
# 安装bisheng编译器
cd ./path/run_package/
./cann-bisheng-compiler_9.0.0_linux-x86_64.run --full --quiet --install-path=${install_path}
```


---


## 3.安装编译产物


将编译生成的算子包安装至目标环境：

```bash
./build_out/cann-ops-math-custom_linux-x86_64.run
```


---


## 4.配置环境变量


```bash
export LD_LIBRARY_PATH=${ASCEND_HOME_PATH}/opp/vendors/custom_math/op_api/lib:${LD_LIBRARY_PATH}
```

>**说明**：`${ASCEND_HOME_PATH}`应替换为CANN实际安装路径。


---


## 5.执行算子样例


```bash
bash build.sh --run_example is_finite eager cust --vendor_name=custom
```


---


## 6.生成单算子OMC模型


使用`atc`工具将算子模型转换为适配目标硬件的OMC格式：

```bash
atc --singleop=is_finite_3_6_5_float16.singleop_json --soc_version=KirinX90 --output=./model --op_debug_level=1 &>atc.log
```

### 参数说明

| 参数                      |说明|
|--------------------------|-----|
| `--singleop`             |单算子模型定义文件|
| `--soc_version`          |目标soc型号（如KirinX90）|
| `--output`               |输出模型文件目录|
| `--op_debug_level`       |调试级别（0-3）|


---


## 7.使用HAP在KirinX90设备上进行推理


### 7.1安装HAP应用

```bash
# 通过hdc安装hap包
hdc -t ${SN} install entry-default-signed.hap
```

### 7.2验证安装

```bash
# 检查已安装应用列表
hdc -t ${SN} shell bm dump -a
```

若输出中包含`com.example.naticvetestdemo`，说明安装成功。

### 7.3传输必要文件

将以下文件传输至设备执行目录：

- `.bin` 输入数据文件
- `.omc` 算子模型文件

```bash
hdc -t ${SN} file send {files} /mnt/hmdfs/100/account/device_view/local/files/Docs/Download/com.example.naticvetestdemo
```

### 7.4启动推理

```bash
hdc -t ${SN} shell aa start -a EntryAbility -b com.example.naticvetestdemo --ps path float16_input_t_is_finite.bin --ps omPath is_finite_3_6_5_float16.omc
```

>**参数说明**：
> - `float16_input_t_is_finite.bin`：输入数据文件
> - `is_finite_3_6_5_float16.omc`：omc算子模型文件
> - 推理完成后，结果输出至 `output0.bin`文件

### 7.5卸载HAP

```bash
# 通过hdc安装hap包
hdc -t ${SN} uninstall com.example.naticvetestdemo
```


---


## 附录：变量说明


| 变量名                    |说明|      示例值    |
|--------------------------|-----|------------------------|
| `${cann_version}`        |CANN包版本号|`9.0.0`|
| `${arch}`                |CPU架构|`aarch64`、`x86_64`|
| `${soc_name}`            |NPU型号名称|`910b`|
| `${install_path}`        |CANN安装路径|`/usr/local/Ascend`|
| `${ASCEND_HOME_PATH}`    |CANN主目录|`/usr/local/Ascend`|
| `${SN}`                  |设备序列号|-|
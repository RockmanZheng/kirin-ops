# 算子工程编译安装指南
本项目开始前，请参考[环境准备](../../../docs/quick_install.md)进行环境搭建，并确保开发套件Ascend-cann-toolkit安装完成。
> **说明**：编译前请确认各算子目录CMakePresets.json里`ASCEND_CANN_PACKAGE_PATH`字段设置为正确的toolkit包安装路径，一般为`${install_path}/cann`。
# 编译安装
切换到相应算子工程目录下执行以下命令，完成对应算子的编译安装：

```bash
 # 为脚本添加执行权限
 chmod +x build_and_install.sh
 # 执行编译安装
 ./build_and_install.sh
```

# 算子调试
- 参考[AddKernelInvocation](../AddKernelInvocation/README.md)样例，将kernel实现部分和相关头文件引用拷贝到add_custom.cpp，执行npu仿真调试。

- 您还可以执行相应算子test目录下的create_onnx.py和gen_data.py，生成相应的onnx单算子测试模型、输入、golden数据后，通过[ATC工具](../../../docs/atc_tools_guide.md)转换测试模型，调用鸿蒙维测接口完成单算子的性能和精度验证。详细流程请参考[维测调优](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/cannkit-debugging-and-optimization)。
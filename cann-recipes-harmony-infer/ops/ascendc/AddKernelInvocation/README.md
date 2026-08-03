## 目录结构介绍
``` 
├── AddKernelInvocation
│   ├── cmake                   // 编译工程文件
│   ├── input                   // 存放脚本生成的输入数据目录
│   ├── output                  // 存放算子运行输出数据和真实数据的目录
│   ├── scripts
│   │   ├── acl.json            // acl配置文件
│   │   ├── gen_data.py         // 验证输出数据和针织数据是否一致的验证脚本
│   │   ├── verify_result.py    // 真值对比文件
│   │── add_custom.cpp          // 算子kernel实现，暴露给main.cpp调用
│   │── CMakeLists.txt          // 编译工程文件
│   │── data_utils.h            // 数据读入写出函数
│   │── main.cpp                // 主函数，调用算子的应用程序，含CPU域及NPU域调用
│   ├── run.sh                  // 编译运行算子的脚本
``` 


## 替换自定义算子
如果将该样例工程替换为自己的算子实现，需要修改以下内容
1.add_custom.cpp为核函数实现，整个文件都要替换成新算子实现
此外，在以下文件中搜索 “迁移算子修改点”
2.run.sh
根据实际算子名，修改FILE_NAME属性
3.scripts/gen_data.py
需自定义生成输入数据的内容（Shape/数据类型/算子的输入参数数量）
4.main.cpp
调用add_custom.cpp中的符号
读数据的逻辑

## 代码实现介绍
本调用样例中实现的是固定shape为8*2048的Add算子。
- kernel实现   
  Add算子的数学表达式为：  
  ```
  z = x + y
  ```
  计算逻辑是：Ascend C提供的矢量计算接口的操作元素都为LocalTensor，输入数据需要先搬运进片上存储，然后使用计算接口完成两个输入参数相加，得到最终结果，再搬出到外部存储上。  
  
  Add算子的实现流程分为3个基本任务：CopyIn，Compute，CopyOut。CopyIn任务负责将Global Memory上的输入Tensor xGm和yGm搬运到Local Memory，分别存储在xLocal、yLocal，Compute任务负责对xLocal、yLocal执行加法操作，计算结果存储在zLocal中，CopyOut任务负责将输出数据从zLocal搬运至Global Memory上的输出Tensor zGm中。具体请参考[add_custom.cpp](./add_custom.cpp)。

- 调用实现  
  1. CPU侧运行验证主要通过ICPU_RUN_KF CPU调测宏等CPU调测库提供的接口来完成；  
  2. NPU侧运行验证主要通过使用<<<>>>内核调用符来完成。

  应用程序通过ASCENDC_CPU_DEBUG 宏区分代码逻辑运行于CPU侧还是NPU侧。
## 运行样例算子
- 打开样例目录

  ```
  cd $HOME/ops/AddKernelInvocation
  ```
- 配置环境变量

  这里的$HOME需要替换为本仓根目录
  ```
  export ASCEND_INSTALL_PATH=$HOME/Ascend/ascend-toolkit/latest
  ```

- 样例执行

  ```
  bash run.sh -r [RUN_MODE] -v  [SOC_VERSION] 
  ```
  - RUN_MODE：编译方式，sim（NPU仿真）。
  - SOC_VERSION：KirinX90 或者 Kirin9030。

  示例如下。
  ```
  bash run.sh -r sim -v KirinX90
  ```   


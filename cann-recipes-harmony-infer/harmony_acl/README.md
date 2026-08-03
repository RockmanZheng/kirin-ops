# AscendCL 
<span style="font-size:24px;">harmony_acl提供了和昇腾平台统一的ACL接口，方便开发者基于同一套接口部署模型推理</span>

### 前置条件
#### 开发环境
使用ubuntu 24.04LTS以上版本，并安装build-essential包，包含主机端编译环境，用于构建UT。可使用如下命令安装
`sudo apt-get install build-essential`

#### 项目编译工具
打开网址https://developer.huawei.com/consumer/cn/download/command-line-tools-for-hmos, 下载linux环境下native开发环境commandline-tools-linux-x64, 并参考https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/build-with-ndk-cmake, 搭建ohos开发环境。将host主机上刚下载解压的command-line-tools所在目录配置到build.sh中的CMD_LINE_TOOLS_DIR变量中。

#### 项目编译依赖环境准备
1. 访问CANN仓获取依赖头文件
默认不需要手动处理，在build.sh脚本中自动获取。如果build脚本中git命令失败，可参考以下网址点击gitcode中下载按钮下载文件包。
下载的文件包放到cann-recipes-harmony-infer/harmony_acl/depends目录

        (1) runtime/acl  访问网址https://gitcode.com/cann/runtime/tree/master/include/external/acl, 下载文件包名为runtime-master-include-external-acl.zip，放置到depends目录。

        (2) ge  访问网址https://gitcode.com/cann/ge/tree/master/inc，下载文件包名为ge-master-inc.zip，放置到depends目录。

        (3) metadef 访问网址https://gitcode.com/cann/metadef/tree/master/inc，下载文件包名为metadef-master-inc.zip，放置到depends目录。

2. 获取json处理头文件
处理方式同上，默认无需手动处理，如果脚本执行失败，访问网址https://gitcode.com/GitHub_Trending/js/json/tags/v3.12.0，下载v3.12.0版本source(zip)，并放置到depends目录中。

3. 安装gtest开发包
可使用如下命令安装`sudo apt-get install libgtest-dev`

### 编译
执行完上述依赖操作后，请到仓cann-recipes-harmony-infer源码目录下的harmony_acl目录执行该脚本./build.sh

### 测试运行
ut运行，编译完成后，cd进入build/ut目录，执行命令export LD_LIBRARY_PATH=`pwd`配置环境变量后执行./ascendcl_ut

### 开发指导
ascendcl库头文件在cann-recipes-harmony-infer/harmony_acl/inclue目录下，库文件libhiai_ascendcl.so在cann-recipes-harmony-infer/harmony_acl/build目录下。集成使用请参考docs目录下文档。
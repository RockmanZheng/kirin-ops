# 前提条件
本项目源码编译用到的前置依赖如下，请注意版本要求。
   - python >= 3.7.0
   - gcc >= 7.3.0
   - cmake >= 3.16.0
   - pigz（可选，安装后可提升打包速度，建议版本 >= 2.4）
   - dos2unix
   - Gawk

   上述依赖包可通过项目根目录install\_deps.sh安装，命令如下，若遇到不支持系统，请参考该文件自行适配。
   ```bash
   bash install_deps.sh
   ```
   
   > **备注**：依赖glibc 2.34以上的版本，推荐使用ubunt22.04以上的环境进行部署。
# 环境准备

1. **安装鸿蒙社区版CANN开发套件包**
   
   下载对应`Ascend-cann-toolkit_${cann_version}_linux-${arch}-mobile-station.run`包，下载链接为[toolkit x86_64包](https://kiri-obs.obs.cn-north-4.myhuaweicloud.com/Cann%20Large%20Model%20Foundation%208.5.0.rc001/Ascend-cann-toolkit_9.0.0_linux-x86_64-mobile-station.run)。
    
    ```bash
    # 确保安装包具有可执行权限
    chmod +x Ascend-cann-toolkit_${cann_version}_linux-${arch}-mobile-station.run
    # 安装命令
    ./Ascend-cann-toolkit_${cann_version}_linux-${arch}-mobile-station.run --install --force --install-path=${install_path}
    ```
    - \$\{cann\_version\}：表示CANN包版本号。
    - \$\{arch\}：表示CPU架构，如aarch64、x86_64。
    - \$\{install\_path\}：表示指定安装路径，默认安装在`/usr/local/Ascend`目录。

2. **配置环境变量**
	
	根据实际场景，选择合适的命令。

    ```bash
   # 默认路径安装，以root用户为例（非root用户，将/usr/local替换为${HOME}）
   source /usr/local/Ascend/cann-${cann_version}/set_env.sh
   # 指定路径安装
   # source ${install_path}/cann-${cann_version}/set_env.sh
    ```
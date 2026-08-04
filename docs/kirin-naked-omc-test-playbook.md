# Kirin Naked OMC Model Run Tool Playbook

这份 playbook 是给“直接在 HarmonyOS 真机上测试裸 `.omc` 算子/模型文件”的 spike 用的。

## 结论

根据 issue #1 里真实物理机器的 shell history，这台 HarmonyOS PC 上的惯例不是安装 HAP runner，而是使用设备上的 native CLI：

```text
/data/local/tmp/model_run_tool
```

因此当前标准测试路径是：

```bash
hdc -t "$SN" file send <model.omc> /data/local/tmp/<model.omc>
hdc -t "$SN" file send <input.bin> /data/local/tmp/<input.bin>
hdc -t "$SN" shell "/data/local/tmp/model_run_tool --model=/data/local/tmp/<model.omc> --input=/data/local/tmp/<input.bin> --output_dir=/data/local/tmp/"
hdc -t "$SN" file recv /data/local/tmp/output_0 ./output.bin
```

不再使用 `com.example.naticvetestdemo`、HAP 安装、`aa start --ps omPath` 这条路径。

## Kirin9030 OMC 搜索状态

截至 2026-08-04，本仓没有找到可直接下载的 Kirin9030/Changsha/Q709030 预编译 `.omc`。

已经确认的公开信息：

```text
cann-recipes-harmony-infer 的 QuantMatmul/SliceGelu/GatherDequantInt8 是源码和编译样例，不是预编译 OMC bundle。
QuantMatmul 文档声明支持 Kirin X90 和 Kirin 9030，CMakePresets 里 ASCEND_COMPUTE_UNIT=kirin9030，op_host 里 AddConfig("kirin9030")。
cann-skills-zyh / cannbot-skills 里的 model-infer-harmony 是 Kirin9030 OMC 生成流程，示例命令是 omg --platform=kirin9030 --target=omc，但仓库没有提交生成后的 .omc。
GitHub code search 没有命中带 soc_version/hiai_version/Bisheng-Compiler/Kirin9030 的 CANN .omc。
GitHub/GitCode/Web 搜索没有找到 gelu_fp16.omc、gelu_fp32.omc、add_1.omc 的公开下载源。
```

所以现在不能把“源码支持 Kirin9030”当成“已有 Kirin9030 OMC artifact”。要直接真机跑 Kirin9030，目前最现实的来源是 issue #1 history 里那台真机/内网机器上的 known-good 文件：

```text
/data/model/gelu_fp16.omc
/data/model/gelu_fp16_input.bin
/data/model/gelu_fp32.omc
/data/model/gelu_fp32_input.bin
/data/model/add_1.omc
/data/model/add_x1.bin
/data/model/add_x2.bin
/data/model/add_fp16_x1.bin
/data/model/add_fp16_x2.bin
```

拿到其中任意一组 `.omc + input.bin` 后，先用 `scripts/package-naked-omc-bundle.sh` 做成标准 bundle，再用 `scripts/test-naked-omc-vetest.sh --bundle-dir ...` 跑。

## 当前根因判断

从最新真机日志可以确定：

```text
hdc 连接正常
/data/local/tmp/model_run_tool 存在且可执行
SobelCustom.omc 和 x.bin 已成功发送到 /data/local/tmp
model_run_tool 已解析到模型名 SobelCustom
失败发生在模型加载阶段：Load model SobelCustom failed. status:1
```

所以当前 blocker 不是 HAP、签名、`aa start`、runner 发现、文件传输或路径拼接。

仅凭 `status:1` 还不能证明唯一根因。最强假设是 `.omc` 与目标 SoC/runtime 不兼容：

```text
当前 prebuilt SobelCustom.omc 暴露的 SoC hint 是 kirin9020
目标 HarmonyOS PC 的真实 Kirin SoC 还没有被自动识别出来
即使目标是 kirin9020，也仍可能是 runtime/HiAI/NNCore 版本或 custom-op 模型加载兼容性问题
```

下一次失败时必须看 evidence 里的诊断文件，而不是只看终端上的 `status:1`。

## 当前已准备文件

注意：下面这个 Sobel release 是旧的 Kirin9020 预编译样例，不是 Kirin9030/Changsha 目标。它现在只能作为脚本和 runner 路径验证材料，不能再作为 Kirin9030 测试 artifact。

Release:

```text
https://github.com/RockmanZheng/kirin-ops/releases/tag/kirin-sobel-naked-omc-2026-08-03
```

Asset:

```text
kirin-sobel-naked-omc-2026-08-03.zip
sha256: ea37de6ecd7af19f1fe74d6c1913e6081726d53d3a1128f4544c4c0e972f873b
```

解压后目录内容：

```text
kirin-sobel-naked-omc-2026-08-03/
  README.txt
  SHA256SUMS
  SobelCustom.omc
  SobelCustom.om
  x.bin
  y.bin
```

文件含义：

```text
SobelCustom.omc  现成 Sobel CANN 模型
SobelCustom.om   同一 demo 携带的 companion model artifact，备用留存
x.bin            输入数据，uint8 NHWC，shape [1, 763, 1024, 3]
y.bin            golden 输出，uint8，shape [1, 1, 761, 1022] flatten 后二进制
```

模型和数据 SHA256：

```text
97dc80ad96c972c0c5ae47cd44d4be09a9af84cd02f7b9705ea6d5cce77e5768  SobelCustom.omc
757ccdc0d382b213714b0256278c688622d43da679659c97dd6b6bc43306d4e6  SobelCustom.om
9f2f4d02d225403d3f480b9e88bb7b2b362f1f8a1751c6e34041d38b0935f03b  x.bin
4f2655e865146d12cfc0513fca54040746c830f23120905079bdec24d3d0e8b1  y.bin
```

## 前置条件

目标机器：

```text
HarmonyOS PC / phone / tablet
Kirin NPU runtime 可用
Developer mode / HDC debugging 已打开
/data/local/tmp/model_run_tool 已存在且可执行
```

控制端：

```text
hdc 可用
能通过 hdc 看到目标设备
已 clone kirin-ops 仓库
```

检查 runner：

```bash
hdc list targets
export SN="<target-id>"

hdc -t "$SN" shell "ls -l /data/local/tmp/model_run_tool && (/data/local/tmp/model_run_tool --version 2>&1 || /data/local/tmp/model_run_tool --help 2>&1)"
```

这里不能只看文件是否存在。runner 必须能实际启动，并输出 version 或 usage。否则后面的 `.omc` 测试还没进入模型加载阶段。

检查芯片类型：

```bash
hdc -t "$SN" shell "param get ohos.boot.chiptype"
```

如果这条命令返回 `Kirin9020`，脚本会把设备侧 chiptype 作为 target SoC 来源。`--target-soc` 只作为设备参数缺失时的补充，或作为人工断言；如果人工断言和设备参数冲突，严格模式会直接失败。

## 在另一台 HarmonyOS PC 上执行

先拿代码和脚本：

```bash
git clone git@github.com-kirin-ops:RockmanZheng/kirin-ops.git
cd kirin-ops
git checkout main
```

如果不用 deploy key alias，也可以用任何能访问私有 repo 的 GitHub 认证方式。

下载 release bundle：

```bash
gh release download kirin-sobel-naked-omc-2026-08-03 \
  --repo RockmanZheng/kirin-ops \
  --pattern 'kirin-sobel-naked-omc-2026-08-03.zip'

shasum -a 256 kirin-sobel-naked-omc-2026-08-03.zip
unzip -o kirin-sobel-naked-omc-2026-08-03.zip
```

如果后续已经有 Kirin9030 bundle，例如 `kirin9030-gelu-fp16-2026-08-04.zip`，下载和解压方式相同：

```bash
gh release download kirin9030-gelu-fp16-2026-08-04 \
  --repo RockmanZheng/kirin-ops \
  --pattern 'kirin9030-gelu-fp16-2026-08-04.zip'

shasum -a 256 kirin9030-gelu-fp16-2026-08-04.zip
unzip -o kirin9030-gelu-fp16-2026-08-04.zip
```

确认设备：

```bash
hdc list targets
export SN="SH236HS0488"
```

执行 Sobel 裸 `.omc`：

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --bundle-dir "$PWD/kirin-sobel-naked-omc-2026-08-03" \
  --no-clear-logs
```

`--no-clear-logs` 是因为我们已经见过部分机器上 `hdc hilog -r` 可能卡住。
脚本现在默认使用 `hdc shell "hilog -r"` 清理缓存日志；OpenHarmony HDC 文档里的清日志示例也是这个形式。`--no-clear-logs` 仍然保留，方便绕过目标机上任何 hilog 行为差异。

如果设备能通过 `ohos.boot.chiptype` 报出 SoC，脚本会优先使用设备侧结果。`--target-soc` 可以作为额外断言；设备侧缺失时，它会作为 fallback：

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --target-soc kirin9020 \
  --bundle-dir "$PWD/kirin-sobel-naked-omc-2026-08-03" \
  --no-clear-logs
```

当前这个 Sobel bundle 的 `.omc` 是 `kirin9020`。如果设备侧 `ohos.boot.chiptype` 与人工传入的 `--target-soc` 冲突，或 target SoC 与模型 SoC 冲突，严格模式下脚本会 fail fast，不会继续发送文件和运行模型。

Kirin9030 bundle 跑法相同，只是 bundle 里会带 `bundle.env`：

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --bundle-dir "$PWD/kirin9030-gelu-fp16-2026-08-04" \
  --device-dir "/data/local/tmp/z84378291" \
  --model-run-tool "/data/local/tmp/z84378291/model_run_tool" \
  --no-clear-logs
```

如果 `bundle.env` 里写了 `TARGET_SOC=kirin9030`，脚本会把它作为 SoC 断言；如果设备侧 `param get ohos.boot.chiptype` 返回了其他 Kirin 版本，严格模式会 fail fast。

## 手工命令

脚本等价于下面这组命令：

```bash
hdc -t "$SN" shell "ls -l /data/local/tmp/model_run_tool"

hdc -t "$SN" file send \
  kirin-sobel-naked-omc-2026-08-03/SobelCustom.omc \
  /data/local/tmp/SobelCustom.omc

hdc -t "$SN" file send \
  kirin-sobel-naked-omc-2026-08-03/x.bin \
  /data/local/tmp/x.bin

hdc -t "$SN" shell "rm -f /data/local/tmp/output_0"

hdc -t "$SN" shell \
  "/data/local/tmp/model_run_tool --model=/data/local/tmp/SobelCustom.omc --input=/data/local/tmp/x.bin --output_dir=/data/local/tmp/"

hdc -t "$SN" shell "ls -lt /data/local/tmp/ | head -20"
hdc -t "$SN" file recv /data/local/tmp/output_0 ./output_sobel.bin

cmp output_sobel.bin kirin-sobel-naked-omc-2026-08-03/y.bin
```

## 多输入算子

issue #1 的 history 里出现过多输入写法：

```bash
--input=/data/local/tmp/add_x1.bin,/data/local/tmp/add_x2.bin
```

脚本支持本地多输入文件用逗号分隔：

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --omc /path/to/add_1.omc \
  --input /path/to/add_x1.bin,/path/to/add_x2.bin \
  --no-compare \
  --no-clear-logs
```

脚本会把每个输入文件发送到 `/data/local/tmp/`，并生成远端逗号分隔参数。

## 通用 OMC Bundle

不要再把脚本和 Sobel 的 `SobelCustom.omc/x.bin/y.bin` 绑死。新的标准 bundle 用 `bundle.env` 描述模型和输入：

```text
kirin9030-gelu-fp16-2026-08-04/
  README.txt
  SHA256SUMS
  bundle.env
  gelu_fp16.omc
  gelu_fp16_input.bin
```

`bundle.env` 示例：

```bash
NAME="kirin9030-gelu-fp16-2026-08-04"
DESCRIPTION="GELU FP16 prebuilt OMC for Kirin9030 naked model_run_tool test"
OMC="gelu_fp16.omc"
INPUT="gelu_fp16_input.bin"
OUTPUT_NAME="output_0"
TARGET_SOC="kirin9030"
COMPARE="0"
```

多输入示例：

```bash
NAME="kirin9030-add-fp16-2026-08-04"
OMC="add_1.omc"
INPUT="add_fp16_x1.bin,add_fp16_x2.bin"
OUTPUT_NAME="output_0"
TARGET_SOC="kirin9030"
COMPARE="0"
```

字段含义：

```text
OMC          bundle 内的 .omc 文件名
INPUT        一个输入文件，或逗号分隔的多个输入文件
GOLDEN       可选；存在时做 byte-for-byte compare
OUTPUT_NAME  model_run_tool 在 device_dir 下生成的输出文件名，默认 output_0
TARGET_SOC   目标芯片断言，例如 kirin9030
COMPARE      1 表示必须和 GOLDEN 比较；0 表示只确认输出能被拉回
```

打包命令：

```bash
scripts/package-naked-omc-bundle.sh \
  --name kirin9030-gelu-fp16-2026-08-04 \
  --description "GELU FP16 prebuilt OMC for Kirin9030 naked model_run_tool test" \
  --omc ./gelu_fp16.omc \
  --input ./gelu_fp16_input.bin \
  --target-soc kirin9030
```

生成：

```text
artifacts/naked-omc/kirin9030-gelu-fp16-2026-08-04/
artifacts/releases/kirin9030-gelu-fp16-2026-08-04.zip
artifacts/releases/kirin9030-gelu-fp16-2026-08-04.zip.sha256
```

上传 release：

```bash
TAG="kirin9030-gelu-fp16-2026-08-04"

gh release create "$TAG" \
  --repo RockmanZheng/kirin-ops \
  --title "Kirin9030 GELU FP16 Naked OMC Bundle 2026-08-04" \
  --notes "Generic naked OMC bundle for Kirin9030/Changsha model_run_tool testing." \
  artifacts/releases/kirin9030-gelu-fp16-2026-08-04.zip \
  artifacts/releases/kirin9030-gelu-fp16-2026-08-04.zip.sha256
```

如果 release 已经存在：

```bash
gh release upload "$TAG" \
  --repo RockmanZheng/kirin-ops \
  --clobber \
  artifacts/releases/kirin9030-gelu-fp16-2026-08-04.zip \
  artifacts/releases/kirin9030-gelu-fp16-2026-08-04.zip.sha256
```

## 从真机/内网机器提取 Known-Good OMC

这些命令要在能连到那台 HarmonyOS target 的机器上执行。`/data/model/...` 是 target 或内网测试机上的路径，不是本机 Mac 的路径。

GELU FP16：

```bash
export SN="SH236HS0488"
mkdir -p kirin9030-gelu-fp16-2026-08-04-source

hdc -t "$SN" file recv /data/model/gelu_fp16.omc \
  kirin9030-gelu-fp16-2026-08-04-source/gelu_fp16.omc

hdc -t "$SN" file recv /data/model/gelu_fp16_input.bin \
  kirin9030-gelu-fp16-2026-08-04-source/gelu_fp16_input.bin

strings -a kirin9030-gelu-fp16-2026-08-04-source/gelu_fp16.omc \
  | grep -Ei 'soc_version|kirin[0-9]+|kirinx[0-9]+|q709030|changsha|chs|platform|hiai_version' \
  | head -80
```

然后打包：

```bash
scripts/package-naked-omc-bundle.sh \
  --name kirin9030-gelu-fp16-2026-08-04 \
  --description "GELU FP16 known-good OMC extracted from HarmonyOS target /data/model for Kirin9030 naked runner test" \
  --omc kirin9030-gelu-fp16-2026-08-04-source/gelu_fp16.omc \
  --input kirin9030-gelu-fp16-2026-08-04-source/gelu_fp16_input.bin \
  --target-soc kirin9030
```

Add FP16 双输入：

```bash
export SN="SH236HS0488"
mkdir -p kirin9030-add-fp16-2026-08-04-source

hdc -t "$SN" file recv /data/model/add_1.omc \
  kirin9030-add-fp16-2026-08-04-source/add_1.omc

hdc -t "$SN" file recv /data/model/add_fp16_x1.bin \
  kirin9030-add-fp16-2026-08-04-source/add_fp16_x1.bin

hdc -t "$SN" file recv /data/model/add_fp16_x2.bin \
  kirin9030-add-fp16-2026-08-04-source/add_fp16_x2.bin

scripts/package-naked-omc-bundle.sh \
  --name kirin9030-add-fp16-2026-08-04 \
  --description "Add FP16 known-good OMC extracted from HarmonyOS target /data/model for Kirin9030 naked runner test" \
  --omc kirin9030-add-fp16-2026-08-04-source/add_1.omc \
  --input kirin9030-add-fp16-2026-08-04-source/add_fp16_x1.bin,kirin9030-add-fp16-2026-08-04-source/add_fp16_x2.bin \
  --target-soc kirin9030
```

## 脚本做了什么

脚本路径：

```text
scripts/test-naked-omc-vetest.sh
```

执行步骤：

```text
1. 检查 hdc 和 target。
2. 记录 hdc binary、版本、checkserver、list targets -v。
3. 从本地 .omc 提取 embedded SoC metadata。
4. 从目标机器 param/uname/param ls/uname/process/runtime lib 扫描收集设备信息。
5. 从 model_run_tool hash、file/readelf、--version、--help 和 strings 收集 runner 信息。
6. 如果能确认 model SoC 和 target SoC 不匹配，严格模式下 fail fast。
7. 实际启动 /data/local/tmp/model_run_tool 做 preflight；如果 runner 不存在、不可访问、架构不匹配或动态链接失败，直接 fail fast。
8. 创建/确认 /data/local/tmp。
9. 发送 .omc 和一个或多个输入 .bin。
10. 记录运行前远端模型/输入/输出文件状态和 hash。
11. 删除旧的 /data/local/tmp/output_0。
12. 执行 model_run_tool。
13. 如果模型加载失败，继续收集运行后文件状态、hilog 和 summary，然后再退出。
14. 拉回 /data/local/tmp/output_0。
15. 如果 y.bin 存在，用 cmp 做 byte-for-byte compare。
16. 写 evidence summary。
17. 自动写可复制粘贴的 `evidence-report.txt`。
18. 自动写 evidence-files.txt，打包 `<evidence-dir>.tgz`，并生成 `<evidence-dir>.tgz.sha256`。
```

Evidence 默认目录：

```text
artifacts/naked-omc-runs/<timestamp>/
```

关键文件：

```text
summary.txt
hdc-info.txt
target-info.txt
target-diagnostics.log
runner-diagnostics.log
model-strings-info.txt
soc-preflight.log
model-run-tool-check.log
send-omc.log
send-input.log
model-run-tool.log
remote-files-before-run.log
remote-list-after-run.log
remote-files-after-run.log
pull-output.log
compare.log
hilog.raw.log
hilog.filtered.log
output_0
evidence-report.txt
evidence-files.txt
../<timestamp>.tgz
../<timestamp>.tgz.sha256
```

如果内网机器不能上传附件，直接复制这个单文件：

```bash
cat artifacts/naked-omc-runs/<timestamp>/evidence-report.txt
```

这个 report 会把关键文本日志按 section 拼在一起。`output_0` 这类二进制输出不会内联，只通过 hash、大小和远端文件列表记录。

默认 archive 路径是：

```text
<evidence-dir>.tgz
```

脚本会把 archive 和 sha256 路径打印到终端，也会写入 `summary.txt`：

```text
evidence_archive=...
evidence_manifest=...
evidence_text=...
```

默认不把 `hilog.raw.log` 放进 archive，除非 `hilog.filtered.log` 为空。这样避免正常情况下打出很大的压缩包。如果诊断需要完整 raw hilog，可以显式打开：

```bash
scripts/test-naked-omc-vetest.sh ... --include-raw-hilog
```

如果你只想在本机保留散落日志，不想生成 `.tgz`：

```bash
scripts/test-naked-omc-vetest.sh ... --no-export-logs
```

如果只想关掉 copy/paste text report，但仍保留 `.tgz`：

```bash
scripts/test-naked-omc-vetest.sh ... --no-export-text
```

## 成功标准

最低可接受：

```text
model_run_tool 返回成功
/data/local/tmp/output_0 被拉回
summary.txt 里 result=PASS_OUTPUT_PULLED_NO_COMPARE
```

更强的成功标准：

```text
output_0 与 y.bin 完全一致
summary.txt 里 result=PASS_CANDIDATE
```

如果没有 golden，`PASS_OUTPUT_PULLED_NO_COMPARE` 说明裸 `.omc` CLI runner 路径已经跑通并产生输出。如果 compare 通过，`PASS_CANDIDATE` 是更强确认。后续再结合 hilog/NPU runtime marker 判断是否确实走了 NPU。

## 常见问题

`model_run_tool` 不存在或不可执行：

```text
model_run_tool not found or not executable
```

处理方式：先从同事机器/内部环境拿到该工具，并放到目标设备 `/data/local/tmp/model_run_tool`，确保有执行权限。

`model_run_tool` 路径看似存在，但实际执行失败：

```text
/bin/sh: /data/local/tmp/model_run_tool: inaccessible or not found
```

含义：

```text
这不是 SobelCustom.omc 的平台兼容性错误。
runner 二进制还没有成功启动，测试没有进入模型加载阶段。
```

常见原因：

```text
/data/local/tmp/model_run_tool 在这台 target 上不存在或不可访问
model_run_tool 是为另一种 ABI/系统镜像构建的
ELF interpreter / 动态库依赖在这台 target 上不存在
/data/local/tmp 的执行策略或权限和另一台机器不同
```

最小诊断命令：

```bash
export SN="SH25BHS4036"

hdc -t "$SN" shell "uname -a; id; ls -l /data/local/tmp/model_run_tool; chmod 755 /data/local/tmp/model_run_tool 2>/dev/null || true; /data/local/tmp/model_run_tool --version 2>&1 || /data/local/tmp/model_run_tool --help 2>&1; echo runner_rc=\$?"
```

如果目标机没有 `file/readelf`，把 runner 拉回控制端检查：

```bash
hdc -t "$SN" file recv /data/local/tmp/model_run_tool /tmp/model_run_tool.$SN
sha256sum /tmp/model_run_tool.$SN
file /tmp/model_run_tool.$SN
readelf -l /tmp/model_run_tool.$SN | grep -i interpreter || true
```

如果另一台 target 上 runner 能正常启动，可以对比两个 runner 是否同一个文件：

```bash
for SN in SH236HS0488 SH25BHS4036; do
  hdc -t "$SN" file recv /data/local/tmp/model_run_tool /tmp/model_run_tool.$SN || true
done

sha256sum /tmp/model_run_tool.SH236HS0488 /tmp/model_run_tool.SH25BHS4036 2>/dev/null || true
file /tmp/model_run_tool.SH236HS0488 /tmp/model_run_tool.SH25BHS4036 2>/dev/null || true
```

如果 `SH236HS0488` 上的 runner 已知能启动，而 `SH25BHS4036` 缺 runner，可以先复制到自己的目标目录，不覆盖公共 `/data/local/tmp/model_run_tool`：

```bash
SOURCE_SN="SH236HS0488"
TARGET_SN="SH25BHS4036"
REMOTE_DIR="/data/local/tmp/z84378291"

scripts/bootstrap-model-run-tool.sh \
  --source-target "$SOURCE_SN" \
  --dest-target "$TARGET_SN" \
  --dest-path "$REMOTE_DIR/model_run_tool"
```

然后用自己的 runner 路径跑 Sobel：

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$TARGET_SN" \
  --bundle-dir "$PWD/kirin-sobel-naked-omc-2026-08-03" \
  --device-dir "$REMOTE_DIR" \
  --model-run-tool "$REMOTE_DIR/model_run_tool" \
  --target-soc Kirin9020 \
  --no-clear-logs
```

脚本内部展开后等价于：

```bash
LOCAL_RUNNER="/tmp/model_run_tool.${SOURCE_SN}"

hdc -t "$SOURCE_SN" shell "ls -l /data/local/tmp/model_run_tool; /data/local/tmp/model_run_tool --version 2>&1 || /data/local/tmp/model_run_tool --help 2>&1"
hdc -t "$SOURCE_SN" file recv /data/local/tmp/model_run_tool "$LOCAL_RUNNER"

hdc -t "$TARGET_SN" shell "mkdir -p $REMOTE_DIR"
hdc -t "$TARGET_SN" file send "$LOCAL_RUNNER" "$REMOTE_DIR/model_run_tool"
hdc -t "$TARGET_SN" shell "chmod 755 $REMOTE_DIR/model_run_tool"
hdc -t "$TARGET_SN" shell "$REMOTE_DIR/model_run_tool --version 2>&1 || $REMOTE_DIR/model_run_tool --help 2>&1"
```

找不到设备：

```text
no hdc targets found
```

处理方式：检查 HarmonyOS PC 的 HDC debugging、USB/IP 连接和 `hdc list targets`。

`hilog -r` 卡住：

```bash
scripts/test-naked-omc-vetest.sh ... --no-clear-logs
```

背景：HDC 官方命令里 `hdc hilog` 用于抓日志，清日志示例是 `hdc shell "hilog -r"`。脚本已经改成 shell 形式，并且有 timeout；如果目标机仍然卡住，就继续加 `--no-clear-logs`。

没有 `output_0`：

```text
output was not pulled successfully
```

优先检查：

```text
model-run-tool.log
remote-list-after-run.log
hilog.filtered.log
输入文件名和 model_run_tool 参数是否匹配
```

模型加载失败：

```text
[INFO] Get modelname:SobelCustom
[ERROR] Load model SobelCustom failed. status:1.
[ERROR] Inference: loading model SobelCustom failed.
[ERROR] [ModelManagerV1]Model Process ret failed.
```

含义：

```text
hdc 连通
model_run_tool 存在且可执行
.omc 和输入文件已经传到 /data/local/tmp
失败点已经进入模型加载阶段
```

本地检查这个 prebuilt Sobel 模型：

```bash
strings -a kirin-sobel-naked-omc-2026-08-03/SobelCustom.omc | grep -E 'soc_version|kirin[0-9]+|Kirin[0-9]+'
```

当前 bundle 里的 `SobelCustom.omc` 暴露出的 SoC hint 是：

```text
soc_version
kirin9020
```

脚本默认会做 SoC preflight：

```text
model-strings-info.txt  记录从 .omc 里提取到的 normalized_soc_versions
target-info.txt         记录目标机器 param/uname
target-diagnostics.log  记录 param ls、process scan、runtime libs 等扩展诊断
soc-preflight.log       记录 model SoC、target SoC、检查结果
```

检查规则：

```text
model SoC 和 target SoC 都已知且匹配: PASS
model SoC 和 target SoC 都已知但不匹配: 严格模式 fail fast
model SoC 未知: WARN 后继续
model SoC 已知但 target SoC 自动检测不到: 严格模式 fail fast，要求传 --target-soc
```

常用参数：

```bash
--target-soc kirin9020   # 设备参数读不到 SoC 时的人工补充；设备能读到 ohos.boot.chiptype 时以设备侧为准
--skip-soc-check         # 临时跳过 SoC preflight
--no-strict              # target SoC 未知或 mismatch 时只报警，继续跑，方便做反证实验
```

如果目标 HarmonyOS PC 的 Kirin/NPU runtime 不是这个 target，或者当前 runtime 不接受这个 codelab 预编译模型，就会在 load 阶段失败。下一步要区分 runner 问题和模型兼容问题：

```bash
hdc -t "$SN" shell "param get const.product.model"
hdc -t "$SN" shell "param get const.product.name"
hdc -t "$SN" shell "param get const.product.software.version"
hdc -t "$SN" shell "param get const.ohos.apiversion"
hdc -t "$SN" shell "param get ohos.boot.chiptype"
hdc -t "$SN" shell "param get const.product.soc"
hdc -t "$SN" shell "param get const.soc_version"
hdc -t "$SN" shell "uname -a"
```

同时用同一台机器 history 里已知能跑的 `gelu_fp16.omc` 或 `add_1.omc` 再跑一遍。如果 gelu/add 还能跑，而 Sobel load 失败，则 runner 路线成立，问题是这个 Sobel prebuilt `.omc` 不是目标机可加载的模型。需要拿目标 SoC 对应的 Sobel `.omc`，或者在正确 `--soc_version` 的 CANN/mobile-station 环境重新生成。

如果这些 known-good 模型只用于确认 runner/runtime 是否健康，而目标 SoC 仍然无法自动识别，可以临时关闭 SoC preflight：

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --omc /data/model/gelu_fp16.omc \
  --input /data/model/gelu_fp16_input.bin \
  --skip-soc-check \
  --no-compare \
  --no-clear-logs
```

需要把这次运行的 evidence 目录贴回 issue，至少包括：

```text
summary.txt
soc-preflight.log
target-diagnostics.log
runner-diagnostics.log
model-run-tool.log
remote-files-after-run.log
hilog.filtered.log
```

现在脚本默认已经把这些文件拼进 `evidence-report.txt`，也打进 `<evidence-dir>.tgz`。不能上传文件时，复制 `evidence-report.txt`；能上传文件时，上传 `.tgz` 和旁边的 `.sha256`。

Compare 失败：

```text
golden compare failed
```

常见原因：

```text
模型和输入不匹配
目标 SoC/runtime 和这个 prebuilt SobelCustom.omc 不兼容
model_run_tool 的输出 layout 与 y.bin 不一致
```

## 当前 blocker

我们已经有自动化脚本，且真机已经跑到 `model_run_tool` 的模型加载阶段。旧 Sobel bundle 的失败已经被定位为内部 platform/form compatibility，不是 HAP/签名/`aa start`/runner 路径问题。

当前 blocker 是：

```text
拿到一个真实 Kirin9030/Changsha/Q709030 可加载的预编译 OMC bundle
bundle 必须包含对应 input .bin；有 golden 更好，没有也可以先 smoke
公开源码目前只证明可生成 Kirin9030 OMC，没找到可下载的预编译 OMC
最优先尝试从真机/内网机器 /data/model 提取 gelu_fp16/add_1 等 known-good OMC
```

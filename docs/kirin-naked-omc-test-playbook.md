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

如果已经知道目标 SoC，建议显式传入，脚本会在传文件和运行前做确定性 preflight：

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --target-soc kirin9020 \
  --bundle-dir "$PWD/kirin-sobel-naked-omc-2026-08-03" \
  --no-clear-logs
```

当前这个 Sobel bundle 的 `.omc` 是 `kirin9020`。如果你传 `--target-soc kirin9030` 或 `--target-soc KirinX90`，严格模式下脚本会 fail fast，不会继续发送文件和运行模型。

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
```

更强的成功标准：

```text
output_0 与 y.bin 完全一致
summary.txt 里 result=PASS_CANDIDATE
```

如果 compare 通过，可以认为这条裸 `.omc` CLI runner 路径已经基本打通。后续再结合 hilog/NPU runtime marker 判断是否确实走了 NPU。

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

我们已经有 Sobel `.omc`、输入、golden 和自动化脚本，且真机已经跑到 `model_run_tool` 的模型加载阶段。

当前 blocker 是：

```text
确认目标 HarmonyOS PC 的真实 Kirin SoC/runtime
确认 history 里的 gelu/add .omc 在同一台设备上仍可加载运行
拿到或重新生成和该目标 SoC 匹配的 SobelCustom.omc
```

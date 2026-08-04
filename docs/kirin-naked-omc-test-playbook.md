# Kirin Naked OMC Real-Device Test Playbook

这份 playbook 是给“先不重新打包 Soble HAP，直接测试裸 `.omc` 文件”的真机 spike 用的。

## 结论

裸 `.omc` 不是一个可以自己执行的可执行文件。它仍然需要一个 HarmonyOS 端 runner 应用来调用 CANN/NN runtime。

这里采用仓库文档里出现过的 native vetest runner 约定：

```text
bundleName: com.example.naticvetestdemo
ability: EntryAbility
device dir: /mnt/hmdfs/100/account/device_view/local/files/Docs/Download/com.example.naticvetestdemo
```

测试方式是：

1. 一次性安装 runner HAP。
2. 通过 `hdc file send` 把 `.omc` 和输入 `.bin` 传到 runner 文档目录。
3. 通过 `aa start ... --ps path ... --ps omPath ...` 启动 runner。
4. 从设备拉回 `output0.bin`。
5. 和 golden `y.bin` 比较。

这条路径不需要把每个 `.omc` 都打包进 Soble app，也不需要给每个 `.omc` 重新做 HAP 签名。

## 当前已准备文件

Release:

```text
https://github.com/RockmanZheng/kirin-ops/releases/tag/kirin-sobel-naked-omc-2026-08-03
```

Asset:

```text
kirin-sobel-naked-omc-2026-08-03.zip
sha256: 7fd2e93320eb78cca820ed1a599cead439bca59675552a7a4dfc6860316118ba
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
SobelCustom.omc  现成 Sobel CANN 模型，裸测主文件
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
```

控制端：

```text
hdc 可用
能通过 hdc 看到目标设备
已 clone kirin-ops 仓库
```

Runner：

```text
com.example.naticvetestdemo 已安装
```

如果 runner 还没装，需要先从同事或 CANN Kit 维测样例拿到签好的 runner HAP。这个仓库目前没有 `naticvetestdemo` HAP 或源码。

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
```

如果只看到一个 target，可以不传 `--target`。如果有多个，指定 target：

```bash
export SN="SH236HS0488"
```

如果 runner 已经安装：

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --bundle-dir "$PWD/kirin-sobel-naked-omc-2026-08-03" \
  --no-clear-logs
```

如果你手里有签好的 runner HAP，但还没安装：

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --runner-hap /absolute/path/to/entry-default-signed.hap \
  --bundle-dir "$PWD/kirin-sobel-naked-omc-2026-08-03" \
  --no-clear-logs
```

`--no-clear-logs` 是因为我们已经见过部分机器上 `hdc hilog -r` 可能卡住。脚本默认也有 5 秒 timeout，但真机 spike 时建议先跳过清日志。

## 脚本做了什么

脚本路径：

```text
scripts/test-naked-omc-vetest.sh
```

执行步骤：

```text
1. 检查 hdc 和 target。
2. 可选安装 --runner-hap。
3. 检查 com.example.naticvetestdemo 是否已安装。
4. 创建/确认设备目录。
5. 发送 SobelCustom.omc 和 x.bin。
6. 启动 runner:
   aa start -a EntryAbility -b com.example.naticvetestdemo --ps path x.bin --ps omPath SobelCustom.omc
7. 捕获 hilog。
8. 拉回 output0.bin。
9. 如果 y.bin 存在，用 cmp 做 byte-for-byte compare。
10. 写 evidence summary。
```

Evidence 默认目录：

```text
artifacts/naked-omc-runs/<timestamp>/
```

关键文件：

```text
summary.txt
target-info.txt
aa-start.log
pull-output.log
compare.log
hilog.raw.log
hilog.filtered.log
output0.bin
```

## 成功标准

最低可接受：

```text
aa start 成功
output0.bin 被拉回
```

更强的成功标准：

```text
output0.bin 与 y.bin 完全一致
summary.txt 里 result=PASS_CANDIDATE
```

如果 compare 通过，可以认为这条裸 `.omc` runner 路径已经基本打通。后续再结合 hilog/NPU runtime marker 判断是否确实走了 NPU。

## 常见问题

Runner 未安装：

```text
runner bundle not found: com.example.naticvetestdemo
```

处理方式：拿到 signed runner HAP 后重跑并加 `--runner-hap /path/to/hap`。

找不到设备：

```text
no hdc targets found
```

处理方式：检查 HarmonyOS PC 的 HDC debugging、USB/IP 连接和 `hdc list targets`。

`hilog -r` 卡住：

```bash
scripts/test-naked-omc-vetest.sh ... --no-clear-logs
```

没有 `output0.bin`：

```text
output was not pulled successfully
```

优先检查：

```text
aa-start.log
hilog.filtered.log
设备目录是否和 runner 代码一致
runner 是否真的按 output0.bin 写结果
```

Compare 失败：

```text
golden compare failed
```

常见原因：

```text
模型和输入不匹配
runner 的输入解析与 x.bin 不一致
目标 SoC/runtime 和这个 prebuilt SobelCustom.omc 不兼容
runner 实际输出 layout 与 y.bin 不一致
```

## 当前 blocker

我们已经有裸测所需的 Sobel `.omc`、输入、golden 和自动化脚本。

还缺的唯一关键文件是：

```text
签好的 native vetest runner HAP，bundleName 应为 com.example.naticvetestdemo
```

如果另一台 HarmonyOS PC 上已经装了这个 runner，就可以直接跑脚本，不需要 HAP 签名流程。

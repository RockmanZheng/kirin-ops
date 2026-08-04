# npu-group-3 Kirin/CANN Docker Environment

Status: created
Owner/work ID: `z84378291`
Remote host: `npu-group-3`

## Purpose

Provide an isolated Docker workspace for Kirin/CANN operator build exploration without touching other users' containers, images, or workspaces.

## Created Resources

- Workspace root: `/data1/z84378291`
- Project workspace: `/data1/z84378291/kirin-ops`
- Artifact directory: `/data1/z84378291/artifacts`
- Cache directory: `/data1/z84378291/cache`
- Docker network: `z84378291-kirin-cann-net`
- Docker container: `z84378291-kirin-cann91`
- Base image: `cann-9.1.0:v2`
- x86 mobile-station container: `z84378291-kirin-mobile-x86`
- x86 mobile-station CANN: `/opt/Ascend/cann-9.0.0`

## Container Shape

The container follows the machine's existing CANN convention:

- `--runtime ascend`
- `--privileged`
- `--ipc=shareable`
- `--shm-size=80g`
- all `/dev/davinci*` devices mounted
- host Ascend driver/add-ons/log paths mounted
- only the user workspace mounted as persistent data: `/data1/z84378291:/data1/z84378291`

Workdir inside the container:

```text
/data1/z84378291/kirin-ops
```

## Toolchain Validation

Validated inside `z84378291-kirin-cann91`:

- `npu-smi info` sees 8 Ascend 910B2 NPUs.
- CANN env: `/usr/local/Ascend/cann-9.1.0`
- `atc`: `/usr/local/Ascend/cann-9.1.0/bin/atc`
- `ccec`: `/usr/local/Ascend/cann-9.1.0/bin/ccec`
- `cmake`: `/usr/local/python3.11.14/bin/cmake`, version 4.3.0
- base Python: `/usr/local/python3.11.14/bin/python3`

The synced source repo is clean remotely:

```text
/data1/z84378291/kirin-ops/cann-recipes-harmony-infer
## master...origin/master
```

## Python Environment

Base container Python was left clean for CANN tooling:

- base `protobuf`: `3.20.3`
- base `onnx`: not installed
- base `cv2`: not installed

Repo ONNX/data-generation dependencies are installed in a dedicated venv:

```text
/data1/z84378291/cache/py311-cann-harmony
```

Validated venv packages:

- `numpy 2.4.6`
- `onnx 1.22.0`
- `opencv-python-headless 5.0.0`
- `protobuf 7.35.1`

Use it inside the container:

```bash
source /data1/z84378291/cache/py311-cann-harmony/bin/activate
```

`ops/ascendc/src/sobel_custom/test/create_onnx.py` was smoke-tested successfully with this venv.

## Local Helpers

From local workspace `/Users/zhenglewen/projects/kirin-ops`:

```bash
scripts/remote-cann-shell.sh
scripts/remote-cann-exec.sh 'pwd && command -v atc && command -v ccec'
scripts/remote-mobile-station-shell.sh
scripts/remote-mobile-station-exec.sh 'uname -m && command -v atc && command -v bisheng'
```

## Caveat

This is a CANN 9.1 server/NPU container on a 910B machine. It is the correct isolated build convention for this host, but it does not yet prove Kirin/mobile-station `.omc` compatibility. The next technical check is whether this toolchain accepts the repo's Kirin targets such as `KirinX90` / `kirinx90`.

## Mobile-Station Result

The x86 mobile-station path is now the known-good Kirin9030 compile/conversion
host path for SobelCustom:

```text
container: z84378291-kirin-mobile-x86
platform: linux/amd64
CANN: /opt/Ascend/cann-9.0.0
ATC: /opt/Ascend/cann-9.0.0/bin/atc
BiSheng: /opt/Ascend/cann-9.0.0/bin/bisheng
custom OPP path: /opt/Ascend/cann-9.0.0/opp/vendors/customize
```

The actual HarmonyOS phone target remains `aarch64`; the x86 container is only
the model-build host.

The first successful artifact was:

```text
/data1/z84378291/artifacts/kirin9030_sobel_mobile_x86_hardened_20260804_092334/model_conversion/SobelCustom_kirin9030.omc
```

See `docs/kirin9030-sobel-mobile-station-build-plan.md` for the repeatable
scripted flow and evidence.

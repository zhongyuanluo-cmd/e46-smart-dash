# Dev Log — Qt 6 Cross-Compilation

> 日期：待填
> Qt version: 6.8.2
> Toolchain: Linaro aarch64-linux-gnu

## 1. Toolchain

- Source: 
- Version: 
- Install path: `C:\toolchains\aarch64-linux-gnu\`
- Test compile: `aarch64-linux-gnu-gcc --version` → 

## 2. Core3566 Runtime Dependencies

```
apt install libfontconfig1-dev libfreetype6-dev libxkbcommon-dev \
    libgbm-dev libdrm-dev libegl1-mesa-dev libgles2-mesa-dev
```

Output: [success / failed]

## 3. Qt Configure

```
cmake ../qt-everywhere-src-6.8.2 \
  -DCMAKE_TOOLCHAIN_FILE=... \
  -DQT_HOST_PATH=... \
  -DFEATURE_opengles2=ON \
  -DFEATURE_eglfs=ON \
  ...
```

- Configure output: [success / failed]
- Build output: [success / failed]
- Build duration: 

## 4. Deployment

- scp target path: `/usr/local/qt6/`
- File count: 
- Total size: 

## 5. Hello World Test

```
$ ./qt-hello -platform eglfs
[TBD]
```

| Check | Expected | Actual | Notes |
|------|:---:|:---:|------|
| Red rectangle visible | yes | ❓ | |
| "E46 Smart Dash" text | yes | ❓ | |
| EGL vendor: Panfrost | yes | ❓ | |
| GL renderer: Mali-G52 | yes | ❓ | |
| Exit code 0 | yes | ❓ | |

## 6. Issues & Workarounds

1. [No issues yet]

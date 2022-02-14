## 编译BisonView


### 先决条件

#### 系统要求
- x86_64 Linux系统,至少8GB以上内存
- 100GB磁盘空间
- 必须先安装git和Python(V3.6+)

我们是在Ubuntu18.04上开发及编译，不支持在Windows和Mac上编译。

#### 配置代理
BisonView是基于Google的chromium,拉取代码需要翻墙。
```
export http_proxy=${ip}:${port}
export http_proxys=${ip}:${port}

```
配置`.boto`文件
```
[Boto]
proxy=${ip}
proxy_port=${port}
```

### 安装depot_tools
```
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
```
将`depot_tools`加入环境变量。
```
export PATH=$PATH:$(pwd)/depot_tools
```


### 获取BisonView代码
```shell
mkdir bisonview && cd bisonview

gclient config --name "src/bison" --unmanaged git@github.com:ShimoFour/BisonView.git

echo target_os = [\"android\"] >> .gclient

gclient sync
```

### 安装编译工具
```
cd src
build/install-build-deps-android.sh
```


### 将buildtools加入环境变量
```
export CHROMIUM_BUILDTOOLS_PATH=$(pwd)/buildtools
```

### 编译

```
gn gen out/debug --args='target_os="android" target_cpu="x86" is_debug=true is_component_build=false'

autoninja -C out/debug bison_view_aar

```

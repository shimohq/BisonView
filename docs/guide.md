
## 接入指南

### 添加依赖
首先，build.gradle中添加BisonView 依赖
```
dependencies {
    implementation 'im.shimo:bisonview:xxx'
}

```

### 排除BisonVie中资源文件
排除Assets中的`.dat`,`.bin`,`.pak`格式文件，不压缩
```
android {
    ...
    ...

    aaptOptions {
          noCompress ".dat", ".bin" , ".pak" // 表示不让aapt压缩的文件后缀
    }
    ...
}

```

### 区分进程加载app模块
BisonView在子进程中渲染网页,渲染进程是一个[isolatedProcess](https://developer.android.com/guide/topics/manifest/service-element),所以需要在Application中区分进程初始化模块:
```
if (BisonInitializer.isBrowserProcess()){
    // 初始化app需要的模块 例如:ARouter...
}

```


### 筛选cpu架构
BisonView.aar 中包含 `armeabi-v7a`,`x86`,`arm64-v8a`,`x86_64`4个架构的动态链接库。
为了减小apk包体积，你可以abiFilters 排除不需要的架构动态链接库:
```
android {
    ...
    ...

    buildTypes {
      ...
        release {
            ndk {
                abiFilters "armeabi-v7a", "arm64-v8a"
            }
        }
    }
    ...
}

```

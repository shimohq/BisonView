
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

### 筛选cpu架构
BisonView.aar 中包含 `armeabi-v7a`,`x86`,`arm64-v8a`,`x86_64`4个架构的动态连结库。
为了减小apk包体积，你可以

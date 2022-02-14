## BisonView - Android WebView






### 用法
添加依赖
```
dependencies {
    implementation 'im.shimo:bisonview:xxx'
}
```

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
在layout xml 中添加
```
...

<im.shimo.bison.BisonView
        android:id="@+id/bison_view"
        android:layout_width="match_parent"
        android:layout_height="match_parent" />

...
```
然后
```
BisonView bisonView = findViewById(R.id.bison_view);
bisonView.loadUrl("https://xxx.xx.xx");
```
BisonView的使用方式大部分和系统的WebView一样，更多信息请查看[接入指南](./docs/guide.md)或[API差异](./docs/ApiDiff.md)。
如果您需要自己编译BisonView请查看[编译BisonView](./docs/build.md)

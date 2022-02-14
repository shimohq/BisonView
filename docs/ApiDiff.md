## BisonView API 差异

以下列出个系统WebView不一样的地方


### BisonView


#### evaluateJavascript

#### loadUrl
加载一个Url , 支持 `https://`, `content://`, `file:///android_asset`和`file:///android_res`。
不支持`javascript://`


#### setBisonWebChromeClient
功能对应系统WebView#setWebChromeClient。

#### setBisonViewClient
功能对应系统WebView#setWebViewClient。

#### getBisonViewRenderProcess
功能对应系统WebView#getWebViewRenderProcess,android5以上全支持，不需要版本判断。

#### setBisonViewRenderProcessClient
功能对应系统WebView#setWebViewRenderProcessClient,android5以上全支持，不需要版本判断。

#### getBisonViewRenderProcessClient
功能对应系统WebView#getWebViewRenderProcessClient,android5以上全支持，不需要版本判断。

#### setRemoteDebuggingEnabled
功能对应系统WebView#setWebContentsDebuggingEnabled。





### BisonViewClient

#### overrideRequest
BisonView特有的API, 用于覆盖Web请求的RequestHeader。
用法：
```
bisonView.setBisonViewClient(new BisonViewClient(){

    public void overrideRequest(BisonView view, WebResourceRequest request) {
        request.getRequestHeaders().put("youKey","youValue");
    }
});

```
### BisonViewSettings
功能对应系统 WebSettings

### BisonViewWebStorage
功能对应系统 WebStorage


其他请参考[系统WebView](https://developer.android.com/reference/android/webkit/WebView)

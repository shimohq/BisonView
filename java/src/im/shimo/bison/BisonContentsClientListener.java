package im.shimo.bison;

interface BisonContentsClientListener {

    void onJsAlert(String url,String message , JsResult result);

    void onJsConfirm(String url, String message, JsResult result);
    
}

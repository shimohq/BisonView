package im.shimo.bison;


interface BisonContentsClientListener {

    void onJsAlert(String url, String message, JsResult result);

    void onJsConfirm(String url, String message, JsResult result);

    void onJsPrompt(String url, String message, String defaultValue, JsPromptResult result);
}

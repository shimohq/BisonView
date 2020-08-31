package im.shimo.bison;


interface JsPromptResultReceiver {
    void confirm(String result);
    void cancel();
}

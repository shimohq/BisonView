package im.shimo.bison;


interface JsPromptResultReceiver {
    public void confirm(String result);
    public void cancel();
}

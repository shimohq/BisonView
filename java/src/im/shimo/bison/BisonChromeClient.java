package im.shimo.bison;

public class BisonChromeClient {

    public void onProgressChanged(BisonView view, int newProgress) {
    }


    public void onReceivedTitle(BisonView view, String title) {
    }

    public boolean onJsAlert(BisonView view, String url, String message,
            JsResult result) {
        return false;
    }

    public boolean onJsConfirm(BisonView view, String url, String message,
            JsResult result) {
        return false;
    }

    public boolean onJsPrompt(BisonView view, String url, String message,
                              String defaultValue, JsPromptResult result) {
        return false;
    }

}

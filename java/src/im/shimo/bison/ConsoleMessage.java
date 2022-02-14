package im.shimo.bison;


public class ConsoleMessage {

    public enum MessageLevel {
        TIP, LOG, WARNING, ERROR, DEBUG
    };


    private MessageLevel mLevel;

    private String mMessage;

    private String mSourceId;

    private int mLineNumber;

    public ConsoleMessage(String message, String sourceId, int lineNumber, MessageLevel msgLevel) {
        mMessage = message;
        mSourceId = sourceId;
        mLineNumber = lineNumber;
        mLevel = msgLevel;
    }

    public MessageLevel messageLevel() {
        return mLevel;
    }

    public String message() {
        return mMessage;
    }

    public String sourceId() {
        return mSourceId;
    }

    public int lineNumber() {
        return mLineNumber;
    }
}

package im.shimo.bison;

/**
 * Abstract base class for objects that are expected to be isomorphic
 * (i.e. have a lazy 1:1 mapping for its entire lifetime) with a support
 * library object.
 */
abstract class BisonSupportLibIsomorphic {
    private Object mSupportLibObject;

    public Object getSupportLibObject() {
        return mSupportLibObject;
    }

    public void setSupportLibObject(Object supportLibObject) {
        assert mSupportLibObject == null;
        mSupportLibObject = supportLibObject;
    }
}
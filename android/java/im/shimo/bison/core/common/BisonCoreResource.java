package im.shimo.bison.core.common;

import android.content.Context;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;

public class BisonCoreResource {

    private static final String TAG = "BisonCoreResource";


    private static boolean loaded;

    private final static String INTERNAL_RESOURCE_CLASSES[] = {
        "org.chromium.components.web_contents_delegate_android.R",
        "org.chromium.content.R",
        "org.chromium.ui.R"
    };

    private final static String GENERATED_RESOURCE_CLASS = "im.shimo.bison.core.R";




    private static void doResetIds(Context context) {
        // internal classes are loaded with the same classLoader of XWalkInternalResources
        ClassLoader classLoader = BisonCoreResource.class.getClassLoader();
        ClassLoader appClassLoader = context.getApplicationContext().getClassLoader();
        for (String resourceClass : INTERNAL_RESOURCE_CLASSES) {
            try {
                Class<?> internalResource = classLoader.loadClass(resourceClass);
                Class<?>[] innerClazzs = internalResource.getClasses();
                for (Class<?> innerClazz : innerClazzs) {
                    Class<?> generatedInnerClazz;
                    String generatedInnerClassName = innerClazz.getName().replace(
                            resourceClass, GENERATED_RESOURCE_CLASS);
                    try {
                        generatedInnerClazz = appClassLoader.loadClass(generatedInnerClassName);
                    } catch (ClassNotFoundException e) {
                        Log.w(TAG, generatedInnerClassName + "is not found.");
                        continue;
                    }
                    Field[] fields = innerClazz.getFields();
                    for (Field field : fields) {
                        // It's final means we are probably not used as library project.
                        if (Modifier.isFinal(field.getModifiers())) field.setAccessible(true);
                        try {
                            int value = generatedInnerClazz.getField(field.getName()).getInt(null);
                            field.setInt(null, value);
                        } catch (IllegalAccessException e) {
                            Log.w(TAG, generatedInnerClazz.getName() + "." +
                                    field.getName() + " is not accessable.");
                        } catch (IllegalArgumentException e) {
                            Log.w(TAG, generatedInnerClazz.getName() + "." +
                                    field.getName() + " is not int.");
                        } catch (NoSuchFieldException e) {
                            Log.w(TAG, generatedInnerClazz.getName() + "." +
                                    field.getName() + " is not found.");
                        }
                        if (Modifier.isFinal(field.getModifiers())) field.setAccessible(false);
                    }
                }
            } catch (ClassNotFoundException e) {
                Log.w(TAG, resourceClass + "is not found.");
            }
        }
    }


    // DOTO jiang 先使用public 后期改回 package method
    public static void resetIds(Context context) {
        if (!loaded) {
            doResetIds(context);
            loaded = true;
        }
    }



}
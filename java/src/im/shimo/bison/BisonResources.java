package im.shimo.bison;

import android.content.Context;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;

public class BisonResources {

    private static final String TAG = "BisonResources";


    private static boolean loaded;

    private final static String INTERNAL_RESOURCE_CLASSES[] = {
            "org.chromium.components.web_contents_delegate_android.R",
            "org.chromium.content.R",
            "org.chromium.ui.R",
    };
    private final static String GENERATED_RESOURCE_CLASS = "im.shimo.bison.R";


    private static void doResetIds(Context context) {
        ClassLoader classLoader = BisonResources.class.getClassLoader();
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
                        Log.w(TAG, generatedInnerClassName + " is not found.");
                        continue;
                    }
                    Field[] fields = innerClazz.getFields();
                    for (Field field : fields) {
                        // It's final means we are probably not used as library project.
                        if (Modifier.isFinal(field.getModifiers())) field.setAccessible(true);
                        try {
                            int value = generatedInnerClazz.getField(field.getName()).getInt(null);
                            field.setInt(null, value);
                            Log.d(TAG, "set " + generatedInnerClazz.getName() + "." + field.getName() + "=" + value);
                        } catch (IllegalAccessException | NoSuchFieldException | IllegalArgumentException ignore) {

                        }
                        if (Modifier.isFinal(field.getModifiers())) field.setAccessible(false);
                    }
                }
            } catch (ClassNotFoundException e) {
                Log.w(TAG, resourceClass + " is not found.");
            }
        }
    }

    static void resetIds(Context context) {
        if (!loaded) {
            doResetIds(context);
            loaded = true;
        }
    }


}

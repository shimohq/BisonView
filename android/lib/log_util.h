
#include <android/log.h>


#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "bison", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "bison", __VA_ARGS__))






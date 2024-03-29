// create by jiang947

#ifndef BISON_COMMON_DEVTOOLS_INSTRUMENTATION_H_
#define BISON_COMMON_DEVTOOLS_INSTRUMENTATION_H_

#include "base/trace_event/trace_event.h"

namespace bison {
namespace devtools_instrumentation {

namespace internal {
constexpr const char* Category() {
  // Declared as a constexpr function to have an external linkage and to be
  // known at compile-time.
  return "Java,devtools,disabled-by-default-devtools.timeline";
}
const char kEmbedderCallback[] = "EmbedderCallback";
const char kCallbackNameArgument[] = "callbackName";
}  // namespace internal

class ScopedEmbedderCallbackTask {
 public:
  explicit ScopedEmbedderCallbackTask(const char* callback_name) {
    TRACE_EVENT_BEGIN1(internal::Category(), internal::kEmbedderCallback,
                       internal::kCallbackNameArgument, callback_name);
  }
  ~ScopedEmbedderCallbackTask() {
    TRACE_EVENT_END0(internal::Category(), internal::kEmbedderCallback);
  }

 private:

};

}  // namespace devtools_instrumentation
}  // namespace bison

#endif  // BISON_COMMON_DEVTOOLS_INSTRUMENTATION_H_

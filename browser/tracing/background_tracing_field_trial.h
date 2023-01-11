#ifndef BISON_BROWSER_TRACING_BACKGROUND_TRACING_FIELD_TRIAL_H_
#define BISON_BROWSER_TRACING_BACKGROUND_TRACING_FIELD_TRIAL_H_

namespace bison {

// Sets up background tracing in system mode if configured. Does not require the
// metrics service to be enabled.
// Since only one of the tracing modes (system/preemptive/reactive) is specified
// in a given config, if system tracing is set up as a result of this method
// call, any calls to MaybeSetupWebViewOnlyTracing() in the same browser process
// will do nothing.
void MaybeSetupSystemTracing();

// Sets up app-only background tracing if configured. Requires the metrics
// service to be enabled.
// Since only one of the tracing modes (system/preemptive/reactive) is specified
// in a given config, if app-only tracing is set up as a result of this method
// call, any calls to MaybeSetupSystemTracing() in the same browser process will
// do nothing.
void MaybeSetupWebViewOnlyTracing();

}  // namespace bison

#endif  // BISON_BROWSER_TRACING_BACKGROUND_TRACING_FIELD_TRIAL_H_

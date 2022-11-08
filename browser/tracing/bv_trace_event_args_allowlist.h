#ifndef BISON_BROWSER_TRACING_AW_TRACE_EVENT_ARGS_ALLOWLIST_H_
#define BISON_BROWSER_TRACING_AW_TRACE_EVENT_ARGS_ALLOWLIST_H_

#include "base/trace_event/trace_event_impl.h"

namespace bison {

// Used to filter trace event arguments against a allowlist of events that
// have been manually vetted to not include any PII.
bool IsTraceEventArgsAllowlisted(
    const char* category_group_name,
    const char* event_name,
    base::trace_event::ArgumentNameFilterPredicate* arg_name_filter);

// Used to filter metadata events that have been manually vetted to not include
// any PII.
bool IsTraceMetadataAllowlisted(const std::string& name);

}  // namespace bison

#endif  // BISON_BROWSER_TRACING_AW_TRACE_EVENT_ARGS_ALLOWLIST_H_

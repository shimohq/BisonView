
#ifndef BISON_BROWSER_PAGE_LOAD_METRICS_PAGE_LOAD_METRICS_INITIALIZE_H_
#define BISON_BROWSER_PAGE_LOAD_METRICS_PAGE_LOAD_METRICS_INITIALIZE_H_

namespace content {
class WebContents;
}

namespace bison {

void InitializePageLoadMetricsForWebContents(
    content::WebContents* web_contents);

}  // namespace bison

#endif  // BISON_BROWSER_PAGE_LOAD_METRICS_PAGE_LOAD_METRICS_INITIALIZE_H_

// create by jiang947


#ifndef BISON_BROWSER_BISON_RENDER_PROCESS_GONE_DELEGATE_H_
#define BISON_BROWSER_BISON_RENDER_PROCESS_GONE_DELEGATE_H_


namespace content {
class WebContents;
}

namespace bison {

// Delegate interface to handle the events that render process was gone.
class BvRenderProcessGoneDelegate {
 public:
  enum class RenderProcessGoneResult { kHandled, kUnhandled, kException };
  // Returns the BvRenderProcessGoneDelegate instance associated with
  // the given |web_contents|.
  static BvRenderProcessGoneDelegate* FromWebContents(
      content::WebContents* web_contents);

  // Notify if render process crashed or was killed.
  virtual RenderProcessGoneResult OnRenderProcessGone(int child_process_id,
                                                      bool crashed) = 0;

 protected:
  BvRenderProcessGoneDelegate() {}
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_RENDER_PROCESS_GONE_DELEGATE_H_

// create by jiang947

#ifndef BISON_BROWSER_BISON_WEB_CONTENTS_DELEGATE_H_
#define BISON_BROWSER_BISON_WEB_CONTENTS_DELEGATE_H_

#include "components/embedder_support/android/delegate/web_contents_delegate_android.h"

using content::BluetoothChooser;
using content::BluetoothScanningPrompt;
using content::JavaScriptDialogManager;
using content::OpenURLParams;
using content::PictureInPictureResult;
using content::RenderFrameHost;
using content::RenderWidgetHost;
using content::WebContents;
using content::WebContentsDelegate;

namespace bison {

class BvJavaScriptDialogManager;

class BvWebContentsDelegate
    : public web_contents_delegate_android::WebContentsDelegateAndroid {
 public:
  BvWebContentsDelegate(JNIEnv* env, jobject obj);
  ~BvWebContentsDelegate() override;

  void RendererUnresponsive(
      WebContents* source,
      RenderWidgetHost* render_widget_host,
      base::RepeatingClosure hang_monitor_restarter) override;

  void RendererResponsive(
      content::WebContents* source,
      content::RenderWidgetHost* render_widget_host) override;

  JavaScriptDialogManager* GetJavaScriptDialogManager(
      WebContents* source) override;
    void FindReply(content::WebContents* web_contents,
                 int request_id,
                 int number_of_matches,
                 const gfx::Rect& selection_rect,
                 int active_match_ordinal,
                 bool final_update) override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      std::unique_ptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override;
  void AddNewContents(content::WebContents* source,
                      std::unique_ptr<content::WebContents> new_contents,
                      const GURL& target_url,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override;

  void NavigationStateChanged(content::WebContents* source,
                              content::InvalidateTypes changed_flags) override;
  void WebContentsCreated(content::WebContents* source_contents,
                          int opener_render_process_id,
                          int opener_render_frame_id,
                          const std::string& frame_name,
                          const GURL& target_url,
                          content::WebContents* new_contents) override;

  void CloseContents(content::WebContents* source) override;
  void ActivateContents(content::WebContents* contents) override;
  void LoadingStateChanged(WebContents* source,
                           bool to_different_document) override;
  bool ShouldResumeRequestsForCreatedWindow() override;
  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      content::MediaResponseCallback callback) override;
  void EnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame,
      const blink::mojom::FullscreenOptions& options) override;
  void ExitFullscreenModeForTab(WebContents* web_contents) override;
  bool IsFullscreenForTabOrPending(const WebContents* web_contents) override;

  void UpdateUserGestureCarryoverInfo(
      content::WebContents* web_contents) override;

  std::unique_ptr<content::FileSelectListener> TakeFileSelectListener();

 private:
  bool is_fullscreen_;

  // Maintain a FileSelectListener instance passed to RunFileChooser() until
  // a callback is called.
  std::unique_ptr<content::FileSelectListener> file_select_listener_;
};

}  // namespace bison


#endif  // BISON_BROWSER_BISON_WEB_CONTENTS_DELEGATE_H_

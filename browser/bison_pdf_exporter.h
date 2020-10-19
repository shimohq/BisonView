// create by jiang947 


#ifndef BISON_BROWSER_BISON_PDF_EXPORTER_H_
#define BISON_BROWSER_BISON_PDF_EXPORTER_H_


#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"

#include "printing/page_range.h"

namespace content {
class WebContents;
}

namespace printing {
class PrintSettings;
}

namespace bison {

class BisonPdfExporter {
 public:
  BisonPdfExporter(JNIEnv* env,
                const base::android::JavaRef<jobject>& obj,
                content::WebContents* web_contents);

  ~BisonPdfExporter();

  void ExportToPdf(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& obj,
                   int fd,
                   const base::android::JavaParamRef<jintArray>& pages,
                   const base::android::JavaParamRef<jobject>& cancel_signal);

 private:
  std::unique_ptr<printing::PrintSettings> CreatePdfSettings(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& obj,
      const printing::PageRanges& page_ranges);
  void DidExportPdf(int page_count);

  JavaObjectWeakGlobalRef java_ref_;
  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(BisonPdfExporter);
};

}  // namespace bison

#endif  // BISON_BROWSER_BISON_PDF_EXPORTER_H_

// create by jiang947

#ifndef BISON_COMMON_BISON_HIT_TEST_DATA_H_
#define BISON_COMMON_BISON_HIT_TEST_DATA_H_

#include "base/strings/string16.h"
#include "url/gurl.h"

namespace bison {

// Holdes all hit test data needed by public BisonView APIs.
// The Java counter part to this is BisonContents.HitTestData.
struct BisonHitTestData {
  // Matches exactly with constants in WebView.HitTestResult, with deprecated
  // values removed.
  enum Type {
    // Default type where nothing we are interested in is hit.
    // |extra_data_for_type| will be empty. All other values should be emtpy
    // except the special case described below.
    // For special case of invalid or javascript scheme url that would
    // otherwise be type an LINK type, |href| will contain the javascript
    // string in the href attribute, and |anchor_text| and |img_src| contain
    // their normal values for the respective type.
    UNKNOWN_TYPE = 0,

    // Special case urls for SRC_LINK_TYPE below. Each type corresponds to a
    // different prefix in content url_constants. |extra_data_for_type| will
    // contain the url but with the prefix removed. |href| will contain the
    // exact href attribute string. Other fields are the same as SRC_LINK_TYPE.
    PHONE_TYPE = 2,
    GEO_TYPE = 3,
    EMAIL_TYPE = 4,

    // Hit on a pure image (without links). |extra_data_for_type|, |href|,
    // and |anchor_text| will be empty. |img_src| will contain the absolute
    // source url of the image.
    IMAGE_TYPE = 5,

    // Hit on a link with valid and non-javascript url and without embedded
    // image. |extra_data_for_type| and |href| will be the valid absolute url
    // of the link. |anchor_text| will contain the anchor text if the link is
    // an anchor tag. |img_src| will be empty.
    // Note 1: If the link url is invalid or javascript scheme, then the type
    // will be UNKNOWN_TYPE.
    // Note 2: Note that this matches SRC_ANCHOR_TYPE in the public WebView
    // Java API, but the actual tag can be something other than <a>, such as
    // <link> or <area>.
    // Note 3: |href| is not the raw attribute string, but the absolute link
    // url.
    SRC_LINK_TYPE = 7,

    // Same as SRC_LINK_TYPE except the link contains an image. |img_src| and
    // |extra_data_for_type| will contain the absolute valid url of the image
    // source. |href| will be the valid absolute url of the link. |anchor_text|
    // will be empty. All notes from SRC_LINK_TYPE apply.
    SRC_IMAGE_LINK_TYPE = 8,

    // Hit on an editable text input element. All other values will be empty.
    EDIT_TEXT_TYPE = 9,
  };

  // For all strings/GURLs, empty/invalid will become null upon conversion to
  // Java.
  int type;  // Only values from enum Type above.
  std::string extra_data_for_type;
  base::string16 href;
  base::string16 anchor_text;
  GURL img_src;

  BisonHitTestData();
  ~BisonHitTestData();
};

}  // namespace bison

#endif  // BISON_COMMON_BISON_HIT_TEST_DATA_H_
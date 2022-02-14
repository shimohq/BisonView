#include "bison/browser/bv_autofill_client.h"

#include <utility>

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_content_browser_client.h"
#include "bison/browser/bv_contents.h"
#include "bison/browser/bv_form_database_service.h"
#include "bison/bison_jni_headers/BvAutofillClient_jni.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "components/autofill/core/browser/payments/legal_message_line.h"
#include "components/autofill/core/browser/ui/autofill_popup_delegate.h"
#include "components/autofill/core/browser/ui/suggestion.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "ui/android/view_android.h"
#include "ui/gfx/geometry/rect_f.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF16ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using content::WebContents;

namespace bison {

BvAutofillClient::~BvAutofillClient() {
  HideAutofillPopup(autofill::PopupHidingReason::kTabGone);
}

void BvAutofillClient::SetSaveFormData(bool enabled) {
  save_form_data_ = enabled;
}

bool BvAutofillClient::GetSaveFormData() {
  return save_form_data_;
}

autofill::PersonalDataManager* BvAutofillClient::GetPersonalDataManager() {
  return nullptr;
}

autofill::AutocompleteHistoryManager*
BvAutofillClient::GetAutocompleteHistoryManager() {
  return BvBrowserContext::FromWebContents(web_contents_)
      ->GetAutocompleteHistoryManager();
}

PrefService* BvAutofillClient::GetPrefs() {
  return user_prefs::UserPrefs::Get(
      BvBrowserContext::FromWebContents(web_contents_));
}

syncer::SyncService* BvAutofillClient::GetSyncService() {
  return nullptr;
}

signin::IdentityManager* BvAutofillClient::GetIdentityManager() {
  return nullptr;
}

autofill::FormDataImporter* BvAutofillClient::GetFormDataImporter() {
  return nullptr;
}

autofill::payments::PaymentsClient* BvAutofillClient::GetPaymentsClient() {
  return nullptr;
}

autofill::StrikeDatabase* BvAutofillClient::GetStrikeDatabase() {
  return nullptr;
}

ukm::UkmRecorder* BvAutofillClient::GetUkmRecorder() {
  return nullptr;
}

ukm::SourceId BvAutofillClient::GetUkmSourceId() {
  // UKM recording is not supported for WebViews.
  return ukm::kInvalidSourceId;
}

autofill::AddressNormalizer* BvAutofillClient::GetAddressNormalizer() {
  return nullptr;
}

security_state::SecurityLevel
BvAutofillClient::GetSecurityLevelForUmaHistograms() {
  // The metrics are not recorded for Android webview, so return the count value
  // which will not be recorded.
  return security_state::SecurityLevel::SECURITY_LEVEL_COUNT;
}

void BvAutofillClient::ShowAutofillSettings(bool show_credit_card_settings) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::ShowUnmaskPrompt(
    const autofill::CreditCard& card,
    UnmaskCardReason reason,
    base::WeakPtr<autofill::CardUnmaskDelegate> delegate) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::OnUnmaskVerificationResult(PaymentsRpcResult result) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::ConfirmAccountNameFixFlow(
    base::OnceCallback<void(const base::string16&)> callback) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::ConfirmExpirationDateFixFlow(
    const autofill::CreditCard& card,
    base::OnceCallback<void(const base::string16&, const base::string16&)>
        callback) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::ConfirmSaveCreditCardLocally(
    const autofill::CreditCard& card,
    SaveCreditCardOptions options,
    LocalSaveCardPromptCallback callback) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::ConfirmSaveCreditCardToCloud(
    const autofill::CreditCard& card,
    const autofill::LegalMessageLines& legal_message_lines,
    SaveCreditCardOptions options,
    UploadSaveCardPromptCallback callback) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::CreditCardUploadCompleted(bool card_saved) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::ConfirmCreditCardFillAssist(
    const autofill::CreditCard& card,
    base::OnceClosure callback) {
  NOTIMPLEMENTED();
}

bool BvAutofillClient::HasCreditCardScanFeature() {
  return false;
}

void BvAutofillClient::ScanCreditCard(CreditCardScanCallback callback) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::ShowAutofillPopup(
    const autofill::AutofillClient::PopupOpenArgs& open_args,
    base::WeakPtr<autofill::AutofillPopupDelegate> delegate) {
  suggestions_ = open_args.suggestions;
  delegate_ = delegate;

  // Convert element_bounds to be in screen space.
  gfx::Rect client_area = web_contents_->GetContainerBounds();
  gfx::RectF element_bounds_in_screen_space =
      open_args.element_bounds + client_area.OffsetFromOrigin();

  ShowAutofillPopupImpl(element_bounds_in_screen_space,
                        open_args.text_direction == base::i18n::RIGHT_TO_LEFT,
                        open_args.suggestions);
}

void BvAutofillClient::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  // Leaving as an empty method since updating autofill popup window
  // dynamically does not seem to be a useful feature for android webview.
  // See crrev.com/18102002 if need to implement.
}

base::span<const autofill::Suggestion> BvAutofillClient::GetPopupSuggestions()
    const {
  NOTIMPLEMENTED();
  return base::span<const autofill::Suggestion>();
}

void BvAutofillClient::PinPopupView() {
  NOTIMPLEMENTED();
}

autofill::AutofillClient::PopupOpenArgs BvAutofillClient::GetReopenPopupArgs()
    const {
  NOTIMPLEMENTED();
  return {};
}

void BvAutofillClient::UpdatePopup(
    const std::vector<autofill::Suggestion>& suggestions,
    autofill::PopupType popup_type) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::HideAutofillPopup(autofill::PopupHidingReason reason) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  delegate_.reset();
  Java_BvAutofillClient_hideAutofillPopup(env, obj);
}

bool BvAutofillClient::IsAutocompleteEnabled() {
  bool enabled = GetSaveFormData();
  if (!autocomplete_uma_recorded_) {
    UMA_HISTOGRAM_BOOLEAN("Autofill.AutocompleteEnabled", enabled);
    autocomplete_uma_recorded_ = true;
  }
  return enabled;
}

void BvAutofillClient::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<autofill::FormStructure*>& forms) {}

void BvAutofillClient::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) {}

bool BvAutofillClient::IsContextSecure() {
  content::SSLStatus ssl_status;
  content::NavigationEntry* navigation_entry =
      web_contents_->GetController().GetLastCommittedEntry();
  if (!navigation_entry)
    return false;

  ssl_status = navigation_entry->GetSSL();
  // Note: As of crbug.com/701018, Chrome relies on SecurityStateTabHelper to
  // determine whether the page is secure, but WebView has no equivalent class.

  return navigation_entry->GetURL().SchemeIsCryptographic() &&
         ssl_status.certificate &&
         !net::IsCertStatusError(ssl_status.cert_status) &&
         !(ssl_status.content_status &
           content::SSLStatus::RAN_INSECURE_CONTENT);
}

bool BvAutofillClient::ShouldShowSigninPromo() {
  return false;
}

bool BvAutofillClient::AreServerCardsSupported() {
  return true;
}

void BvAutofillClient::ExecuteCommand(int id) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::LoadRiskData(
    base::OnceCallback<void(const std::string&)> callback) {
  NOTIMPLEMENTED();
}

void BvAutofillClient::Dismissed(JNIEnv* env,
                                 const JavaParamRef<jobject>& obj) {
  anchor_view_.Reset();
}

void BvAutofillClient::SuggestionSelected(JNIEnv* env,
                                          const JavaParamRef<jobject>& object,
                                          jint position) {
  if (delegate_) {
    delegate_->DidAcceptSuggestion(suggestions_[position].value,
                                   suggestions_[position].frontend_id,
                                   position);
  }
}

// Ownership: The native object is created (if autofill enabled) and owned by
// BvContents. The native object creates the java peer which handles most
// autofill functionality at the java side. The java peer is owned by Java
// BvContents. The native object only maintains a weak ref to it.
BvAutofillClient::BvAutofillClient(WebContents* contents)
    : web_contents_(contents) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> delegate;
  delegate.Reset(
      Java_BvAutofillClient_create(env, reinterpret_cast<intptr_t>(this)));

  BvContents* bison_contents = BvContents::FromWebContents(web_contents_);
  bison_contents->SetAutofillClient(delegate);
  java_ref_ = JavaObjectWeakGlobalRef(env, delegate);
}

void BvAutofillClient::ShowAutofillPopupImpl(
    const gfx::RectF& element_bounds,
    bool is_rtl,
    const std::vector<autofill::Suggestion>& suggestions) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  // We need an array of AutofillSuggestion.
  size_t count = suggestions.size();

  ScopedJavaLocalRef<jobjectArray> data_array =
      Java_BvAutofillClient_createAutofillSuggestionArray(env, count);

  for (size_t i = 0; i < count; ++i) {
    ScopedJavaLocalRef<jstring> name =
        ConvertUTF16ToJavaString(env, suggestions[i].value);
    ScopedJavaLocalRef<jstring> label =
        ConvertUTF16ToJavaString(env, suggestions[i].label);
    Java_BvAutofillClient_addToAutofillSuggestionArray(
        env, data_array, i, name, label, suggestions[i].frontend_id);
  }
  ui::ViewAndroid* view_android = web_contents_->GetNativeView();
  if (!view_android)
    return;

  const ScopedJavaLocalRef<jobject> current_view = anchor_view_.view();
  if (current_view.is_null())
    anchor_view_ = view_android->AcquireAnchorView();

  const ScopedJavaLocalRef<jobject> view = anchor_view_.view();
  if (view.is_null())
    return;

  view_android->SetAnchorRect(view, element_bounds);
  Java_BvAutofillClient_showAutofillPopup(env, obj, view, is_rtl, data_array);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BvAutofillClient)

}  // namespace bison

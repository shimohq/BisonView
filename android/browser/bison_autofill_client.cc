// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/android/browser/bison_autofill_client.h"

#include <utility>

#include "bison/android/browser/bison_browser_context.h"
#include "bison/android/browser/bison_content_browser_client.h"
#include "bison/android/browser/bison_contents.h"
#include "bison/android/browser/bison_form_database_service.h"
#include "bison/android/browser_jni_headers/BisonAutofillClient_jni.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
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

BisonAutofillClient::~BisonAutofillClient() {
  HideAutofillPopup();
}

void BisonAutofillClient::SetSaveFormData(bool enabled) {
  save_form_data_ = enabled;
}

bool BisonAutofillClient::GetSaveFormData() {
  return save_form_data_;
}

autofill::PersonalDataManager* BisonAutofillClient::GetPersonalDataManager() {
  return nullptr;
}

autofill::AutocompleteHistoryManager*
BisonAutofillClient::GetAutocompleteHistoryManager() {
  return BisonBrowserContext::FromWebContents(web_contents_)
      ->GetAutocompleteHistoryManager();
}

PrefService* BisonAutofillClient::GetPrefs() {
  return user_prefs::UserPrefs::Get(
      BisonBrowserContext::FromWebContents(web_contents_));
}

syncer::SyncService* BisonAutofillClient::GetSyncService() {
  return nullptr;
}

signin::IdentityManager* BisonAutofillClient::GetIdentityManager() {
  return nullptr;
}

autofill::FormDataImporter* BisonAutofillClient::GetFormDataImporter() {
  return nullptr;
}

autofill::payments::PaymentsClient* BisonAutofillClient::GetPaymentsClient() {
  return nullptr;
}

autofill::StrikeDatabase* BisonAutofillClient::GetStrikeDatabase() {
  return nullptr;
}

ukm::UkmRecorder* BisonAutofillClient::GetUkmRecorder() {
  return nullptr;
}

ukm::SourceId BisonAutofillClient::GetUkmSourceId() {
  // UKM recording is not supported for WebViews.
  return ukm::kInvalidSourceId;
}

autofill::AddressNormalizer* BisonAutofillClient::GetAddressNormalizer() {
  return nullptr;
}

security_state::SecurityLevel
BisonAutofillClient::GetSecurityLevelForUmaHistograms() {
  // The metrics are not recorded for Android webview, so return the count value
  // which will not be recorded.
  return security_state::SecurityLevel::SECURITY_LEVEL_COUNT;
}

void BisonAutofillClient::ShowAutofillSettings(bool show_credit_card_settings) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ShowUnmaskPrompt(
    const autofill::CreditCard& card,
    UnmaskCardReason reason,
    base::WeakPtr<autofill::CardUnmaskDelegate> delegate) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::OnUnmaskVerificationResult(PaymentsRpcResult result) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ShowLocalCardMigrationDialog(
    base::OnceClosure show_migration_dialog_closure) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ConfirmMigrateLocalCardToCloud(
    const autofill::LegalMessageLines& legal_message_lines,
    const std::string& user_email,
    const std::vector<autofill::MigratableCreditCard>& migratable_credit_cards,
    LocalCardMigrationCallback start_migrating_cards_callback) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ShowLocalCardMigrationResults(
    const bool has_server_error,
    const base::string16& tip_message,
    const std::vector<autofill::MigratableCreditCard>& migratable_credit_cards,
    MigrationDeleteCardCallback delete_local_card_callback) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ShowWebauthnOfferDialog(
    WebauthnOfferDialogCallback callback) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ConfirmSaveAutofillProfile(
    const autofill::AutofillProfile& profile,
    base::OnceClosure callback) {
  // Since there is no confirmation needed to save an Autofill Profile,
  // running |callback| will proceed with saving |profile|.
  std::move(callback).Run();
}

void BisonAutofillClient::ConfirmSaveCreditCardLocally(
    const autofill::CreditCard& card,
    SaveCreditCardOptions options,
    LocalSaveCardPromptCallback callback) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ConfirmAccountNameFixFlow(
    base::OnceCallback<void(const base::string16&)> callback) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ConfirmExpirationDateFixFlow(
    const autofill::CreditCard& card,
    base::OnceCallback<void(const base::string16&, const base::string16&)>
        callback) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ConfirmSaveCreditCardToCloud(
    const autofill::CreditCard& card,
    const autofill::LegalMessageLines& legal_message_lines,
    SaveCreditCardOptions options,
    UploadSaveCardPromptCallback callback) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::CreditCardUploadCompleted(bool card_saved) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ConfirmCreditCardFillAssist(
    const autofill::CreditCard& card,
    base::OnceClosure callback) {
  NOTIMPLEMENTED();
}

bool BisonAutofillClient::HasCreditCardScanFeature() {
  return false;
}

void BisonAutofillClient::ScanCreditCard(const CreditCardScanCallback& callback) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::ShowAutofillPopup(
    const gfx::RectF& element_bounds,
    base::i18n::TextDirection text_direction,
    const std::vector<autofill::Suggestion>& suggestions,
    bool /*unused_autoselect_first_suggestion*/,
    autofill::PopupType popup_type,
    base::WeakPtr<autofill::AutofillPopupDelegate> delegate) {
  suggestions_ = suggestions;
  delegate_ = delegate;

  // Convert element_bounds to be in screen space.
  gfx::Rect client_area = web_contents_->GetContainerBounds();
  gfx::RectF element_bounds_in_screen_space =
      element_bounds + client_area.OffsetFromOrigin();

  ShowAutofillPopupImpl(element_bounds_in_screen_space,
                        text_direction == base::i18n::RIGHT_TO_LEFT,
                        suggestions);
}

void BisonAutofillClient::UpdateAutofillPopupDataListValues(
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  // Leaving as an empty method since updating autofill popup window
  // dynamically does not seem to be a useful feature for android webview.
  // See crrev.com/18102002 if need to implement.
}

void BisonAutofillClient::HideAutofillPopup() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  delegate_.reset();
  Java_BisonAutofillClient_hideAutofillPopup(env, obj);
}

bool BisonAutofillClient::IsAutocompleteEnabled() {
  bool enabled = GetSaveFormData();
  if (!autocomplete_uma_recorded_) {
    UMA_HISTOGRAM_BOOLEAN("Autofill.AutocompleteEnabled", enabled);
    autocomplete_uma_recorded_ = true;
  }
  return enabled;
}

void BisonAutofillClient::PropagateAutofillPredictions(
    content::RenderFrameHost* rfh,
    const std::vector<autofill::FormStructure*>& forms) {}

void BisonAutofillClient::DidFillOrPreviewField(
    const base::string16& autofilled_value,
    const base::string16& profile_full_name) {}

bool BisonAutofillClient::IsContextSecure() {
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

bool BisonAutofillClient::ShouldShowSigninPromo() {
  return false;
}

bool BisonAutofillClient::AreServerCardsSupported() {
  return true;
}

void BisonAutofillClient::ExecuteCommand(int id) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::LoadRiskData(
    base::OnceCallback<void(const std::string&)> callback) {
  NOTIMPLEMENTED();
}

void BisonAutofillClient::Dismissed(JNIEnv* env,
                                 const JavaParamRef<jobject>& obj) {
  anchor_view_.Reset();
}

void BisonAutofillClient::SuggestionSelected(JNIEnv* env,
                                          const JavaParamRef<jobject>& object,
                                          jint position) {
  if (delegate_) {
    delegate_->DidAcceptSuggestion(suggestions_[position].value,
                                   suggestions_[position].frontend_id,
                                   position);
  }
}

// Ownership: The native object is created (if autofill enabled) and owned by
// BisonContents. The native object creates the java peer which handles most
// autofill functionality at the java side. The java peer is owned by Java
// BisonContents. The native object only maintains a weak ref to it.
BisonAutofillClient::BisonAutofillClient(WebContents* contents)
    : web_contents_(contents) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> delegate;
  delegate.Reset(
      Java_BisonAutofillClient_create(env, reinterpret_cast<intptr_t>(this)));

  BisonContents* aw_contents = BisonContents::FromWebContents(web_contents_);
  aw_contents->SetBisonAutofillClient(delegate);
  java_ref_ = JavaObjectWeakGlobalRef(env, delegate);
}

void BisonAutofillClient::ShowAutofillPopupImpl(
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
      Java_BisonAutofillClient_createAutofillSuggestionArray(env, count);

  for (size_t i = 0; i < count; ++i) {
    ScopedJavaLocalRef<jstring> name =
        ConvertUTF16ToJavaString(env, suggestions[i].value);
    ScopedJavaLocalRef<jstring> label =
        ConvertUTF16ToJavaString(env, suggestions[i].label);
    Java_BisonAutofillClient_addToAutofillSuggestionArray(
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
  Java_BisonAutofillClient_showAutofillPopup(env, obj, view, is_rtl, data_array);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BisonAutofillClient)

}  // namespace bison

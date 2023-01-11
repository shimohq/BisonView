#include "bison/browser/bv_ssl_host_state_delegate.h"

#include "base/callback.h"
#include "net/base/hash_value.h"

using content::SSLHostStateDelegate;

namespace bison {

namespace internal {

CertPolicy::CertPolicy() {
}
CertPolicy::~CertPolicy() {
}

// For an allowance, we consider a given |cert| to be a match to a saved
// allowed cert if the |error| is an exact match to or subset of the errors
// in the saved CertStatus.
bool CertPolicy::Check(const net::X509Certificate& cert, int error) const {
  net::SHA256HashValue fingerprint = cert.CalculateChainFingerprint256();
  auto allowed_iter = allowed_.find(fingerprint);
  if ((allowed_iter != allowed_.end()) && (allowed_iter->second & error) &&
      ((allowed_iter->second & error) == error)) {
    return true;
  }
  return false;
}

void CertPolicy::Allow(const net::X509Certificate& cert, int error) {
  // If this same cert had already been saved with a different error status,
  // this will replace it with the new error status.
  net::SHA256HashValue fingerprint = cert.CalculateChainFingerprint256();
  allowed_[fingerprint] = error;
}

}  // namespace internal

BvSSLHostStateDelegate::BvSSLHostStateDelegate() {
}

BvSSLHostStateDelegate::~BvSSLHostStateDelegate() {
}

void BvSSLHostStateDelegate::HostRanInsecureContent(
    const std::string& host,
    int child_id,
    InsecureContentType content_type) {
}

bool BvSSLHostStateDelegate::DidHostRunInsecureContent(
    const std::string& host,
    int child_id,
    InsecureContentType content_type) {
  return false;
}

void BvSSLHostStateDelegate::AllowHttpForHost(
    const std::string& host,
    content::WebContents* web_contents) {
}

bool BvSSLHostStateDelegate::IsHttpAllowedForHost(
    const std::string& host,
    content::WebContents* web_contents) {

  return false;
}

void BvSSLHostStateDelegate::AllowCert(const std::string& host,
                                       const net::X509Certificate& cert,
                                       int error,
                                       content::WebContents* web_contents) {
  cert_policy_for_host_[host].Allow(cert, error);
}

void BvSSLHostStateDelegate::Clear(
    base::RepeatingCallback<bool(const std::string&)> host_filter) {
  if (!host_filter) {
    cert_policy_for_host_.clear();
    return;
  }

  for (auto it = cert_policy_for_host_.begin();
       it != cert_policy_for_host_.end();) {
    auto next_it = std::next(it);

    if (host_filter.Run(it->first))
      cert_policy_for_host_.erase(it);

    it = next_it;
  }
}

SSLHostStateDelegate::CertJudgment BvSSLHostStateDelegate::QueryPolicy(
    const std::string& host,
    const net::X509Certificate& cert,
    int error,
    content::WebContents* web_contents) {
  return cert_policy_for_host_[host].Check(cert, error)
             ? SSLHostStateDelegate::ALLOWED
             : SSLHostStateDelegate::DENIED;
}

void BvSSLHostStateDelegate::RevokeUserAllowExceptions(
    const std::string& host) {
  cert_policy_for_host_.erase(host);
}

bool BvSSLHostStateDelegate::HasAllowException(
    const std::string& host,
    content::WebContents* web_contents) {
  auto policy_iterator = cert_policy_for_host_.find(host);
  return policy_iterator != cert_policy_for_host_.end() &&
         policy_iterator->second.HasAllowException();
}

}  // namespace bison

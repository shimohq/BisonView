// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/bv_content_browser_client.h"

#include "bison/browser/bv_browser_context.h"
#include "bison/browser/bv_print_manager.h"
#include "bison/browser/renderer_host/bv_render_view_host_ext.h"

#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/cdm/browser/media_drm_storage_impl.h"
#include "components/content_capture/browser/onscreen_content_provider.h"
#include "components/page_load_metrics/browser/metrics_web_contents_observer.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/content/security_interstitial_tab_helper.h"
#include "components/spellcheck/spellcheck_buildflags.h"
#include "content/public/browser/browser_associated_interface.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "media/mojo/buildflags.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "components/spellcheck/browser/spell_check_host_impl.h"
#endif

namespace bison {

namespace {

#if BUILDFLAG(ENABLE_MOJO_CDM)
void CreateOriginId(cdm::MediaDrmStorageImpl::OriginIdObtainedCB callback) {
  std::move(callback).Run(true, base::UnguessableToken::Create());
}

void AllowEmptyOriginIdCB(base::OnceCallback<void(bool)> callback) {
  // Since CreateOriginId() always returns a non-empty origin ID, we don't need
  // to allow empty origin ID.
  std::move(callback).Run(false);
}

void CreateMediaDrmStorage(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingReceiver<::media::mojom::MediaDrmStorage> receiver) {
  DCHECK(render_frame_host);

  if (render_frame_host->GetLastCommittedOrigin().opaque()) {
    DVLOG(1) << __func__ << ": Unique origin.";
    return;
  }

  auto* bv_browser_context =
      static_cast<BvBrowserContext*>(render_frame_host->GetBrowserContext());
  DCHECK(bv_browser_context) << "BvBrowserContext not available.";

  PrefService* pref_service = bv_browser_context->GetPrefService();
  DCHECK(pref_service);

  // The object will be deleted on connection error, or when the frame navigates
  // away.
  new cdm::MediaDrmStorageImpl(
      render_frame_host, pref_service, base::BindRepeating(&CreateOriginId),
      base::BindRepeating(&AllowEmptyOriginIdCB), std::move(receiver));
}
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

// Helper method that checks the RenderProcessHost is still alive before hopping
// over to the IO thread.
// jiang
// void MaybeCreateSafeBrowsing(
//     int rph_id,
//     content::ResourceContext* resource_context,
//     base::RepeatingCallback<scoped_refptr<safe_browsing::UrlCheckerDelegate>()>
//         get_checker_delegate,
//     mojo::PendingReceiver<safe_browsing::mojom::SafeBrowsing> receiver) {
//   DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

//   content::RenderProcessHost* render_process_host =
//       content::RenderProcessHost::FromID(rph_id);
//   if (!render_process_host)
//     return;

//   content::GetIOThreadTaskRunner({})->PostTask(
//       FROM_HERE,
//       base::BindOnce(&safe_browsing::MojoSafeBrowsingImpl::MaybeCreate, rph_id,
//                      resource_context, std::move(get_checker_delegate),
//                      std::move(receiver)));
// }

}  // anonymous namespace

void BvContentBrowserClient::BindMediaServiceReceiver(
    content::RenderFrameHost* render_frame_host,
    mojo::GenericPendingReceiver receiver) {
#if BUILDFLAG(ENABLE_MOJO_CDM)
  if (auto r = receiver.As<media::mojom::MediaDrmStorage>()) {
    CreateMediaDrmStorage(render_frame_host, std::move(r));
    return;
  }
#endif
}

void BvContentBrowserClient::
    RegisterAssociatedInterfaceBindersForRenderFrameHost(
        content::RenderFrameHost& render_frame_host,
        blink::AssociatedInterfaceRegistry& associated_registry) {
  // TODO(lingqi): Swap the parameters so that lambda functions are not needed.
  associated_registry.AddInterface(base::BindRepeating(
      [](content::RenderFrameHost* render_frame_host,
         mojo::PendingAssociatedReceiver<autofill::mojom::AutofillDriver>
             receiver) {
        autofill::ContentAutofillDriverFactory::BindAutofillDriver(
            std::move(receiver), render_frame_host);
      },
      &render_frame_host));
  associated_registry.AddInterface(base::BindRepeating(
      [](content::RenderFrameHost* render_frame_host,
         mojo::PendingAssociatedReceiver<
             content_capture::mojom::ContentCaptureReceiver> receiver) {
        content_capture::OnscreenContentProvider::BindContentCaptureReceiver(
            std::move(receiver), render_frame_host);
      },
      &render_frame_host));
  associated_registry.AddInterface(base::BindRepeating(
      [](content::RenderFrameHost* render_frame_host,
         mojo::PendingAssociatedReceiver<mojom::FrameHost> receiver) {
        BvRenderViewHostExt::BindFrameHost(std::move(receiver),
                                           render_frame_host);
      },
      &render_frame_host));
  associated_registry.AddInterface(base::BindRepeating(
      [](content::RenderFrameHost* render_frame_host,
         mojo::PendingAssociatedReceiver<
             page_load_metrics::mojom::PageLoadMetrics> receiver) {
        page_load_metrics::MetricsWebContentsObserver::BindPageLoadMetrics(
            std::move(receiver), render_frame_host);
      },
      &render_frame_host));
  associated_registry.AddInterface(base::BindRepeating(
      [](content::RenderFrameHost* render_frame_host,
         mojo::PendingAssociatedReceiver<printing::mojom::PrintManagerHost>
             receiver) {
        BvPrintManager::BindPrintManagerHost(std::move(receiver),
                                             render_frame_host);
      },
      &render_frame_host));
  associated_registry.AddInterface(base::BindRepeating(
      [](content::RenderFrameHost* render_frame_host,
         mojo::PendingAssociatedReceiver<
             security_interstitials::mojom::InterstitialCommands> receiver) {
        security_interstitials::SecurityInterstitialTabHelper::
            BindInterstitialCommands(std::move(receiver), render_frame_host);
      },
      &render_frame_host));
}

void BvContentBrowserClient::ExposeInterfacesToRenderer(
    service_manager::BinderRegistry* registry,
    blink::AssociatedInterfaceRegistry* associated_registry,
    content::RenderProcessHost* render_process_host) {
  // content::ResourceContext* resource_context =
  //     render_process_host->GetBrowserContext()->GetResourceContext();
  //jiang
  // registry->AddInterface(
  //     base::BindRepeating(
  //         &MaybeCreateSafeBrowsing, render_process_host->GetID(),
  //         resource_context,
  //         base::BindRepeating(
  //             &BvContentBrowserClient::GetSafeBrowsingUrlCheckerDelegate,
  //             base::Unretained(this))),
  //     content::GetUIThreadTaskRunner({}));

#if BUILDFLAG(ENABLE_SPELLCHECK)
  auto create_spellcheck_host =
      [](mojo::PendingReceiver<spellcheck::mojom::SpellCheckHost> receiver) {
        mojo::MakeSelfOwnedReceiver(std::make_unique<SpellCheckHostImpl>(),
                                    std::move(receiver));
      };
  registry->AddInterface(base::BindRepeating(create_spellcheck_host),
                         content::GetUIThreadTaskRunner({}));
#endif
}

}  // namespace android_webview

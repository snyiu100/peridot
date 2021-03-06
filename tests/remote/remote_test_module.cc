// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/cpp/component.h>
#include <fuchsia/cpp/modular.h>
#include <lib/async/cpp/task.h>
#include <lib/async/default.h>

#include "garnet/lib/callback/scoped_callback.h"
#include "lib/app/cpp/connect.h"
#include "lib/app_driver/cpp/module_driver.h"
#include "lib/fidl/cpp/binding_set.h"
#include "lib/fidl/cpp/interface_request.h"
#include "lib/fsl/tasks/message_loop.h"
#include "lib/fxl/memory/weak_ptr.h"
#include "peridot/lib/testing/reporting.h"
#include "peridot/lib/testing/testing.h"

using modular::testing::TestPoint;

namespace {

// This is how long we wait for the test to finish before we timeout and tear
// down our test.
//
// HACK(mesch): This is rather long because we stop the test module very
// quickly, so the StopCall that takes it down has to wait for the StoryShell
// (and flutter, and dart) to come up before it can defocus the module. On a
// slow machine, dart and flutter start really slowly. On a faster machine, test
// should pass much quicker.
constexpr int kTimeoutMilliseconds = 120000;

class ParentApp {
 public:
  ParentApp(
      modular::ModuleHost* module_host,
      fidl::InterfaceRequest<views_v1::ViewProvider> /*view_provider_request*/,
      fidl::InterfaceRequest<component::ServiceProvider> /*outgoing_services*/)
      : module_host_(module_host), weak_ptr_factory_(this) {
    modular::testing::Init(module_host->application_context(), __FILE__);
    initialized_.Pass();

    // Start a timer to quit in case another test component misbehaves and we
    // time out.
    async::PostDelayedTask(
        async_get_default(),
        callback::MakeScoped(
            weak_ptr_factory_.GetWeakPtr(),
            [this] { module_host_->module_context()->Done(); }),
        zx::msec(kTimeoutMilliseconds));

    remote_invoker_ =
        module_host_->application_context()
            ->ConnectToEnvironmentService<modular::RemoteInvoker>();
    remote_invoker_connected_.Pass();

    module_host_->module_context()->Ready();

    remote_invoker_->StartOnDevice(
        "test1", "test2", [this](fidl::StringPtr page_id) {
          if (page_id.get().empty()) {
            FXL_LOG(INFO) << "Failed to send rehydrate";
          } else {
            FXL_LOG(INFO) << "Sent rehydrate to page " << page_id;
            rehydrate_story_called_.Pass();
          }
          module_host_->module_context()->Done();
        });
  }

  // Called by ModuleDriver.
  void Terminate(const std::function<void()>& done) {
    stopped_.Pass();
    modular::testing::Done(done);
  }

 private:
  modular::ModuleHost* module_host_;
  component::ServiceProviderPtr incoming_services_;
  component::ServiceProviderPtr outgoing_services_;
  modular::ComponentContextPtr component_context_;
  modular::RemoteInvokerPtr remote_invoker_;

  TestPoint initialized_{"Remote service test module initialized"};
  TestPoint remote_invoker_connected_{"Remote service connected"};
  TestPoint rehydrate_story_called_{"Rehydrate story called"};
  TestPoint stopped_{"Remote service test module stopped"};

  fxl::WeakPtrFactory<ParentApp> weak_ptr_factory_;
};

}  // namespace

int main(int /*argc*/, const char** /*argv*/) {
  fsl::MessageLoop loop;
  auto app_context = component::ApplicationContext::CreateFromStartupInfo();
  modular::ModuleDriver<ParentApp> driver(app_context.get(),
                                          [&loop] { loop.QuitNow(); });
  loop.Run();
  return 0;
}

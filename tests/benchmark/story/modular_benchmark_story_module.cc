// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <trace-provider/provider.h>
#include <trace/event.h>
#include <trace/observer.h>
#include <string>

#include <fuchsia/cpp/modular.h>
#include <fuchsia/cpp/views_v1.h>
#include "lib/app_driver/cpp/module_driver.h"
#include "lib/fidl/cpp/binding.h"
#include "lib/fsl/tasks/message_loop.h"
#include "peridot/tests/benchmark/story/tracing_waiter.h"

namespace {

// This Module updates its root link 100 times and then just sits there until
// it's terminated.
class NullModule : modular::LinkWatcher {
 public:
  NullModule(
      modular::ModuleHost* const module_host,
      fidl::InterfaceRequest<views_v1::ViewProvider> /*view_provider_request*/,
      fidl::InterfaceRequest<component::ServiceProvider> /*outgoing_services*/)
      : module_host_(module_host), link_watcher_binding_(this) {
    module_host_->module_context()->Ready();
    module_host_->module_context()->GetLink(nullptr, link_.NewRequest());

    // Will call Notify() with current value.
    link_->WatchAll(link_watcher_binding_.NewBinding());
  }

  // Called by ModuleDriver.
  void Terminate(const std::function<void()>& done) { done(); }

 private:
  void Set() {
    TRACE_ASYNC_BEGIN("benchmark", "link/set", 0);

    // Corresponding TRACE_FLOW_BEGIN() is in the user shell.
    TRACE_FLOW_END("benchmark", "link/trans", count_);

    link_->Set(nullptr, std::to_string(count_));
  }

  // |LinkWatcher|
  void Notify(fidl::StringPtr json) override {
    // First invocation is from WatchAll(); next from Set().
    if (count_ == -1) {
      count_ = 0;
      tracing_waiter_.WaitForTracing([this] { Set(); });
      return;
    }

    TRACE_ASYNC_END("benchmark", "link/set", 0);
    if (++count_ <= 100) {
      Set();
    }
  }

  modular::ModuleHost* const module_host_;
  modular::TracingWaiter tracing_waiter_;
  modular::LinkPtr link_;
  fidl::Binding<modular::LinkWatcher> link_watcher_binding_;

  int count_{-1};
};

}  // namespace

int main(int /*argc*/, const char** /*argv*/) {
  fsl::MessageLoop loop;
  auto app_context = component::ApplicationContext::CreateFromStartupInfo();
  modular::ModuleDriver<NullModule> driver(app_context.get(),
                                           [&loop] { loop.QuitNow(); });
  loop.Run();
  return 0;
}

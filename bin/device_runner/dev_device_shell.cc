// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implementation of the DeviceShell service that passes a command line
// configurable user name to its UserProvider, and is able to run a story with a
// single module through its life cycle.

#include <memory>

#include "apps/modular/lib/fidl/single_service_view_app.h"
#include "apps/modular/lib/testing/reporting.h"
#include "apps/modular/lib/testing/testing.h"
#include "apps/modular/services/device/device_shell.fidl.h"
#include "apps/modular/services/device/user_provider.fidl.h"
#include "lib/ftl/command_line.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/macros.h"
#include "lib/mtl/tasks/message_loop.h"

namespace {

class Settings {
 public:
  explicit Settings(const ftl::CommandLine& command_line) {
    // device_name will be set to the device's hostname if it is empty or null
    device_name =
        command_line.GetOptionValueWithDefault("device_name", "");

    // default user is incognito
    user = command_line.GetOptionValueWithDefault("user", "");

    // If passed, runs as a test harness.
    test = command_line.HasOption("test");
  }

  std::string device_name;
  std::string user;
  bool test{};
};

class DevDeviceShellApp : modular::SingleServiceViewApp<modular::DeviceShell>,
                          modular::UserWatcher {
 public:
  DevDeviceShellApp(const Settings& settings)
      : settings_(settings), user_watcher_binding_(this) {
    if (settings_.test) {
      modular::testing::Init(application_context(), __FILE__);
    }
  }
  ~DevDeviceShellApp() override = default;

 private:
  // |SingleServiceViewApp|
  void CreateView(
      fidl::InterfaceRequest<mozart::ViewOwner> view_owner_request,
      fidl::InterfaceRequest<app::ServiceProvider> services) override {
    view_owner_request_ = std::move(view_owner_request);
    Connect();
  }

  // |DeviceShell|
  void Initialize(fidl::InterfaceHandle<modular::DeviceShellContext>
                      device_shell_context) override {
    device_shell_context_.Bind(std::move(device_shell_context));
    device_shell_context_->GetUserProvider(user_provider_.NewRequest());

    Connect();
  }

  // |DeviceShell|
  void Terminate(const TerminateCallback& done) override {
    FTL_LOG(INFO) << "DeviceShell::Terminate()";

    if (settings_.test) {
      modular::testing::Teardown();
    }

    mtl::MessageLoop::GetCurrent()->PostQuitTask();
    done();
  }

  // |DeviceShell|
  void GetAuthenticationContext(
      const fidl::String& username,
      fidl::InterfaceRequest<modular::AuthenticationContext> request) override {
    FTL_LOG(INFO)
        << "DeviceShell::GetAuthenticationContext() is unimplemented.";
  }

  // |UserWatcher|
  void OnLogout() override {
    FTL_LOG(INFO) << "UserWatcher::OnLogout()";
    device_shell_context_->Shutdown();
  }

  void Login(const std::string& account_id) {
    auto params = modular::UserLoginParams::New();
    params->account_id = account_id;
    params->view_owner = std::move(view_owner_request_);
    params->user_controller = user_controller_.NewRequest();
    user_provider_->Login(std::move(params));
    user_controller_->Watch(user_watcher_binding_.NewBinding());
  }

  void Connect() {
    if (user_provider_ && view_owner_request_) {
      if (settings_.user.empty()) {
        // Incognito mode.
        Login("");
        return;
      }

      user_provider_->PreviousUsers(
          [this](fidl::Array<modular::auth::AccountPtr> accounts) {
            FTL_LOG(INFO) << "Found " << accounts.size()
                          << " users in the user "
                          << "database";

            // Not running in incognito mode. Add the user if not already
            // added.
            std::string account_id;
            for (const auto& account : accounts) {
              if (account->display_name == settings_.user) {
                account_id = account->id;
                break;
              }
            }
            if (account_id.empty()) {
              user_provider_->AddUser(
                  modular::auth::IdentityProvider::DEV, settings_.user,
                  settings_.device_name, "" /* servername */,
                  [this](modular::auth::AccountPtr account,
                         const fidl::String& status) { Login(account->id); });
            } else {
              Login(account_id);
            }
          });
    }
  }

  const Settings settings_;
  fidl::Binding<modular::UserWatcher> user_watcher_binding_;
  fidl::InterfaceRequest<mozart::ViewOwner> view_owner_request_;
  modular::DeviceShellContextPtr device_shell_context_;
  modular::UserControllerPtr user_controller_;
  modular::UserProviderPtr user_provider_;
  FTL_DISALLOW_COPY_AND_ASSIGN(DevDeviceShellApp);
};

}  // namespace

int main(int argc, const char** argv) {
  auto command_line = ftl::CommandLineFromArgcArgv(argc, argv);
  Settings settings(command_line);

  mtl::MessageLoop loop;
  DevDeviceShellApp app(settings);
  loop.Run();
  return 0;
}

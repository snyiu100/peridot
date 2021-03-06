// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "peridot/lib/testing/component_context_fake.h"

#include "lib/fxl/logging.h"

namespace modular {

ComponentContextFake::ComponentContextFake() {}

ComponentContextFake::~ComponentContextFake() = default;

void ComponentContextFake::Connect(
    fidl::InterfaceRequest<ComponentContext> request) {
  bindings_.AddBinding(this, std::move(request));
}

void ComponentContextFake::GetLedger(
    fidl::InterfaceRequest<ledger::Ledger> request,
    GetLedgerCallback result) {
  FXL_NOTIMPLEMENTED();
}

void ComponentContextFake::ConnectToAgent(
    fidl::StringPtr url,
    fidl::InterfaceRequest<component::ServiceProvider>
        incoming_services_request,
    fidl::InterfaceRequest<AgentController> agent_controller_request) {
  FXL_NOTIMPLEMENTED();
}

void ComponentContextFake::ObtainMessageQueue(
    fidl::StringPtr name,
    fidl::InterfaceRequest<MessageQueue> request) {
  FXL_NOTIMPLEMENTED();
}

void ComponentContextFake::DeleteMessageQueue(fidl::StringPtr name) {
  FXL_NOTIMPLEMENTED();
}

void ComponentContextFake::GetMessageSender(
    fidl::StringPtr queue_token,
    fidl::InterfaceRequest<MessageSender> request) {
  FXL_NOTIMPLEMENTED();
}

void ComponentContextFake::GetEntityResolver(
    fidl::InterfaceRequest<EntityResolver> request) {
  entity_resolver_.Connect(std::move(request));
}

void ComponentContextFake::CreateEntityWithData(
    fidl::VectorPtr<TypeToDataEntry> type_to_data,
    CreateEntityWithDataCallback result) {
  FXL_NOTIMPLEMENTED();
}

void ComponentContextFake::GetPackageName(GetPackageNameCallback result) {
  FXL_NOTIMPLEMENTED();
}

}  // namespace modular

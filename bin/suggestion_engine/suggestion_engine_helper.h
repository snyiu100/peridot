// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERIDOT_BIN_SUGGESTION_ENGINE_SUGGESTION_ENGINE_HELPER_H_
#define PERIDOT_BIN_SUGGESTION_ENGINE_SUGGESTION_ENGINE_HELPER_H_

#include "peridot/bin/suggestion_engine/suggestion_prototype.h"

namespace modular {
// (proposer ID, proposal ID) => suggestion prototype
using SuggestionPrototypeMap = std::map<std::pair<std::string, std::string>,
                                        std::unique_ptr<SuggestionPrototype>>;

// Creates a suggestion prototype owned by the given |SuggestionPrototypeMap|.
SuggestionPrototype* CreateSuggestionPrototype(SuggestionPrototypeMap* owner,
                                               const std::string& source_url,
                                               Proposal proposal);

} // namespace modular

#endif  // PERIDOT_BIN_SUGGESTION_ENGINE_SUGGESTION_ENGINE_HELPER_H_

//
// Copyright 2024 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "TopicFilterDefs.h"

namespace cc_mqtt311_client
{

struct ReuseState
{
    SubFiltersMap m_subFilters;
};

} // namespace cc_mqtt311_client

// Copyright (C) 2019 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef MBOOT_LOG_H
#define MBOOT_LOG_H

#include <log/logger_support.h>
#include <log/macros.h>
#include <log/log_dbglevels.h>

#include "messages.h"

namespace mboot {

extern isc::log::Logger mboot_logger;

} // end of namespace mboot

#endif

// Copyright (C) 2019 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef MBOOT_MESSAGES_H
#define MBOOT_MESSAGES_H

#include <log/message_types.h>

extern const isc::log::MessageID MBOOT_LOAD;
extern const isc::log::MessageID MBOOT_UNLOAD;
extern const isc::log::MessageID MBOOT_BAD_OPTION;
extern const isc::log::MessageID MBOOT_PACKET_UNPACK_FAILED;
extern const isc::log::MessageID MBOOT_PACKET_PACK_FAILED;
extern const isc::log::MessageID MBOOT_DETECT;
extern const isc::log::MessageID MBOOT_DEVEL;

#endif // MBOOT_MESSAGES_H

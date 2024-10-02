// Copyright (C) 2019 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstddef>
#include <log/message_types.h>
#include <log/message_initializer.h>

extern const isc::log::MessageID MBOOT_LOAD = "MBOOT_LOAD";
extern const isc::log::MessageID MBOOT_UNLOAD = "MBOOT_UNLOAD";
extern const isc::log::MessageID MBOOT_BAD_OPTION = "MBOOT_BAD_OPTION";
extern const isc::log::MessageID MBOOT_PACKET_UNPACK_FAILED = "MBOOT_PACKET_UNPACK_FAILED";
extern const isc::log::MessageID MBOOT_PACKET_PACK_FAILED = "MBOOT_PACKET_PACK_FAILED";
extern const isc::log::MessageID MBOOT_DETECT = "MBOOT_DETECT";
extern const isc::log::MessageID MBOOT_DEVEL = "MBOOT_DEVEL";

namespace {

const char* values[] = {
	"MBOOT_LOAD", "mboot hooks library has been loaded",
	"MBOOT_UNLOAD", "mboot hooks library has been unloaded",
	"MBOOT_BAD_OPTION", "unable to load, missing or invalid parameter: %1",
	"MBOOT_PACKET_UNPACK_FAILED", "failed to parse query from %1 to %2, received over interface %3, reason: %4",
	"MBOOT_PACKET_PACK_FAIL", "%1: preparing on-wire-format of the packet to be sent failed %2",
	"MBOOT_DETECT", "netboot request from %1",
	"MBOOT_DEVEL", "%1",
	NULL
};

const isc::log::MessageInitializer initializer(values);

}

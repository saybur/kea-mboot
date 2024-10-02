// Copyright (C) 2019-2022 Internet Systems Consortium, Inc. ("ISC")
// Copyright (C) 2024 saybur
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <config.h>

#include <hooks/hooks.h>
#include <dhcp/pkt4.h>
#include <process/daemon.h>
#include <stats/stats_mgr.h>

#include <vector>

#include "log.h"

using namespace isc;
using namespace isc::dhcp;
using namespace isc::hooks;
using namespace isc::process;
using namespace mboot;

extern "C" {

static void mboot_extract(Pkt4Ptr resp, isc::util::OutputBuffer& buf,
		uint16_t code) {
	OptionPtr opt = resp->getOption(code);
	if (opt != NULL) {
		opt->pack(buf);
		while (resp->delOption(code));
	}
}

int pkt4_send(CalloutHandle& handle) {
	CalloutHandle::CalloutNextStep status = handle.getStatus();
	if (status == CalloutHandle::NEXT_STEP_DROP) {
		return (0);
	}

	Pkt4Ptr query;
	handle.getArgument("query4", query);
	if (!query->inClass("netboot")) {
		return (0);
	}

	Pkt4Ptr resp;
	handle.getArgument("response4", resp);

	// strip boot options and write (in corrected order) to a buffer
	LOG_INFO(mboot_logger, MBOOT_DEVEL)
			.arg("performing netboot packing");
	isc::util::OutputBuffer mbuf(1024);
	mboot_extract(resp, mbuf, 1);
	mboot_extract(resp, mbuf, 234);
	mboot_extract(resp, mbuf, 235);
	mboot_extract(resp, mbuf, 237);
	mboot_extract(resp, mbuf, 238);
	mboot_extract(resp, mbuf, 28);
	mboot_extract(resp, mbuf, 230);
	mboot_extract(resp, mbuf, 232);
	mboot_extract(resp, mbuf, 233);

	try {
		resp->pack();

		// remove end flag, write boot options, then re-add end
		isc::util::OutputBuffer& pbuf = resp->getBuffer();
		pbuf.trim(1);
		pbuf.writeData(mbuf.getData(), mbuf.getLength());
		pbuf.writeUint8(0xFF);
	} catch (const std::exception& ex) {
		LOG_ERROR(mboot_logger, MBOOT_PACKET_PACK_FAILED)
			.arg(resp->getLabel())
			.arg(ex.what());
	}

	return (0);
}

int load(LibraryHandle& /* handle */) {
	const std::string& proc_name = Daemon::getProcName();
	if (proc_name != "kea-dhcp4") {
		isc_throw(isc::Unexpected, "Bad process name: " << proc_name
				<< ", expected kea-dhcp4");
	}
	LOG_INFO(mboot_logger, MBOOT_LOAD);

	return (0);
}

int unload() {
	LOG_INFO(mboot_logger, MBOOT_UNLOAD);
	return (0);
}

} // end extern "C"

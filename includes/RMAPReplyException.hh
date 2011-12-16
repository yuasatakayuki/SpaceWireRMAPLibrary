/*
 * RMAPReplyException.hh
 *
 *  Created on: Aug 5, 2011
 *      Author: yuasa
 */

#ifndef RMAPREPLYEXCEPTION_HH_
#define RMAPREPLYEXCEPTION_HH_

#include "CxxUtilities/CxxUtilities.hh"

#include "RMAPReplyStatus.hh"
#include <string>

class RMAPReplyException: public CxxUtilities::Exception, public RMAPReplyStatus {
public:
	RMAPReplyException(uint32_t status) :
		CxxUtilities::Exception(status) {
	}

public:
	std::string toString() {
		return RMAPReplyStatus::replyStatusToString(this->status);
	}
};

#endif /* RMAPREPLYEXCEPTION_HH_ */

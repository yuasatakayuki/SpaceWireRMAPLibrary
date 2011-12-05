/*
 * RMAPNode.hh
 *
 *  Created on: Dec 5, 2011
 *      Author: yuasa
 */

#ifndef RMAPNODE_HH_
#define RMAPNODE_HH_

#include <CxxUtilities/CommonHeader.hh>
#include "RMAPMemoryObject.hh"

class RMAPNode {
protected:
	std::string id;
public:
	std::string getID() const {
		return id;
	}

	void setID(std::string id) {
		this->id = id;
	}
};

#endif /* RMAPNODE_HH_ */

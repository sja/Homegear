/*
 * GD.cpp
 *
 *  Created on: May 19, 2013
 *      Author: sathya
 */

#include "GD.h"

std::string GD::startUpPath = "";
HomeMaticDevices GD::devices;
XMLRPC::Server GD::xmlrpcServer;
int32_t GD::debugLevel = 5;

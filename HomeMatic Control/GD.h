#ifndef GD_H_
#define GD_H_

#include <vector>
#include <string>

#include "Database.h"
#include "Cul.h"
#include "HomeMaticDevices.h"
#include "XMLRPC/Server.h"
#include "XMLRPC/Devices.h"

class Cul;
class Database;
class HomeMaticDevices;

class GD {
public:
	static std::string workingDirectory;
	static std::string executablePath;
	static HomeMaticDevices devices;
	static XMLRPC::Server xmlrpcServer;
	static XMLRPC::Devices xmlrpcDevices;
	static Database db;
	static Cul cul;
	static int32_t debugLevel;

	virtual ~GD() { }
private:
	//Non public constructor
	GD();
};

#endif /* GD_H_ */
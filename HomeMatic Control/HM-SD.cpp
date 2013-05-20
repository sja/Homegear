#include "HM-SD.h"

HM_SD::HM_SD() : HomeMaticDevice()
{
}

HM_SD::HM_SD(std::string serializedObject) : HomeMaticDevice(serializedObject.substr(serializedObject.find_first_of("|") + 1))
{
	init();

	serializedObject = serializedObject.substr(0, serializedObject.find_first_of("|"));
	if(GD::debugLevel == 5) cout << "Unserializing: " << serializedObject << endl;

	std::istringstream stringstream(serializedObject);
	std::string entry;
	int32_t i = 0;
	while(std::getline(stringstream, entry, ';'))
	{
		switch(i)
		{
		case 0:
			unserializeFilters(entry);
			break;
		case 1:
			unserializeOverwriteResponses(entry);
			break;
		}
		i++;
	}
}

HM_SD::HM_SD(std::string serialNumber, int32_t address) : HomeMaticDevice(serialNumber, address)
{
	init();
}

HM_SD::~HM_SD()
{
}

void HM_SD::init()
{
	HomeMaticDevice::init();

	_deviceType = HMSD;
}

void HM_SD::unserializeFilters(std::string serializedData)
{
	std::istringstream stringstream(serializedData);
	int32_t i = 0;
	HM_SD_Filter filter;
	std::string entry;
	while(std::getline(stringstream, entry, ','))
	{
		if(i % 2 == 0) filter.filterType = (FilterType)std::stol(entry, 0, 16);
		else
		{
			filter.filterValue = std::stol(entry, 0, 16);
			_filters.push_back(filter);
		}
		i++;
	}
}

void HM_SD::unserializeOverwriteResponses(std::string serializedData)
{
	std::istringstream stringstream(serializedData);
	int32_t i = 0;
	HM_SD_OverwriteResponse overwriteResponse;
	std::string entry;
	while(std::getline(stringstream, entry, ','))
	{
		if(i % 3 == 0) overwriteResponse.packetPartToCapture = entry;
		else if(i % 3 == 1) overwriteResponse.response = entry;
		else
		{
			overwriteResponse.sendAfter = std::stol(entry, 0, 16);
			_responsesToOverwrite.push_back(overwriteResponse);
		}
		i++;
	}
}

std::string HM_SD::serialize()
{
	std::string serializedObject = HomeMaticDevice::serialize();
	std::ostringstream stringstream;
	stringstream << std::hex;
	for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end();)
	{
		stringstream << (int32_t)i->filterType << ",";
		stringstream << i->filterValue;
		++i;
		if(i != _filters.end()) stringstream << ",";
	}
	stringstream << ";";
	for(std::list<HM_SD_OverwriteResponse>::const_iterator i = _responsesToOverwrite.begin(); i != _responsesToOverwrite.end();)
	{
		stringstream << i->packetPartToCapture << ",";
		stringstream << i->response << ",";
		stringstream << i->sendAfter << ",";
		++i;
		if(i != _responsesToOverwrite.end()) stringstream << ",";
	}
	stringstream << "|"; //The "|" seperates the base class from the inherited class part
	stringstream << std::dec;
	return stringstream.str() + serializedObject;
}

void HM_SD::packetReceived(BidCoSPacket* packet)
{
	_receivedPacket = *packet;
    bool printPacket = false;
    for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
    {
        switch(i->filterType)
        {
            case FilterType::SenderAddress:
                if(_receivedPacket.senderAddress() == i->filterValue) printPacket = true;
                break;
            case FilterType::DestinationAddress:
                if(_receivedPacket.destinationAddress() == i->filterValue) printPacket = true;
                break;
            case FilterType::DeviceType:
                //Only possible for paired devices
                break;
            case FilterType::MessageType:
                if(_receivedPacket.messageType() == i->filterValue) printPacket = true;
                break;
        }
    }
    if(_filters.size() == 0) printPacket = true;
    std::chrono::time_point<std::chrono::system_clock> timepoint = std::chrono::system_clock::now();
    if(printPacket) cout << std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count() << " Received: " << _receivedPacket.hexString() << '\n';
    for(std::list<HM_SD_OverwriteResponse>::const_iterator i = _responsesToOverwrite.begin(); i != _responsesToOverwrite.end(); ++i)
    {
        std::string packetHex = _receivedPacket.hexString();
        if(packetHex.find(i->packetPartToCapture) != std::string::npos)
        {
            std::chrono::milliseconds sleepingTime(i->sendAfter); //Don't respond too fast
            std::this_thread::sleep_for(sleepingTime);
            BidCoSPacket packet;
            std::stringstream stringstream;
            stringstream << std::hex << std::setw(2) << (i->response.size() / 2) + 1;
            std::string lengthHex = stringstream.str();
            packet.import("A" + lengthHex + packetHex.substr(2, 2) + i->response + "\r\n");
            std::chrono::time_point<std::chrono::system_clock> timepoint = std::chrono::system_clock::now();
            cout << std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count() << " Overwriting response: " << '\n';
            GD::cul.sendPacket(packet);
        }
    }
}

void HM_SD::addFilter(FilterType filterType, int32_t filterValue)
{
    HM_SD_Filter filter;
    filter.filterType = filterType;
    filter.filterValue = filterValue;
    _filters.push_back(filter);
}

void HM_SD::removeFilter(FilterType filterType, int32_t filterValue)
{
    _filters.remove_if([&](HM_SD_Filter filter){ return filter.filterType == filterType && filter.filterValue == filterValue; });
}

void HM_SD::addOverwriteResponse(std::string packetPartToCapture, std::string response, int32_t sendAfter)
{
    HM_SD_OverwriteResponse overwriteResponse;
    overwriteResponse.sendAfter = sendAfter;
    overwriteResponse.packetPartToCapture = packetPartToCapture;
    overwriteResponse.response = response;
    _responsesToOverwrite.push_back(overwriteResponse);
}

void HM_SD::removeOverwriteResponse(std::string packetPartToCapture)
{
    _responsesToOverwrite.remove_if([&](HM_SD_OverwriteResponse entry){ return entry.packetPartToCapture == packetPartToCapture; });
}

void HM_SD::handleCLICommand(std::string command)
{
	if(command == "add filter")
	{
		std::string input;
		cout << "Enter filter type or press \"l\" to list filter types: ";
		do
		{
			cin >> input;
			if(input == "l")
			{
				cout << "Filter types:" << endl;
				cout << "\t00: Sender address" << endl;
				cout << "\t01: Destination address" << endl;
				cout << "\t03: Message type" << endl << endl;
				cout << "Enter filter type or press \"l\" to list filter types: ";
			}
		}while(input == "l");
		int32_t filterType = -1;
		try	{ filterType = std::stol(input, 0, 16); } catch(...) {}
		if(filterType < 0 || filterType > 3) cout << "Invalid filter type." << endl;
		else
		{
			cout << "Please enter a filter value in hexadecimal format: ";
			HM_SD_Filter filter;
			filter.filterType = (FilterType)filterType;
			filter.filterValue = getHexInput();
			_filters.push_back(filter);
			cout << "Filter added." << endl;
		}
	}
	else if(command == "remove filter")
	{
		std::string input;
		cout << "Enter filter type or press \"l\" to list filter types: ";
		while(input == "l")
		{
			cin >> input;
			cout << "Filter types:" << endl;
			cout << "\t00: Sender address" << endl;
			cout << "\t01: Destination address" << endl;
			cout << "\t03: Message type" << endl << endl;
			cout << "Enter filter type or press \"l\" to list filter types: ";
		}
		int32_t filterType = getHexInput();
		if(filterType < 0 || filterType > 3) cout << "Invalid filter type." << endl;
		else
		{
			cout << "Please enter a filter value in hexadecimal format: ";
			uint32_t oldSize = _filters.size();
			removeFilter((FilterType)filterType, getHexInput());
			if(_filters.size() != oldSize) cout << "Filter removed." << endl;
			else cout << "Filter not found." << endl;
		}
	}
	else if(command == "list filters")
	{
		for(std::list<HM_SD_Filter>::const_iterator i = _filters.begin(); i != _filters.end(); ++i)
		{
			cout << "Filter type: " << std::hex << (int32_t)i->filterType << "\tFilter value: " << i->filterValue << std::dec << endl;
		}
	}
}

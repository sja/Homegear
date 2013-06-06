/*
 * HomeMaticDevices.cpp
 *
 *  Created on: May 20, 2013
 *      Author: sathya
 */

#include "HomeMaticDevices.h"
#include "GD.h"
#include "Devices/HM-CC-VD.h"
#include "Devices/HM-CC-TC.h"
#include "Devices/HM-SD.h"

HomeMaticDevices::HomeMaticDevices()
{
}

HomeMaticDevices::~HomeMaticDevices()
{
}

void HomeMaticDevices::load()
{
	GD::db.executeCommand("CREATE TABLE IF NOT EXISTS devices (address INTEGER, deviceTYPE INTEGER, serializedObject TEXT, dutyCycleMessageCounter INTEGER, lastDutyCycle INTEGER)");
	std::vector<std::vector<DataColumn>> rows = GD::db.executeCommand("SELECT * FROM devices");
	for(std::vector<std::vector<DataColumn> >::iterator row = rows.begin(); row != rows.end(); ++row)
	{
		std::shared_ptr<HomeMaticDevice> device;
		HMDeviceTypes deviceType;
		std::string serializedObject;
		uint8_t dutyCycleMessageCounter = 0;
		for(std::vector<DataColumn>::iterator col = row->begin(); col != row->end(); ++col)
		{
			if(col->index == 1)
			{
				deviceType = (HMDeviceTypes)col->intValue;
			}
			else if(col->index == 2)
			{
				serializedObject = col->textValue;
			}
			else if(col->index == 3)
			{
				dutyCycleMessageCounter = col->intValue;
			}
			else if(col->index == 4)
			{
				switch(deviceType)
				{
				case HMDeviceTypes::HMCCTC:
					device = std::shared_ptr<HomeMaticDevice>(new HM_CC_TC());
					break;
				case HMDeviceTypes::HMCCVD:
					device = std::shared_ptr<HomeMaticDevice>(new HM_CC_VD());
					break;
				case HMDeviceTypes::HMCENTRAL:
					_central = std::shared_ptr<HomeMaticCentral>(new HomeMaticCentral());
					device = _central;
					break;
				case HMDeviceTypes::HMSD:
					device = std::shared_ptr<HomeMaticDevice>(new HM_SD());
					break;
				default:
					break;
				}
				if(device != nullptr)
				{
					device->unserialize(serializedObject, dutyCycleMessageCounter, col->intValue);
					_devices.push_back(device);
					device = nullptr;
				}
			}
		}
	}
}

void HomeMaticDevices::stopDutyCycles()
{
	for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		std::thread stop(&HomeMaticDevices::stopDutyCycle, this, (*i));
		stop.detach();
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(8000));
}

void HomeMaticDevices::stopDutyCycle(std::shared_ptr<HomeMaticDevice> device)
{
	device->stopDutyCycle();
}

void HomeMaticDevices::save()
{
	try
	{
		std::cout << "Waiting for duty cycles to stop..." << std::endl;
		//The stopping is necessary, because there is a small time gap between setting "_lastDutyCycleEvent" and the duty cycle message counter.
		//If saving takes place within this gap, the paired duty cycle devices are out of sync after restart of this program.
		stopDutyCycles();
		for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			std::ostringstream command;
			command << "DELETE FROM devices WHERE address=" << std::dec << (*i)->address();
			GD::db.executeCommand(command.str());
			std::ostringstream command2;
			command2 << "INSERT INTO devices VALUES(" << (*i)->address() << "," << (uint32_t)(*i)->deviceType() << ",'" << (*i)->serialize() << "'," << (int32_t)(*i)->messageCounter()->at(1) << "," << (*i)->lastDutyCycleEvent() << ")";
			GD::db.executeCommand(command2.str());
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
}

void HomeMaticDevices::add(HomeMaticDevice* device)
{
	if(device == nullptr) return;
	std::ostringstream command;
	command << "INSERT INTO devices VALUES(" << std::dec << device->address() << "," << (uint32_t)device->deviceType() << ",'" << device->serialize() << "'," << (int32_t)device->messageCounter()->at(1) << ","  << device->lastDutyCycleEvent() << ")";
	GD::db.executeCommand(command.str());
	if(device->deviceType() == HMDeviceTypes::HMCENTRAL)
	{
		_central = std::shared_ptr<HomeMaticCentral>((HomeMaticCentral*)device);
		_devices.push_back(_central);
	}
	else
	{
		_devices.push_back(std::shared_ptr<HomeMaticDevice>(device));
	}
}

bool HomeMaticDevices::remove(int32_t address)
{
	for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		if((*i)->address() == address)
		{
			if(GD::debugLevel >= 5) std::cout << "Removing device pointer from device array..." << std::endl;
			_devices.erase(i);

			if(GD::debugLevel >= 5) std::cout << "Deleting database entry..." << std::endl;
			std::ostringstream command;
			command << "DELETE FROM devices WHERE address=" << std::dec << address;
			GD::db.executeCommand(command.str());
			return true;
		}
	}
	return false;
}

std::shared_ptr<HomeMaticDevice> HomeMaticDevices::get(int32_t address)
{
	for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		if((*i)->address() == address)
		{
			return (*i);
		}
	}
	return nullptr;
}

std::shared_ptr<HomeMaticDevice> HomeMaticDevices::get(std::string serialNumber)
{
	for(std::vector<std::shared_ptr<HomeMaticDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		if((*i)->serialNumber() == serialNumber)
		{
			return (*i);
		}
	}
	return nullptr;
}

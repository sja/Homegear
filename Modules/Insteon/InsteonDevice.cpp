/* Copyright 2013-2014 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "InsteonDevice.h"
#include "GD.h"

namespace Insteon
{

InsteonDevice::InsteonDevice(IDeviceEventSink* eventHandler) : LogicalDevice(eventHandler)
{
}

InsteonDevice::InsteonDevice(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : LogicalDevice(deviceID, serialNumber, address, eventHandler)
{
}

InsteonDevice::~InsteonDevice()
{
}

void InsteonDevice::init()
{
	try
	{
		if(_initialized) return; //Prevent running init two times

		GD::physicalDevice->addLogicalDevice(this);

		_initialized = true;
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool InsteonDevice::packetReceived(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(_disposing) return false;

	}
	catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

bool InsteonDevice::isCentral()
{
	return _deviceType == (uint32_t)DeviceType::INSTEONCENTRAL;
}

void InsteonDevice::saveVariable(uint32_t index, int64_t intValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		BaseLib::DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(intValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_variableDatabaseIDs[index])));
			BaseLib::Obj::ins->db.executeWriteCommand("UPDATE deviceVariables SET integerValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(index)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(intValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			int32_t result = BaseLib::Obj::ins->db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void InsteonDevice::saveVariable(uint32_t index, std::string& stringValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		BaseLib::DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(stringValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_variableDatabaseIDs[index])));
			BaseLib::Obj::ins->db.executeWriteCommand("UPDATE deviceVariables SET stringValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(index)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(stringValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			int32_t result = BaseLib::Obj::ins->db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void InsteonDevice::saveVariable(uint32_t index, std::vector<uint8_t>& binaryValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		BaseLib::DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(binaryValue)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_variableDatabaseIDs[index])));
			BaseLib::Obj::ins->db.executeWriteCommand("UPDATE deviceVariables SET binaryValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(index)));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn()));
			data.push_back(std::shared_ptr<BaseLib::DataColumn>(new BaseLib::DataColumn(binaryValue)));
			int32_t result = BaseLib::Obj::ins->db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void InsteonDevice::loadVariables()
{
	try
	{
		_databaseMutex.lock();
		BaseLib::DataTable rows = BaseLib::Obj::ins->db.executeCommand("SELECT * FROM deviceVariables WHERE deviceID=" + std::to_string(_deviceID));
		for(BaseLib::DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			/*switch(row->second.at(2)->intValue)
			{
			case 0:
				_firmwareVersion = row->second.at(3)->intValue;
				break;
			case 1:
				_centralAddress = row->second.at(3)->intValue;
				break;
			case 2:
				unserializeMessageCounters(row->second.at(5)->binaryValue);
				break;
			}*/
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	_databaseMutex.unlock();
}

void InsteonDevice::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;
		//saveVariable(0, _firmwareVersion);
		//saveVariable(1, _centralAddress);
		//saveMessageCounters(); //2
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void InsteonDevice::sendPacket(std::shared_ptr<InsteonPacket> packet)
{
	try
	{
		GD::physicalDevice->sendPacket(packet);
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
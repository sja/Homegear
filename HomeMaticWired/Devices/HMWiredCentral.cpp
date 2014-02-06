/* Copyright 2013 Sathya Laufer
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

#include "HMWiredCentral.h"
#include "../../GD.h"
#include "../../HelperFunctions.h"
#include "../../Version.h"

namespace HMWired {

HMWiredCentral::HMWiredCentral()
{
	init();
}

HMWiredCentral::HMWiredCentral(uint32_t deviceID, std::string serialNumber, int32_t address) : HMWiredDevice(deviceID, serialNumber, address)
{
	init();
}

HMWiredCentral::~HMWiredCentral()
{

}

void HMWiredCentral::init()
{
	try
	{
		HMWiredDevice::init();

		_deviceType = GD::deviceTypes.get(DeviceID::HMWIREDCENTRAL);
	}
	catch(const std::exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

std::string HMWiredCentral::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;
		/*if(_currentPeer)
		{
			if(command == "unselect")
			{
				_currentPeer.reset();
				return "Peer unselected.\n";
			}
			if(!_currentPeer) return "No peer selected.\n";
			return _currentPeer->handleCLICommand(command);
		}*/
		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "unselect\t\tUnselect this device" << std::endl;
			stringStream << "search\t\tSearches for new devices on the bus" << std::endl;
			stringStream << "peers list\t\tList all peers" << std::endl;
			stringStream << "peers add\t\tManually adds a peer (without pairing it! Only for testing)" << std::endl;
			stringStream << "peers remove\t\tRemove a peer (without unpairing)" << std::endl;
			stringStream << "peers unpair\t\tUnpair a peer" << std::endl;
			stringStream << "peers select\t\tSelect a peer" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 6, "search") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command searches for new devices on the bus." << std::endl;
						stringStream << "Usage: search" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			std::shared_ptr<RPC::RPCVariable> result = searchDevices();
			if(result->errorStruct) stringStream << "Error: " << result->structValue->at("faultString")->stringValue << std::endl;
			else stringStream << "Search completed successfully." << std::endl;
			return stringStream.str();
		}
		/*else if(command.compare(0, 9, "peers add") == 0)
		{
			uint32_t deviceType;
			int32_t peerAddress = 0;
			std::string serialNumber;
			int32_t firmwareVersion = 0;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					int32_t temp = HelperFunctions::getNumber(element, true);
					if(temp == 0) return "Invalid device type. Device type has to be provided in hexadecimal format.\n";
					deviceType = temp;
				}
				else if(index == 3)
				{
					peerAddress = HelperFunctions::getNumber(element, true);
					if(peerAddress == 0 || peerAddress != (peerAddress & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				else if(index == 4)
				{
					if(element.length() != 10) return "Invalid serial number. Please provide a serial number with a length of 10 characters.\n";
					serialNumber = element;
				}
				else if(index == 5)
				{
					firmwareVersion = HelperFunctions::getNumber(element, true);
					if(firmwareVersion == 0) return "Invalid firmware version. The firmware version has to be passed in hexadecimal format.\n";
				}
				index++;
			}
			if(index < 6)
			{
				stringStream << "Description: This command manually adds a peer without pairing. Please only use this command for testing." << std::endl;
				stringStream << "Usage: peers add DEVICETYPE ADDRESS SERIALNUMBER FIRMWAREVERSION" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  DEVICETYPE:\t\tThe 2 byte device type of the peer to add in hexadecimal format. Example: 0039" << std::endl;
				stringStream << "  ADDRESS:\t\tThe 3 byte address of the peer to add in hexadecimal format. Example: 1A03FC" << std::endl;
				stringStream << "  SERIALNUMBER:\t\tThe 10 character long serial number of the peer to add. Example: JEQ0123456" << std::endl;
				stringStream << "  FIRMWAREVERSION:\tThe 1 byte firmware version of the peer to add in hexadecimal format. Example: 1F" << std::endl;
				return stringStream.str();
			}
			if(peerExists(peerAddress)) stringStream << "This peer is already paired to this central." << std::endl;
			else
			{
				std::shared_ptr<Peer> peer = createPeer(peerAddress, firmwareVersion, GD::deviceTypes.get(DeviceFamily::HomeMaticBidCoS, deviceType), serialNumber, 0, 0, std::shared_ptr<BidCoSPacket>(), false);
				if(!peer || !peer->rpcDevice) return "Device type not supported.\n";

				try
				{
					_peersMutex.lock();
					_peers[peer->getAddress()] = peer;
					if(!peer->getSerialNumber().empty()) _peersBySerial[peer->getSerialNumber()] = peer;
					_peersMutex.unlock();
					peer->save(true, true, false);
					peer->initializeCentralConfig();
				}
				catch(const std::exception& ex)
				{
					_peersMutex.unlock();
					HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				}
				catch(Exception& ex)
				{
					_peersMutex.unlock();
					HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				}
				catch(...)
				{
					_peersMutex.unlock();
					HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
				}

				stringStream << "Added peer 0x" << std::hex << peerAddress << " of type 0x" << (int32_t)deviceType << " with serial number " << serialNumber << " and firmware version 0x" << firmwareVersion << "." << std::dec << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 12, "peers remove") == 0)
		{
			int32_t peerAddress;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					peerAddress = HelperFunctions::getNumber(element, true);
					if(peerAddress == 0 || peerAddress != (peerAddress & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command removes a peer without trying to unpair it first." << std::endl;
				stringStream << "Usage: peers remove ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe 3 byte address of the peer to remove in hexadecimal format. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			if(!peerExists(peerAddress)) stringStream << "This device is not paired to this central." << std::endl;
			else
			{
				if(_currentPeer && _currentPeer->getAddress() == peerAddress) _currentPeer.reset();
				deletePeer(peerAddress);
				stringStream << "Removed device 0x" << std::hex << peerAddress << "." << std::dec << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 12, "peers unpair") == 0)
		{
			int32_t peerAddress;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					peerAddress = HelperFunctions::getNumber(element, true);
					if(peerAddress == 0 || peerAddress != (peerAddress & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command unpairs a peer." << std::endl;
				stringStream << "Usage: peers unpair ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe 3 byte address of the peer to unpair in hexadecimal format. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			if(!peerExists(peerAddress)) stringStream << "This device is not paired to this central." << std::endl;
			else
			{
				if(_currentPeer && _currentPeer->getAddress() == peerAddress) _currentPeer.reset();
				stringStream << "Unpairing device 0x" << std::hex << peerAddress << std::dec << std::endl;
				unpair(peerAddress, true);
			}
			return stringStream.str();
		}
		else if(command == "enable aes")
		{
			std::string input;
			stringStream << "Please enter the devices address: ";
			int32_t address = getHexInput();
			if(!peerExists(address)) stringStream << "This device is not paired to this central." << std::endl;
			else
			{
				stringStream << "Enabling AES for device device 0x" << std::hex << address << std::dec << std::endl;
				sendEnableAES(address, 1);
			}
			return stringStream.str();
		}
		else if(command.compare(0, 10, "peers list") == 0)
		{
			try
			{
				std::string filterType;
				std::string filterValue;

				std::stringstream stream(command);
				std::string element;
				int32_t index = 0;
				while(std::getline(stream, element, ' '))
				{
					if(index < 2)
					{
						index++;
						continue;
					}
					else if(index == 2)
					{
						if(element == "help")
						{
							index = -1;
							break;
						}
						filterType = HelperFunctions::toLower(element);
					}
					else if(index == 3) filterValue = element;
					index++;
				}
				if(index == -1)
				{
					stringStream << "Description: This command unpairs a peer." << std::endl;
					stringStream << "Usage: peers list [FILTERTYPE] [FILTERVALUE]" << std::endl << std::endl;
					stringStream << "Parameters:" << std::endl;
					stringStream << "  FILTERTYPE:\tSee filter types below." << std::endl;
					stringStream << "  FILTERVALUE:\tDepends on the filter type. If a number is required, it has to be in hexadecimal format." << std::endl << std::endl;
					stringStream << "Filter types:" << std::endl;
					stringStream << "  ADDRESS: Filter by address." << std::endl;
					stringStream << "      FILTERVALUE: The 3 byte address of the peer to filter (e. g. 1DA44D)." << std::endl;
					stringStream << "  SERIAL: Filter by serial number." << std::endl;
					stringStream << "      FILTERVALUE: The serial number of the peer to filter (e. g. JEQ0554309)." << std::endl;
					stringStream << "  TYPE: Filter by device type." << std::endl;
					stringStream << "      FILTERVALUE: The 2 byte device type in hexadecimal format." << std::endl;
					stringStream << "  CONFIGPENDING: List peers with pending config." << std::endl;
					stringStream << "      FILTERVALUE: empty" << std::endl;
					stringStream << "  UNREACH: List all unreachable peers." << std::endl;
					stringStream << "      FILTERVALUE: empty" << std::endl;
					return stringStream.str();
				}

				if(_peers.empty())
				{
					stringStream << "No peers are paired to this central." << std::endl;
					return stringStream.str();
				}
				_peersMutex.lock();
				for(std::unordered_map<std::string, std::shared_ptr<Peer>>::iterator i = _peersBySerial.begin(); i != _peersBySerial.end(); ++i)
				{
					if(filterType == "address")
					{
						int32_t address = HelperFunctions::getNumber(filterValue, true);
						if(i->second->getAddress() != address) continue;
					}
					else if(filterType == "serial")
					{
						if(i->second->getSerialNumber() != filterValue) continue;
					}
					else if(filterType == "type")
					{
						int32_t deviceType = HelperFunctions::getNumber(filterValue, true);
						if((int32_t)i->second->getDeviceType().type() != deviceType) continue;
					}
					else if(filterType == "configpending")
					{
						if(i->second->serviceMessages)
						{
							if(!i->second->serviceMessages->getConfigPending()) continue;
						}
					}
					else if(filterType == "unreach")
					{
						if(i->second->serviceMessages)
						{
							if(!i->second->serviceMessages->getUnreach()) continue;
						}
					}

					stringStream << "Address: 0x" << std::hex << i->second->getAddress() << "\tSerial number: " << i->second->getSerialNumber() << "\tDevice type: 0x" << std::setfill('0') << std::setw(4) << (int32_t)i->second->getDeviceType().type();
					if(i->second->rpcDevice)
					{
						std::shared_ptr<RPC::DeviceType> type = i->second->rpcDevice->getType(i->second->getDeviceType(), i->second->getFirmwareVersion());
						std::string typeID;
						if(type) typeID = (" (" + type->id + ")");
						typeID.resize(25, ' ');
						stringStream << typeID;
					}
					if(i->second->serviceMessages)
					{
						std::string configPending(i->second->serviceMessages->getConfigPending() ? "Yes" : "No");
						std::string unreachable(i->second->serviceMessages->getUnreach() ? "Yes" : "No");
						stringStream << "\tConfig pending: " << configPending << "\tUnreachable: " << unreachable;
					}
					stringStream << std::endl << std::dec;
				}
				_peersMutex.unlock();
				return stringStream.str();
			}
			catch(const std::exception& ex)
			{
				_peersMutex.unlock();
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(Exception& ex)
			{
				_peersMutex.unlock();
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_peersMutex.unlock();
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
		else if(command.compare(0, 12, "peers select") == 0)
		{
			int32_t address;

			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help") break;
					address = HelperFunctions::getNumber(element, true);
					if(address == 0 || address != (address & 0xFFFFFF)) return "Invalid address. Address has to be provided in hexadecimal format and with a maximum size of 3 bytes. A value of \"0\" is not allowed.\n";
				}
				index++;
			}
			if(index == 2)
			{
				stringStream << "Description: This command selects a peer." << std::endl;
				stringStream << "Usage: peers select ADDRESS" << std::endl << std::endl;
				stringStream << "Parameters:" << std::endl;
				stringStream << "  ADDRESS:\tThe 3 byte address of the peer to select in hexadecimal format. Example: 1A03FC" << std::endl;
				return stringStream.str();
			}

			_currentPeer = getPeer(address);
			if(!_currentPeer) stringStream << "This peer is not paired to this central." << std::endl;
			else
			{
				stringStream << "Peer with address 0x" << std::hex << address << " and device type 0x" << (int32_t)_currentPeer->getDeviceType().type() << " selected." << std::dec << std::endl;
				stringStream << "For information about the peer's commands type: \"help\"" << std::endl;
			}
			return stringStream.str();
		}*/
		else return "Unknown command.\n";
	}
	catch(const std::exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

std::shared_ptr<RPC::RPCVariable> HMWiredCentral::searchDevices()
{
	try
	{
		lockBus();

		int32_t addressMask = 0;


		unlockBus();
		//Todo: Return number of new devices and call "newDevices"
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(5));
	}
	catch(const std::exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	unlockBus();
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

} /* namespace HMWired */

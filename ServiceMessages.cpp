#include "ServiceMessages.h"
#include "GD.h"
#include "HelperFunctions.h"

ServiceMessages::ServiceMessages(Peer* peer, std::string serializedObject) : ServiceMessages(peer)
{
	try
	{
		if(serializedObject.empty()) return;
		HelperFunctions::printDebug("Unserializing service message: " + serializedObject);

		std::istringstream stringstream(serializedObject);
		uint32_t pos = 0;
		_unreach = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
		_stickyUnreach = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
		_configPending = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
		_lowbat = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
		int32_t errorSize = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
		for(uint32_t i = 0; i < errorSize; i++)
		{
			int32_t channel = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
			_errors[channel] = std::stoll(serializedObject.substr(pos, 2), 0, 16); pos += 2;
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ServiceMessages::stopThreads()
{
	try
	{
		if(_setConfigPendingThread.joinable()) _setConfigPendingThread.join();
		if(_setUnreachThread.joinable()) _setUnreachThread.join();
		if(_checkUnreachThread.joinable()) _checkUnreachThread.join();
		if(_endUnreachThread.joinable()) _endUnreachThread.join();
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string ServiceMessages::serialize()
{
	try
	{
		std::ostringstream stringstream;
		stringstream << std::hex << std::uppercase << std::setfill('0');
		stringstream << std::setw(1) << (int32_t)_unreach;
		stringstream << std::setw(1) << (int32_t)_stickyUnreach;
		stringstream << std::setw(1) << (int32_t)_configPending;
		stringstream << std::setw(1) << (int32_t)_lowbat;
		stringstream << std::setw(2) << (int32_t)_errors.size();
		for(std::map<uint32_t, uint8_t>::const_iterator i = _errors.begin(); i != _errors.end(); ++i)
		{
			stringstream << std::setw(2) << i->first;
			stringstream << std::setw(2) << (int32_t)i->second;
		}
		stringstream << std::dec;
		return stringstream.str();
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
}

bool ServiceMessages::set(std::string id, bool value)
{
	try
	{
		if(id == "UNREACH" && value != _unreach) _unreach = value;
		else if(id == "STICKY_UNREACH" && value != _stickyUnreach) _stickyUnreach = value;
		else if(id == "CONFIG_PENDING" && value != _configPending) _configPending = value;
		else if(id == "LOWBAT" && value != _lowbat) _lowbat = value;
		else return false;

		if(_peer->valuesCentral.at(0).find(id) != _peer->valuesCentral.at(0).end())
		{
			_peer->valuesCentral.at(0).at(id).data.at(0) = (uint8_t)value;

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({id}));
			std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
			rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value)));

			GD::rpcClient.broadcastEvent(_peer->getSerialNumber() + ":0", valueKeys, rpcValues);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return true;
}

void ServiceMessages::set(std::string id, uint8_t value, uint32_t channel)
{
	try
	{
		if(id == "ERROR")
		{
			if(_errors.find(channel) != _errors.end() && value == 0) _errors.erase(channel);
			if(value > 0) _errors[channel] = value;
		}

		//RPC Broadcast is done in peer's packetReceived
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<RPC::RPCVariable> ServiceMessages::get()
{
	try
	{
		std::shared_ptr<RPC::RPCVariable> serviceMessages(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		std::shared_ptr<RPC::RPCVariable> array;
		if(_unreach)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peer->getSerialNumber() + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("UNREACH"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(_stickyUnreach)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peer->getSerialNumber() + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("STICKY_UNREACH"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(_configPending)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peer->getSerialNumber() + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("CONFIG_PENDING"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(_lowbat)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peer->getSerialNumber() + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("LOWBAT"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		for(std::map<uint32_t, uint8_t>::const_iterator i = _errors.begin(); i != _errors.end(); ++i)
		{
			if(i->second == 0) continue;
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peer->getSerialNumber() + ":" + std::to_string(i->first))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("ERROR"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)i->second)));
			serviceMessages->arrayValue->push_back(array);
		}
		return serviceMessages;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

void ServiceMessages::checkUnreach()
{
	try
	{
		if(_checkUnreachThread.joinable()) _checkUnreachThread.join();
		_checkUnreachThread = std::thread(&ServiceMessages::checkUnreachThread, this);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ServiceMessages::checkUnreachThread()
{
	try
	{
		if(!_peer) return;
		uint32_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if(_peer->rpcDevice && _peer->rpcDevice->cyclicTimeout > 0 && (time - _peer->getLastPacketReceived()) > _peer->rpcDevice->cyclicTimeout && !_unreach)
		{
			_unreach = true;
			_stickyUnreach = true;

			if(_peer->valuesCentral.at(0).find("UNREACH") != _peer->valuesCentral.at(0).end())
			{
				_peer->valuesCentral.at(0).at("UNREACH").data.at(0) = 1;
			}

			if(_peer->valuesCentral.at(0).find("STICKY_UNREACH") != _peer->valuesCentral.at(0).end())
			{
				_peer->valuesCentral.at(0).at("STICKY_UNREACH").data.at(0) = 1;
			}

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({"UNREACH", "STICKY_UNREACH"}));
			std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
			rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));

			GD::rpcClient.broadcastEvent(_peer->getSerialNumber() + ":0", valueKeys, rpcValues);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ServiceMessages::endUnreach()
{
	try
	{
		if(_endUnreachThread.joinable()) _endUnreachThread.join();
		_endUnreachThread = std::thread(&ServiceMessages::endUnreachThread, this);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ServiceMessages::endUnreachThread()
{
	try
	{
		if(_unreach == true)
		{
			_unreach = false;
			_unreachResendCounter = 0;

			if(_peer->valuesCentral.find(0) != _peer->valuesCentral.end() && _peer->valuesCentral.at(0).find("UNREACH") != _peer->valuesCentral.at(0).end())
			{
				_peer->valuesCentral.at(0).at("UNREACH").data.at(0) = 0;

				std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({"UNREACH"}));
				std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
				rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(false)));

				GD::rpcClient.broadcastEvent(_peer->getSerialNumber() + ":0", valueKeys, rpcValues);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ServiceMessages::setConfigPending(bool value)
{
	try
	{
		if(_setConfigPendingThread.joinable()) _setConfigPendingThread.join();
		_setConfigPendingThread = std::thread(&ServiceMessages::setConfigPendingThread, this, value);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ServiceMessages::setConfigPendingThread(bool value)
{
	try
	{
		if(!_peer) return;
		if(value != _configPending)
		{
			_configPending = value;
			//Sleep for two seconds before setting values. This is necessary so HomeMatic Config does not think,
			//the paremeters weren't received.
			if(_configPending)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				if(!_configPending) return; //Was changed during sleeping
			}

			if(_peer && _peer->valuesCentral.at(0).find("CONFIG_PENDING") != _peer->valuesCentral.at(0).end())
			{
				_peer->valuesCentral.at(0).at("CONFIG_PENDING").data.at(0) = (uint8_t)value;

				std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({"CONFIG_PENDING"}));
				std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
				rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value)));

				GD::rpcClient.broadcastEvent(_peer->getSerialNumber() + ":0", valueKeys, rpcValues);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ServiceMessages::setUnreach(bool value)
{
	try
	{
		if(_setUnreachThread.joinable()) _setUnreachThread.join();
		_setUnreachThread = std::thread(&ServiceMessages::setUnreachThread, this, value);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void ServiceMessages::setUnreachThread(bool value)
{
	try
	{
		if(!_peer) return;
		if(value != _unreach)
		{
			if(value == true && _unreachResendCounter < 3 && _peer->rpcDevice && ((_peer->rpcDevice->rxModes & RPC::Device::RXModes::Enum::always) || (_peer->rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst)) && !_peer->pendingBidCoSQueues->empty())
			{
				std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
				if(central)
				{
					HelperFunctions::printInfo("Info: Queue is not finished (device: 0x" + HelperFunctions::getHexString(_peer->address) + "). Retrying (" + std::to_string(_unreachResendCounter) + ")...");
					central->enqueuePendingQueues(_peer->address);
					_unreachResendCounter++;
					return;
				}
			}
			_unreachResendCounter = 0;
			_unreach = value;

			if(value) HelperFunctions::printInfo("Info: Device 0x" + HelperFunctions::getHexString(_peer->address) + " is unreachable.");
			if(_peer->valuesCentral.at(0).find("UNREACH") != _peer->valuesCentral.at(0).end())
			{
				_peer->valuesCentral.at(0).at("UNREACH").data.at(0) = (uint8_t)value;

				std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({"UNREACH"}));
				std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
				rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value)));

				if(_unreach && _peer->valuesCentral.at(0).find("STICKY_UNREACH") != _peer->valuesCentral.at(0).end())
				{
					_peer->valuesCentral.at(0).at("STICKY_UNREACH").data.at(0) = (uint8_t)value;

					valueKeys->push_back("STICKY_UNREACH");
					rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
				}

				GD::rpcClient.broadcastEvent(_peer->getSerialNumber() + ":0", valueKeys, rpcValues);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

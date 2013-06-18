#include "Client.h"
#include "../GD.h"

namespace RPC
{
void Client::broadcastEvent(std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> values)
{
	if(!valueKeys || !values || valueKeys->size() != values->size()) return;
	std::string methodName("event"); //We can't just create the methods RPCVariable with new RPCVariable("methodName", "event") because "event" is not a string object. That's way we create the string object here.
	for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
	{
		std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters(new std::list<std::shared_ptr<RPCVariable>>());
		std::shared_ptr<RPCVariable> array(new RPCVariable(RPCVariableType::rpcArray));
		std::shared_ptr<RPCVariable> method;
		for(uint32_t i = 0; i < valueKeys->size(); i++)
		{
			method.reset(new RPCVariable(RPCVariableType::rpcStruct));
			array->arrayValue->push_back(method);
			method->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("methodName", methodName)));
			std::shared_ptr<RPCVariable> params(new RPCVariable(RPCVariableType::rpcArray));
			method->structValue->push_back(params);
			params->name = "params";
			params->arrayValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable((*server)->id)));
			params->arrayValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable(deviceAddress)));
			params->arrayValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable(valueKeys->at(i))));
			params->arrayValue->push_back(values->at(i));
		}
		parameters->push_back(array);
		//Sadly some clients only support multicall and not "event" directly for single events. That's why we use multicall even if there is only one value.
		std::thread t(&RPCClient::invokeBroadcast, &_client, (*server)->address.first, (*server)->address.second, "system.multicall", parameters);
		t.detach();
	}
}

void Client::listDevices(std::pair<std::string, std::string> address)
{
	std::shared_ptr<RemoteRPCServer> server = getServer(address);
	if(!server) return;
	std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters(new std::list<std::shared_ptr<RPCVariable>> { std::shared_ptr<RPCVariable>(new RPCVariable(server->id)) });
	std::shared_ptr<RPCVariable> result = _client.invoke(address.first, address.second, "listDevices", parameters);
	if(result->errorStruct)
	{
		if(GD::debugLevel >= 2)
		{
			std::cout << "Error calling XML RPC method listDevices on server " << address.first << " with port " << address.second << ". Error struct: ";
			result->print();
		}
		return;
	}
	if(result->type != RPCVariableType::rpcArray) return;
	server->knownDevices->clear();
	for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = result->arrayValue->begin(); i != result->arrayValue->end(); ++i)
	{
		if((*i)->type == RPCVariableType::rpcStruct)
		{
			std::pair<std::string, int32_t> device;
			for(std::vector<std::shared_ptr<RPCVariable>>::iterator j = (*i)->structValue->begin(); j != (*i)->structValue->end(); ++j)
			{
				if((*j)->name == "ADDRESS")
				{
					device.first = (*j)->stringValue;
					if(device.first.empty()) break;
				}
				else if((*j)->name == "VERSION")
				{
					device.second = (*j)->integerValue;
				}
			}
			server->knownDevices->insert(device);
		}
	}
}

void Client::sendUnknownDevices(std::pair<std::string, std::string> address)
{
	std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
	if(!central)
	{
		if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method sendUnknownDevices. Please add a central device." << std::endl;
	}
	std::shared_ptr<RemoteRPCServer> server = getServer(address);
	if(!server) return;
	std::shared_ptr<RPCVariable> devices = GD::devices.getCentral()->listDevices(server->knownDevices);
	std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters(new std::list<std::shared_ptr<RPCVariable>>{ std::shared_ptr<RPCVariable>(new RPCVariable(server->id)), devices });
	std::shared_ptr<RPCVariable> result = _client.invoke(address.first, address.second, "newDevices", parameters);
	if(result->errorStruct)
	{
		if(GD::debugLevel >= 2)
		{
			std::cout << "Error calling XML RPC method newDevices on server " << address.first << " with port " << address.second << ". Error struct: ";
			result->print();
		}
		return;
	}
}

void Client::broadcastNewDevices(std::shared_ptr<RPCVariable> deviceDescriptions)
{
	if(!deviceDescriptions) return;
	std::string methodName("newDevices");
	for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
	{
		std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters(new std::list<std::shared_ptr<RPCVariable>>());
		parameters->push_back(std::shared_ptr<RPCVariable>(new RPCVariable((*server)->id)));
		parameters->push_back(deviceDescriptions);
		std::thread t(&RPCClient::invokeBroadcast, &_client, (*server)->address.first, (*server)->address.second, "newDevices", parameters);
		t.detach();
	}
}

void Client::broadcastDeleteDevices(std::shared_ptr<RPCVariable> deviceAddresses)
{
	if(!deviceAddresses) return;
	for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
	{
		std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters(new std::list<std::shared_ptr<RPCVariable>>());
		parameters->push_back(std::shared_ptr<RPCVariable>(new RPCVariable((*server)->id)));
		parameters->push_back(deviceAddresses);
		std::thread t(&RPCClient::invokeBroadcast, &_client, (*server)->address.first, (*server)->address.second, "deleteDevices", parameters);
		t.detach();
	}
}

void Client::broadcastUpdateDevices(std::string address, Hint::Enum hint)
{
	if(!address.empty()) return;
	std::string methodName("deleteDevices");
	for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = _servers->begin(); server != _servers->end(); ++server)
	{
		std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters(new std::list<std::shared_ptr<RPCVariable>>());
		parameters->push_back(std::shared_ptr<RPCVariable>(new RPCVariable((*server)->id)));
		parameters->push_back(std::shared_ptr<RPCVariable>(new RPCVariable(address)));
		parameters->push_back(std::shared_ptr<RPCVariable>(new RPCVariable((int32_t)hint)));
		std::thread t(&RPCClient::invokeBroadcast, &_client, (*server)->address.first, (*server)->address.second, "updateDevices", parameters);
		t.detach();
	}
}

void Client::reset()
{
	_servers.reset(new std::vector<std::shared_ptr<RemoteRPCServer>>());
}

void Client::addServer(std::pair<std::string, std::string> address, std::string id)
{
	for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = _servers->begin(); i != _servers->end(); ++i)
	{
		if((*i)->address == address) return;
	}
	std::shared_ptr<RemoteRPCServer> server(new RemoteRPCServer());
	server->address = address;
	server->id = id;
	_servers->push_back(server);
}

void Client::removeServer(std::pair<std::string, std::string> server)
{
	for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = _servers->begin(); i != _servers->end(); ++i)
	{
		if((*i)->address == server)
		{
			_servers->erase(i);
			return;
		}
	}
}

std::shared_ptr<RemoteRPCServer> Client::getServer(std::pair<std::string, std::string> address)
{
	for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator i = _servers->begin(); i != _servers->end(); ++i)
	{
		if((*i)->address == address) return *i;
	}
	return std::shared_ptr<RemoteRPCServer>();
}

} /* namespace RPC */
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

#include "RPCServer.h"
#include "../GD/GD.h"
#include "../../Modules/Base/BaseLib.h"

using namespace RPC;

RPCServer::Client::Client()
{
	 socket = std::shared_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(GD::bl.get()));
	 socketDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor());
}

RPCServer::Client::~Client()
{
	if(tlsSession) gnutls_bye(tlsSession, GNUTLS_SHUT_WR);
	GD::bl->fileDescriptorManager.shutdown(socketDescriptor);
	if(tlsSession) gnutls_deinit(tlsSession);
}

RPCServer::RPCServer()
{
	_out.init(GD::bl.get());

	_rpcDecoder = std::unique_ptr<BaseLib::RPC::RPCDecoder>(new BaseLib::RPC::RPCDecoder(GD::bl.get()));
	_rpcEncoder = std::unique_ptr<BaseLib::RPC::RPCEncoder>(new BaseLib::RPC::RPCEncoder(GD::bl.get()));
	_xmlRpcDecoder = std::unique_ptr<BaseLib::RPC::XMLRPCDecoder>(new BaseLib::RPC::XMLRPCDecoder(GD::bl.get()));
	_xmlRpcEncoder = std::unique_ptr<BaseLib::RPC::XMLRPCEncoder>(new BaseLib::RPC::XMLRPCEncoder(GD::bl.get()));

	_settings.reset(new ServerSettings::Settings());
	_rpcMethods.reset(new std::map<std::string, std::shared_ptr<RPCMethod>>);
	_serverFileDescriptor.reset(new BaseLib::FileDescriptor);
	_threadPriority = GD::bl->settings.rpcServerThreadPriority();
	_threadPolicy = GD::bl->settings.rpcServerThreadPolicy();
}

RPCServer::~RPCServer()
{
	stop();
	_rpcMethods->clear();
}

void RPCServer::start(std::shared_ptr<ServerSettings::Settings>& settings)
{
	try
	{
		_stopServer = false;
		_settings = settings;
		if(!_settings)
		{
			_out.printError("Error: Settings is nullptr.");
			return;
		}
		_out.setPrefix("RPC Server (Port " + std::to_string(settings->port) + "): ");
		if(_settings->ssl)
		{
			int32_t result = 0;
			if((result = gnutls_certificate_allocate_credentials(&_x509Cred)) != GNUTLS_E_SUCCESS)
			{
				_out.printError("Error: Could not allocate TLS certificate structure: " + std::string(gnutls_strerror(result)));
				return;
			}
			if((result = gnutls_certificate_set_x509_key_file(_x509Cred, GD::bl->settings.certPath().c_str(), GD::bl->settings.keyPath().c_str(), GNUTLS_X509_FMT_PEM)) < 0)
			{
				_out.printError("Error: Could not load certificate or key file: " + std::string(gnutls_strerror(result)));
				gnutls_certificate_free_credentials(_x509Cred);
				return;
			}
			if(!_dhParams)
			{
			if(GD::bl->settings.loadDHParamsFromFile())
			{
				gnutls_datum_t data;
				if((result = gnutls_load_file(GD::bl->settings.dhParamPath().c_str(), &data)) != GNUTLS_E_SUCCESS)
				{
					_out.printError("Error: Could not load DH parameter file: " + std::string(gnutls_strerror(result)));
					gnutls_certificate_free_credentials(_x509Cred);
					return;
				}

			}
			else
			{
				uint32_t bits = gnutls_sec_param_to_pk_bits(GNUTLS_PK_DH, GNUTLS_SEC_PARAM_ULTRA);
				if((result = gnutls_dh_params_init(&_dhParams)) != GNUTLS_E_SUCCESS)
				{
					_out.printError("Error: Could not initialize DH parameters: " + std::string(gnutls_strerror(result)));
					gnutls_certificate_free_credentials(_x509Cred);
					return;
				}
				if((result = gnutls_dh_params_generate2(_dhParams, bits)) != GNUTLS_E_SUCCESS)
				{
					_out.printError("Error: Could not generate DH parameters: " + std::string(gnutls_strerror(result)));
					gnutls_certificate_free_credentials(_x509Cred);
					return;
				}
			}
			}
			if((result = gnutls_priority_init(&_tlsPriorityCache, "PERFORMANCE:%SERVER_PRECEDENCE", NULL)) != GNUTLS_E_SUCCESS)
			{
				_out.printError("Error: Could not initialize cipher priorities: " + std::string(gnutls_strerror(result)));
				gnutls_certificate_free_credentials(_x509Cred);
				return;
			}
			gnutls_certificate_set_dh_params(_x509Cred, _dhParams);
		}
		_mainThread = std::thread(&RPCServer::mainThread, this);
		BaseLib::Threads::setThreadPriority(GD::bl.get(), _mainThread.native_handle(), _threadPriority, _threadPolicy);
		_stopped = false;
		return;
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::stop()
{
	try
	{
		if(_stopped) return;
		_stopped = true;
		_stopServer = true;
		if(_mainThread.joinable()) _mainThread.join();
		int32_t result = 0;
		if(_x509Cred)
		{
			gnutls_certificate_free_credentials(_x509Cred);
			_x509Cred = nullptr;
		}
		if(_tlsPriorityCache)
		{
			gnutls_priority_deinit(_tlsPriorityCache);
			_tlsPriorityCache = nullptr;
		}
		if(_dhParams)
		{
			gnutls_dh_params_deinit(_hhParams);
			_dhParams = nullptr;
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

uint32_t RPCServer::connectionCount()
{
	try
	{
		_stateMutex.lock();
		uint32_t connectionCount = _clients.size();
		_stateMutex.unlock();
		return connectionCount;
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _stateMutex.unlock();
    return 0;
}

void RPCServer::registerMethod(std::string methodName, std::shared_ptr<RPCMethod> method)
{
	try
	{
		if(_rpcMethods->find(methodName) != _rpcMethods->end())
		{
			_out.printWarning("Warning: Could not register RPC method, because a method with this name already exists.");
			return;
		}
		_rpcMethods->insert(std::pair<std::string, std::shared_ptr<RPCMethod>>(methodName, method));
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::closeClientConnection(std::shared_ptr<Client> client)
{
	try
	{
		removeClient(client->id);
		if(client->tlsSession) gnutls_bye(client->tlsSession, GNUTLS_SHUT_WR);
		GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
		if(client->tlsSession)
		{
			gnutls_deinit(client->tlsSession);
			client->tlsSession = nullptr;
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::mainThread()
{
	try
	{
		getSocketDescriptor();
		while(!_stopServer)
		{
			try
			{
				if(!_serverFileDescriptor || _serverFileDescriptor->descriptor < 0)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
					getSocketDescriptor();
					continue;
				}
				std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor = getClientSocketDescriptor();
				if(!clientFileDescriptor || clientFileDescriptor->descriptor < 0) continue;
				_stateMutex.lock();
				if(_clients.size() >= _maxConnections)
				{
					_stateMutex.unlock();
					_out.printError("Error: Client connection rejected, because there are too many clients connected to me.");
					GD::bl->fileDescriptorManager.shutdown(clientFileDescriptor);
					continue;
				}
				std::shared_ptr<Client> client(new Client());
				client->id = _currentClientID++;
				client->socketDescriptor = clientFileDescriptor;
				_clients[client->id] = client;
				_stateMutex.unlock();

				if(_settings->ssl)
				{
					getSSLSocketDescriptor(client);
					if(!client->tlsSession)
					{
						//Remove client from _clients again. Socket is already closed.
						closeClientConnection(client);
						continue;
					}
				}
				client->socket = std::shared_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(GD::bl.get(), client->socketDescriptor, client->tlsSession));

				client->readThread = std::thread(&RPCServer::readClient, this, client);
				BaseLib::Threads::setThreadPriority(GD::bl.get(), client->readThread.native_handle(), _threadPriority, _threadPolicy);
				client->readThread.detach();
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				_stateMutex.unlock();
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				_stateMutex.unlock();
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
				_stateMutex.unlock();
			}
		}
		GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool RPCServer::clientValid(std::shared_ptr<Client>& client)
{
	try
	{
		if(client->socketDescriptor->descriptor < 0) return false;
		return true;
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _stateMutex.unlock();
    return false;
}

void RPCServer::sendRPCResponseToClient(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> data, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		if(!clientValid(client)) return;
		if(!data || data->empty()) return;
		bool error = false;
		try
		{
			client->socket->proofwrite(data);
		}
		catch(BaseLib::SocketDataLimitException& ex)
		{
			_out.printWarning("Warning: " + ex.what());
		}
		catch(BaseLib::SocketOperationException& ex)
		{
			_out.printError("Error: " + ex.what());
			error = true;
		}
		if(!keepAlive || error) closeClientConnection(client);
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::analyzeRPC(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		std::string methodName;
		std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters;
		if(packetType == PacketType::Enum::binaryRequest) parameters = _rpcDecoder->decodeRequest(packet, methodName);
		else if(packetType == PacketType::Enum::xmlRequest) parameters = _xmlRpcDecoder->decodeRequest(packet, methodName);
		if(!parameters)
		{
			_out.printWarning("Warning: Could not decode RPC packet.");
			return;
		}
		PacketType::Enum responseType = (packetType == PacketType::Enum::binaryRequest) ? PacketType::Enum::binaryResponse : PacketType::Enum::xmlResponse;
		if(!parameters->empty() && parameters->at(0)->errorStruct)
		{
			sendRPCResponseToClient(client, parameters->at(0), responseType, keepAlive);
			return;
		}
		callMethod(client, methodName, parameters, responseType, keepAlive);
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::sendRPCResponseToClient(std::shared_ptr<Client> client, std::shared_ptr<BaseLib::RPC::RPCVariable> variable, PacketType::Enum responseType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		std::shared_ptr<std::vector<char>> data;
		if(responseType == PacketType::Enum::xmlResponse)
		{
			data = _xmlRpcEncoder->encodeResponse(variable);
			std::string header = getHttpResponseHeader(data->size());
			data->push_back('\r');
			data->push_back('\n');
			data->insert(data->begin(), header.begin(), header.end());
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response packet: " + std::string(&data->at(0), data->size()));
			}
			sendRPCResponseToClient(client, data, keepAlive);
		}
		else if(responseType == PacketType::Enum::binaryResponse)
		{
			data = _rpcEncoder->encodeResponse(variable);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response binary:");
				_out.printBinary(data);
			}
			sendRPCResponseToClient(client, data, keepAlive);
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCServer::callMethod(std::string& methodName, std::shared_ptr<BaseLib::RPC::RPCVariable>& parameters)
{
	try
	{
		if(!parameters) parameters = std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		if(_stopped) return BaseLib::RPC::RPCVariable::createError(100000, "Server is stopped.");
		if(_rpcMethods->find(methodName) == _rpcMethods->end())
		{
			_out.printError("Warning: RPC method not found: " + methodName);
			return BaseLib::RPC::RPCVariable::createError(-32601, ": Requested method not found.");
		}
		if(GD::bl->debugLevel >= 4)
		{
			_out.printInfo("Info: Method called: " + methodName + " Parameters:");
			for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = parameters->arrayValue->begin(); i != parameters->arrayValue->end(); ++i)
			{
				(*i)->print();
			}
		}
		std::shared_ptr<BaseLib::RPC::RPCVariable> ret = _rpcMethods->at(methodName)->invoke(parameters->arrayValue);
		if(GD::bl->debugLevel >= 5)
		{
			_out.printDebug("Response: ");
			ret->print();
		}
		return ret;
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::RPC::RPCVariable::createError(-32500, ": Unknown application error.");
}

void RPCServer::callMethod(std::shared_ptr<Client> client, std::string methodName, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters, PacketType::Enum responseType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		if(_rpcMethods->find(methodName) == _rpcMethods->end())
		{
			_out.printError("Warning: RPC method not found: " + methodName);
			sendRPCResponseToClient(client, BaseLib::RPC::RPCVariable::createError(-32601, ": Requested method not found."), responseType, keepAlive);
			return;
		}
		if(GD::bl->debugLevel >= 4)
		{
			_out.printInfo("Info: Method called: " + methodName + " Parameters:");
			for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				(*i)->print();
			}
		}
		std::shared_ptr<BaseLib::RPC::RPCVariable> ret = _rpcMethods->at(methodName)->invoke(parameters);
		if(GD::bl->debugLevel >= 5)
		{
			_out.printDebug("Response: ");
			ret->print();
		}
		sendRPCResponseToClient(client, ret, responseType, keepAlive);
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string RPCServer::getHttpResponseHeader(uint32_t contentLength)
{
	std::string header;
	header.append("HTTP/1.1 200 OK\r\n");
	header.append("Connection: close\r\n");
	header.append("Content-Type: text/xml\r\n");
	header.append("Content-Length: ").append(std::to_string(contentLength + 21)).append("\r\n\r\n");
	header.append("<?xml version=\"1.0\"?>");
	return header;
}

void RPCServer::analyzeRPCResponse(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType, bool keepAlive)
{
	try
	{
		if(_stopped) return;
		std::shared_ptr<BaseLib::RPC::RPCVariable> response;
		if(packetType == PacketType::Enum::binaryResponse) response = _rpcDecoder->decodeResponse(packet);
		else if(packetType == PacketType::Enum::xmlResponse) response = _xmlRpcDecoder->decodeResponse(packet);
		if(!response) return;
		if(GD::bl->debugLevel >= 3)
		{
			_out.printWarning("Warning: RPC server received RPC response. This shouldn't happen. Response data: ");
			response->print();
		}
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::packetReceived(std::shared_ptr<Client> client, std::shared_ptr<std::vector<char>> packet, PacketType::Enum packetType, bool keepAlive)
{
	try
	{
		if(packetType == PacketType::Enum::binaryRequest || packetType == PacketType::Enum::xmlRequest) analyzeRPC(client, packet, packetType, keepAlive);
		else if(packetType == PacketType::Enum::binaryResponse || packetType == PacketType::Enum::xmlResponse) analyzeRPCResponse(client, packet, packetType, keepAlive);
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCServer::removeClient(int32_t clientID)
{
	try
	{
		_stateMutex.lock();
		if(_clients.find(clientID) != _clients.end()) _clients.erase(clientID);
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _stateMutex.unlock();
}

void RPCServer::readClient(std::shared_ptr<Client> client)
{
	try
	{
		if(!client) return;
		int32_t bufferMax = 1024;
		char buffer[bufferMax + 1];
		std::shared_ptr<std::vector<char>> packet(new std::vector<char>());
		uint32_t packetLength = 0;
		int32_t bytesRead;
		uint32_t dataSize = 0;
		PacketType::Enum packetType;
		HTTP http;

		_out.printDebug("Listening for incoming packets from client number " + std::to_string(client->socketDescriptor->descriptor) + ".");
		while(!_stopServer)
		{
			try
			{
				bytesRead = client->socket->proofread(buffer, bufferMax);
				//Some clients send only one byte in the first packet
				if(packetLength == 0 && bytesRead == 1) bytesRead += client->socket->proofread(&buffer[1], bufferMax - 1);
			}
			catch(BaseLib::SocketTimeOutException& ex)
			{
				continue;
			}
			catch(BaseLib::SocketClosedException& ex)
			{
				_out.printInfo("Info: " + ex.what());
				break;
			}
			catch(BaseLib::SocketOperationException& ex)
			{
				_out.printError(ex.what());
				break;
			}

			if(!clientValid(client)) break;

			if(GD::bl->debugLevel >= 5)
			{
				std::vector<uint8_t> rawPacket;
				rawPacket.insert(rawPacket.begin(), buffer, buffer + bytesRead);
				_out.printDebug("Debug: Packet received: " + BaseLib::HelperFunctions::getHexString(rawPacket));
			}
			if(!strncmp(&buffer[0], "Bin", 3))
			{
				http.reset();
				//buffer[3] & 1 is true for buffer[3] == 0xFF, too
				packetType = (buffer[3] & 1) ? PacketType::Enum::binaryResponse : PacketType::Enum::binaryRequest;
				if(bytesRead < 8) continue;
				uint32_t headerSize = 0;
				if(buffer[3] & 0x40)
				{
					GD::bl->hf.memcpyBigEndian((char*)&headerSize, buffer + 4, 4);
					if(bytesRead < headerSize + 12)
					{
						_out.printError("Error: Binary rpc packet has invalid header size.");
						continue;
					}
					GD::bl->hf.memcpyBigEndian((char*)&dataSize, buffer + 8 + headerSize, 4);
					dataSize += headerSize + 4;
				}
				else GD::bl->hf.memcpyBigEndian((char*)&dataSize, buffer + 4, 4);
				_out.printDebug("Receiving binary rpc packet with size: " + std::to_string(dataSize), 6);
				if(dataSize == 0) continue;
				if(headerSize > 1024)
				{
					_out.printError("Error: Binary rpc packet with header larger than 1 KiB received.");
					continue;
				}
				if(dataSize > 104857600)
				{
					_out.printError("Error: Packet with data larger than 100 MiB received.");
					continue;
				}
				packet.reset(new std::vector<char>());
				packet->insert(packet->end(), buffer, buffer + bytesRead);
				std::shared_ptr<BaseLib::RPC::RPCHeader> header = _rpcDecoder->decodeHeader(packet);
				if(_settings->authType == ServerSettings::Settings::AuthType::basic)
				{
					if(!client->auth.initialized()) client->auth = Auth(client->socket, _settings->validUsers);
					bool authFailed = false;
					try
					{
						if(!client->auth.basicServer(header))
						{
							_out.printError("Error: Authorization failed. Closing connection.");
							break;
						}
						else _out.printDebug("Client successfully authorized using basic authentification.");
					}
					catch(AuthException& ex)
					{
						_out.printError("Error: Authorization failed. Closing connection. Error was: " + ex.what());
						break;
					}
				}
				if(dataSize > bytesRead - 8) packetLength = bytesRead - 8;
				else
				{
					packetLength = 0;
					std::thread t(&RPCServer::packetReceived, this, client, packet, packetType, true);
					BaseLib::Threads::setThreadPriority(GD::bl.get(), t.native_handle(), _threadPriority, _threadPolicy);
					t.detach();
				}
			}
			else if(!strncmp(&buffer[0], "POST", 4) || !strncmp(&buffer[0], "HTTP/1.", 7))
			{
				packetType = (!strncmp(&buffer[0], "POST", 4)) ? PacketType::Enum::xmlRequest : PacketType::Enum::xmlResponse;
				//We are using string functions to process the buffer. So just to make sure,
				//they don't do something in the memory after buffer, we add '\0'
				buffer[bytesRead] = '\0';

				try
				{
					http.reset();
					http.process(buffer, bytesRead);
				}
				catch(HTTPException& ex)
				{
					_out.printError("XML RPC Server: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, bytesRead));
				}

				if(http.getHeader()->contentLength > 104857600)
				{
					_out.printError("Error: Packet with data larger than 100 MiB received.");
					continue;
				}

				if(_settings->authType == ServerSettings::Settings::AuthType::basic)
				{
					if(!client->auth.initialized()) client->auth = Auth(client->socket, _settings->validUsers);
					bool authFailed = false;
					try
					{
						if(!client->auth.basicServer(http))
						{
							_out.printError("Error: Authorization failed for host " + http.getHeader()->host + ". Closing connection.");
							break;
						}
						else _out.printDebug("Client successfully authorized using basic authentification.");
					}
					catch(AuthException& ex)
					{
						_out.printError("Error: Authorization failed for host " + http.getHeader()->host + ". Closing connection. Error was: " + ex.what());
						break;
					}
				}
			}
			else if(packetLength > 0 || http.dataProcessed())
			{
				if(packetType == PacketType::Enum::binaryRequest || packetType == PacketType::Enum::binaryRequest)
				{
					if(packetLength + bytesRead > dataSize)
					{
						_out.printError("Error: Packet length is wrong.");
						packetLength = 0;
						continue;
					}
					packet->insert(packet->end(), buffer, buffer + bytesRead);
					packetLength += bytesRead;
					if(packetLength == dataSize)
					{
						packet->push_back('\0');
						std::thread t(&RPCServer::packetReceived, this, client, packet, packetType, true);
						BaseLib::Threads::setThreadPriority(GD::bl.get(), t.native_handle(), _threadPriority, _threadPolicy);
						t.detach();
						packetLength = 0;
					}
				}
				else
				{
					try
					{
						http.process(buffer, bytesRead);
					}
					catch(HTTPException& ex)
					{
						_out.printError("XML RPC Server: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer, bytesRead));
					}

					if(http.getContentSize() > 104857600)
					{
						http.reset();
						_out.printError("Error: Packet with data larger than 100 MiB received.");
					}
				}
			}
			else
			{
				_out.printError("Error: Uninterpretable packet received. Closing connection. Packet was: " + std::string(buffer, buffer + bytesRead));
				break;
			}
			if(http.isFinished())
			{
				std::thread t(&RPCServer::packetReceived, this, client, http.getContent(), packetType, http.getHeader()->connection == HTTP::Connection::Enum::keepAlive);
				BaseLib::Threads::setThreadPriority(GD::bl.get(), t.native_handle(), _threadPriority, _threadPolicy);
				t.detach();
				packetLength = 0;
				http.reset();
			}
		}
		//This point is only reached, when stopServer is true or the socket is closed
		closeClientConnection(client);
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<BaseLib::FileDescriptor> RPCServer::getClientSocketDescriptor()
{
	std::shared_ptr<BaseLib::FileDescriptor> fileDescriptor;
	try
	{
		timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		fd_set readFileDescriptor;
		FD_ZERO(&readFileDescriptor);
		FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);
		if(!select(_serverFileDescriptor->descriptor + 1, &readFileDescriptor, NULL, NULL, &timeout)) return fileDescriptor;

		struct sockaddr_storage clientInfo;
		socklen_t addressSize = sizeof(addressSize);
		fileDescriptor = GD::bl->fileDescriptorManager.add(accept(_serverFileDescriptor->descriptor, (struct sockaddr *) &clientInfo, &addressSize));
		if(!fileDescriptor) return fileDescriptor;

		getpeername(fileDescriptor->descriptor, (struct sockaddr*)&clientInfo, &addressSize);

		uint32_t port;
		char ipString[INET6_ADDRSTRLEN];
		if (clientInfo.ss_family == AF_INET) {
			struct sockaddr_in *s = (struct sockaddr_in *)&clientInfo;
			port = ntohs(s->sin_port);
			inet_ntop(AF_INET, &s->sin_addr, ipString, sizeof(ipString));
		} else { // AF_INET6
			struct sockaddr_in6 *s = (struct sockaddr_in6 *)&clientInfo;
			port = ntohs(s->sin6_port);
			inet_ntop(AF_INET6, &s->sin6_addr, ipString, sizeof(ipString));
		}
		std::string ipString2(&ipString[0]);
		_out.printInfo("Info: Connection from " + ipString2 + ":" + std::to_string(port) + " accepted. Client number: " + std::to_string(fileDescriptor->descriptor));
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return fileDescriptor;
}

void RPCServer::getSSLSocketDescriptor(std::shared_ptr<Client> client)
{
	try
	{
		if(!_tlsPriorityCache)
		{
			_out.printError("Error: Could not initiate TLS connection. _tlsPriorityCache is nullptr.");
			return;
		}
		if(!_x509Cred)
		{
			_out.printError("Error: Could not initiate TLS connection. _x509Cred is nullptr.");
			return;
		}
		int32_t result = 0;
		if((result = gnutls_init(&client->tlsSession, GNUTLS_SERVER)) != GNUTLS_E_SUCCESS)
		{
			_out.printError("Error: Could not initialize TLS session: " + std::string(gnutls_strerror(result)));
			client->tlsSession = nullptr;
			return;
		}
		if(!client->tlsSession)
		{
			_out.printError("Error: Client TLS session is nullptr.");
			return;
		}
		if((result = gnutls_priority_set(client->tlsSession, _tlsPriorityCache)) != GNUTLS_E_SUCCESS)
		{
			_out.printError("Error: Could not set cipher priority on TLS session: " + std::string(gnutls_strerror(result)));
			GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
			gnutls_deinit(client->tlsSession);
			client->tlsSession = nullptr;
			return;
		}
		if((result = gnutls_credentials_set(client->tlsSession, GNUTLS_CRD_CERTIFICATE, _x509Cred)) != GNUTLS_E_SUCCESS)
		{
			_out.printError("Error: Could not set x509 credentials on TLS session: " + std::string(gnutls_strerror(result)));
			GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
			gnutls_deinit(client->tlsSession);
			client->tlsSession = nullptr;
			return;
		}
		gnutls_certificate_server_set_request(client->tlsSession, GNUTLS_CERT_IGNORE);
		if(!client->socketDescriptor || client->socketDescriptor->descriptor == -1)
		{
			_out.printError("Error setting TLS socket descriptor: Provided socket descriptor is invalid.");
			gnutls_deinit(client->tlsSession);
			client->tlsSession = nullptr;
			return;
		}
		gnutls_transport_set_ptr(client->tlsSession, (gnutls_transport_ptr_t)client->socketDescriptor->descriptor);
		do
		{
			result = gnutls_handshake(client->tlsSession);
        } while (result < 0 && gnutls_error_is_fatal(result) == 0);
		if(result < 0)
		{
			_out.printError("Error: TLS handshake has failed: " + std::string(gnutls_strerror(result)));
			GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
			gnutls_deinit(client->tlsSession);
			client->tlsSession = nullptr;
			return;
		}
		return;
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
    if(client->tlsSession) gnutls_deinit(client->tlsSession);
    client->tlsSession = nullptr;
}

void RPCServer::getSocketDescriptor()
{
	try
	{
		addrinfo hostInfo;
		addrinfo *serverInfo = nullptr;

		int32_t yes = 1;

		memset(&hostInfo, 0, sizeof(hostInfo));

		hostInfo.ai_family = AF_UNSPEC;
		hostInfo.ai_socktype = SOCK_STREAM;
		hostInfo.ai_flags = AI_PASSIVE;
		char buffer[100];
		std::string port = std::to_string(_settings->port);
		int32_t result;
		if((result = getaddrinfo(_settings->interface.c_str(), port.c_str(), &hostInfo, &serverInfo)) != 0)
		{
			_out.printCritical("Error: Could not get address information: " + std::string(gai_strerror(result)));
			return;
		}

		bool bound = false;
		int32_t error = 0;
		for(struct addrinfo *info = serverInfo; info != 0; info = info->ai_next)
		{
			_serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(info->ai_family, info->ai_socktype, info->ai_protocol));
			if(_serverFileDescriptor->descriptor == -1) continue;
			if(!(fcntl(_serverFileDescriptor->descriptor, F_GETFL) & O_NONBLOCK))
			{
				if(fcntl(_serverFileDescriptor->descriptor, F_SETFL, fcntl(_serverFileDescriptor->descriptor, F_GETFL) | O_NONBLOCK) < 0) throw BaseLib::Exception("Error: Could not set socket options.");
			}
			if(setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t)) == -1) throw BaseLib::Exception("Error: Could not set socket options.");
			if(bind(_serverFileDescriptor->descriptor, info->ai_addr, info->ai_addrlen) == -1)
			{
				error = errno;
				continue;
			}
			std::string address;
			switch (info->ai_family)
			{
				case AF_INET:
					inet_ntop (info->ai_family, &((struct sockaddr_in *) info->ai_addr)->sin_addr, buffer, 100);
					address = std::string(buffer);
					break;
				case AF_INET6:
					inet_ntop (info->ai_family, &((struct sockaddr_in6 *) info->ai_addr)->sin6_addr, buffer, 100);
					address = std::string(buffer);
					break;
			}
			_out.printInfo("Info: RPC Server started listening on address " + address + " and port " + port);
			bound = true;
			break;
		}
		freeaddrinfo(serverInfo);
		if(!bound)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			_out.printCritical("Error: Server could not start listening on port " + port + ": " + std::string(strerror(error)));
			return;
		}
		if(_serverFileDescriptor->descriptor == -1 || !bound || listen(_serverFileDescriptor->descriptor, _backlog) == -1)
		{
			GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
			_out.printCritical("Error: Server could not start listening on port " + port + ": " + std::string(strerror(errno)));
			return;
		}
    }
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

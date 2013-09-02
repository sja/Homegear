#include "BidCoSPacketManager.h"
#include "GD.h"
#include "HelperFunctions.h"

BidCoSPacketInfo::BidCoSPacketInfo()
{
	time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

BidCoSPacketManager::BidCoSPacketManager()
{
	try
	{
		_workerThread = std::thread(&BidCoSPacketManager::worker, this);
		HelperFunctions::setThreadPriority(_workerThread.native_handle(), 19);
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

BidCoSPacketManager::~BidCoSPacketManager()
{
	if(!_disposing) dispose();
	if(_workerThread.joinable()) _workerThread.join();
}

void BidCoSPacketManager::dispose(bool wait)
{
	_disposing = true;
	_stopWorkerThread = true;
}

void BidCoSPacketManager::worker()
{
	try
	{
		std::chrono::milliseconds sleepingTime(1000);
		uint32_t counter = 0;
		int32_t lastPacket;
		lastPacket = 0;
		while(!_stopWorkerThread)
		{
			try
			{
				std::this_thread::sleep_for(sleepingTime);
				if(_stopWorkerThread) return;
				if(counter > 100)
				{
					counter = 0;
					_packetMutex.lock();
					if(_packets.size() > 0)
					{
						int32_t packetsPerSecond = (_packets.size() * 1000) / sleepingTime.count();
						int32_t timePerPacket = (GD::settings.workerThreadWindow() * 10) / packetsPerSecond;
						if(timePerPacket < 10) timePerPacket = 10;
						sleepingTime = std::chrono::milliseconds(timePerPacket);
					}
					_packetMutex.unlock();
				}
				_packetMutex.lock();
				if(!_packets.empty())
				{
					if(!_packets.empty())
					{
						std::unordered_map<int32_t, std::shared_ptr<BidCoSPacketInfo>>::iterator nextPacket = _packets.find(lastPacket);
						if(nextPacket != _packets.end())
						{
							nextPacket++;
							if(nextPacket == _packets.end()) nextPacket = _packets.begin();
						}
						else nextPacket = _packets.begin();
						lastPacket = nextPacket->first;
					}
				}
				std::shared_ptr<BidCoSPacketInfo> packet;
				if(_packets.find(lastPacket) != _packets.end()) packet = _packets.at(lastPacket);
				_packetMutex.unlock();
				if(packet) deletePacket(lastPacket, packet->id);
				counter++;
			}
			catch(const std::exception& ex)
			{
				_packetMutex.unlock();
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(Exception& ex)
			{
				_packetMutex.unlock();
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_packetMutex.unlock();
				HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
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

void BidCoSPacketManager::set(int32_t address, std::shared_ptr<BidCoSPacket>& packet, int64_t time)
{
	try
	{
		if(_disposing) return;
		_packetMutex.lock();
		if(_packets.find(address) != _packets.end()) _packets.erase(_packets.find(address));
		_packetMutex.unlock();

		std::shared_ptr<BidCoSPacketInfo> info(new BidCoSPacketInfo());
		info->packet = packet;
		info->id = _id++;
		if(time > 0) info->time = time;
		_packetMutex.lock();
		_packets.insert(std::pair<int32_t, std::shared_ptr<BidCoSPacketInfo>>(address, info));
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
    _packetMutex.unlock();
}

void BidCoSPacketManager::deletePacket(int32_t address, uint32_t id)
{
	try
	{
		if(_disposing) return;
		_packetMutex.lock();
		if(_packets.find(address) != _packets.end() && _packets.at(address) && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() <= _packets.at(address)->time + 1000)
		{
			_packetMutex.unlock();
			return;
		}
		if(_packets.find(address) != _packets.end() && _packets.at(address) && _packets.at(address)->id == id)
		{
			_packets.erase(address);
		}
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
    _packetMutex.unlock();
}

std::shared_ptr<BidCoSPacket> BidCoSPacketManager::get(int32_t address)
{
	try
	{
		if(_disposing) return std::shared_ptr<BidCoSPacket>();
		_packetMutex.lock();
		//Make a copy to make sure, the element exists
		std::shared_ptr<BidCoSPacket> packet((_packets.find(address) != _packets.end()) ? _packets[address]->packet : nullptr);
		_packetMutex.unlock();
		return packet;
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
    _packetMutex.unlock();
    return std::shared_ptr<BidCoSPacket>();
}

std::shared_ptr<BidCoSPacketInfo> BidCoSPacketManager::getInfo(int32_t address)
{
	try
	{
		if(_disposing) return std::shared_ptr<BidCoSPacketInfo>();
		//Make a copy to make sure, the element exists
		_packetMutex.lock();
		std::shared_ptr<BidCoSPacketInfo> info((_packets.find(address) != _packets.end()) ? _packets[address] : nullptr);
		_packetMutex.unlock();
		return info;
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
    _packetMutex.unlock();
    return std::shared_ptr<BidCoSPacketInfo>();
}

void BidCoSPacketManager::keepAlive(int32_t address)
{
	try
	{
		if(_disposing) return;
		_packetMutex.lock();
		if(_packets.find(address) != _packets.end()) _packets[address]->time = HelperFunctions::getTime();
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
    _packetMutex.unlock();
}

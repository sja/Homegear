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

#include "HMWiredPacketManager.h"
#include "../../GD/GD.h"

namespace HMWired
{
HMWiredPacketInfo::HMWiredPacketInfo()
{
	time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

HMWiredPacketManager::HMWiredPacketManager()
{
	try
	{
		_workerThread = std::thread(&HMWiredPacketManager::worker, this);
		Threads::setThreadPriority(_workerThread.native_handle(), 19);
	}
	catch(const std::exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

HMWiredPacketManager::~HMWiredPacketManager()
{
	if(!_disposing) dispose();
	if(_workerThread.joinable()) _workerThread.join();
}

void HMWiredPacketManager::dispose(bool wait)
{
	_disposing = true;
	_stopWorkerThread = true;
}

void HMWiredPacketManager::worker()
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
						if(packetsPerSecond <= 0) packetsPerSecond = 1;
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
						std::unordered_map<int32_t, std::shared_ptr<HMWiredPacketInfo>>::iterator nextPacket = _packets.find(lastPacket);
						if(nextPacket != _packets.end())
						{
							nextPacket++;
							if(nextPacket == _packets.end()) nextPacket = _packets.begin();
						}
						else nextPacket = _packets.begin();
						lastPacket = nextPacket->first;
					}
				}
				std::shared_ptr<HMWiredPacketInfo> packet;
				if(_packets.find(lastPacket) != _packets.end()) packet = _packets.at(lastPacket);
				_packetMutex.unlock();
				if(packet) deletePacket(lastPacket, packet->id);
				counter++;
			}
			catch(const std::exception& ex)
			{
				_packetMutex.unlock();
				GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(Exception& ex)
			{
				_packetMutex.unlock();
				GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_packetMutex.unlock();
				GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
	}
    catch(const std::exception& ex)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HMWiredPacketManager::set(int32_t address, std::shared_ptr<HMWiredPacket>& packet, int64_t time)
{
	try
	{
		if(_disposing) return;
		_packetMutex.lock();
		if(_packets.find(address) != _packets.end()) _packets.erase(_packets.find(address));
		_packetMutex.unlock();

		std::shared_ptr<HMWiredPacketInfo> info(new HMWiredPacketInfo());
		info->packet = packet;
		info->id = _id++;
		if(time > 0) info->time = time;
		_packetMutex.lock();
		_packets.insert(std::pair<int32_t, std::shared_ptr<HMWiredPacketInfo>>(address, info));
	}
	catch(const std::exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _packetMutex.unlock();
}

void HMWiredPacketManager::deletePacket(int32_t address, uint32_t id)
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
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _packetMutex.unlock();
}

std::shared_ptr<HMWiredPacket> HMWiredPacketManager::get(int32_t address)
{
	try
	{
		if(_disposing) return std::shared_ptr<HMWiredPacket>();
		_packetMutex.lock();
		//Make a copy to make sure, the element exists
		std::shared_ptr<HMWiredPacket> packet((_packets.find(address) != _packets.end()) ? _packets[address]->packet : nullptr);
		_packetMutex.unlock();
		return packet;
	}
	catch(const std::exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _packetMutex.unlock();
    return std::shared_ptr<HMWiredPacket>();
}

std::shared_ptr<HMWiredPacketInfo> HMWiredPacketManager::getInfo(int32_t address)
{
	try
	{
		if(_disposing) return std::shared_ptr<HMWiredPacketInfo>();
		//Make a copy to make sure, the element exists
		_packetMutex.lock();
		std::shared_ptr<HMWiredPacketInfo> info((_packets.find(address) != _packets.end()) ? _packets[address] : nullptr);
		_packetMutex.unlock();
		return info;
	}
	catch(const std::exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _packetMutex.unlock();
    return std::shared_ptr<HMWiredPacketInfo>();
}

void HMWiredPacketManager::keepAlive(int32_t address)
{
	try
	{
		if(_disposing) return;
		_packetMutex.lock();
		if(_packets.find(address) != _packets.end()) _packets[address]->time = GD::helperFunctions->getTime();
	}
	catch(const std::exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::output->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _packetMutex.unlock();
}
}

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

#ifndef HM_CFG_LAN_H
#define HM_CFG_LAN_H

#include "../BidCoSPacket.h"
#include "../../Base/BaseLib.h"

#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

namespace BidCoS
{

class HM_CFG_LAN  : public BaseLib::Systems::PhysicalDevice
{
    public:
        HM_CFG_LAN(std::shared_ptr<BaseLib::Systems::PhysicalDeviceSettings> settings);
        virtual ~HM_CFG_LAN();
        void startListening();
        void stopListening();
        void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet);
        int64_t lastAction() { return _lastAction; }
        virtual bool isOpen() { return _socket.connected(); }
    protected:
        int64_t _lastAction = 0;
        std::string _hostname;
        std::string _port;
        BaseLib::SocketOperations _socket;
        std::mutex _sendMutex;
        std::list<std::vector<char>> _initCommandQueue;
        std::vector<char> _keepAlivePacket = { 'K', '\r', '\n' };

        //AES stuff
        bool _aesInitialized = false;
        bool _aesExchangeComplete = false;
        bool _useAES = false;
        std::vector<uint8_t> _key;
		std::vector<uint8_t> _remoteIV;
		std::vector<uint8_t> _myIV;
        EVP_CIPHER_CTX* _ctxEncrypt;
        EVP_CIPHER_CTX* _ctxDecrypt;

        std::vector<char> encrypt(std::vector<char>& data);
        std::vector<uint8_t> decrypt(std::vector<uint8_t>& data);
        bool aesKeyExchange(std::vector<uint8_t>& data);
        bool openSSLInit();
        void openSSLCleanup();
        void openSSLPrintError();
        //End AES stuff

        void reconnect();
        void createInitCommandQueue();
        void processData(std::vector<uint8_t>& data);
        void processInit(std::string& packet);
        void parsePacket(std::string& packet);
        void send(std::vector<char>& data, bool raw, bool printData);
        void sendKeepAlive();
        void listen();
        void getFileDescriptor(bool& timedout);
        std::shared_ptr<BaseLib::FileDescriptor> getConnection(std::string& hostname, const std::string& port, std::string& ipAddress);
    private:
};

}
#endif // TCPSOCKETDEVICE_H

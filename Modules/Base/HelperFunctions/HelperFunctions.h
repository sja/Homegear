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

#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#include "../Systems/DeviceFamilies.h"

#include <algorithm>
#include <ctime>
#include <map>
#include <fstream>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509_vfy.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

class HelperFunctions {
public:
	HelperFunctions();
	virtual ~HelperFunctions();

	virtual bool fileExists(std::string filename);
	virtual std::string getFileContent(std::string filename);
	virtual std::vector<std::string> getFiles(std::string path);

	virtual int64_t getTime();
	virtual int32_t getTimeSeconds();
	virtual std::string getTimeString(int64_t time = 0);

	virtual inline std::string &ltrim(std::string &s)
	{
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
			return s;
	}

	virtual inline std::string &rtrim(std::string &s)
	{
			s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
			return s;
	}

	virtual inline std::string& trim(std::string& s)
	{
			return ltrim(rtrim(s));
	}

	virtual inline std::string& toLower (std::string& s)
	{
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return s;
	}

	virtual inline std::wstring& toLower (std::wstring& s)
	{
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return s;
	}

	virtual inline std::string& stringReplace(std::string& haystack, std::string search, std::string replace)
	{
		int32_t pos = 0;
		while(true)
		{
			 pos = haystack.find(search, pos);
			 if (pos == std::string::npos) break;
			 haystack.replace(pos, search.size(), replace);
			 pos += search.size();
		}
		return haystack;
	}

	virtual std::pair<std::string, std::string> split(std::string string, char delimiter);

	virtual inline double getDouble(std::string &s)
	{
		double number = 0;
		try { number = std::stod(s); } catch(...) {}
		return number;
	}

	virtual inline bool isNotAlphaNumeric(char c)
	{
		return !(isalpha(c) || isdigit(c) || (c == '_') || (c == '-'));
	}

	virtual bool isAlphaNumeric(std::string& s)
	{
		return find_if
		(
			s.begin(),
			s.end(),
			[](const char c){ return !(isalpha(c) || isdigit(c) || (c == '_') || (c == '-')); }
		) == s.end();
	}

	virtual bool isNumber(std::string& s)
	{
		int32_t xpos = s.find('x');
		if(xpos == -1) try { std::stoll(s, 0, 10); } catch(...) { return false; }
		else try { std::stoll(s, 0, 16); } catch(...) { return false; }
		return true;
	}

	virtual int32_t getNumber(std::string& s, bool isHex = false)
	{
		int32_t xpos = s.find('x');
		int32_t number = 0;
		if(xpos == -1 && !isHex) try { number = std::stoll(s, 0, 10); } catch(...) {}
		else try { number = std::stoll(s, 0, 16); } catch(...) {}
		return number;
	}

	virtual int32_t getNumber(char hexChar)
	{
		if(_hexMap.find(hexChar) == _hexMap.end()) return 0;
		return _hexMap.at(hexChar);
	}

	virtual uint32_t getUnsignedNumber(std::string &s, bool isHex = false)
	{
		int32_t xpos = s.find('x');
		uint32_t number = 0;
		if(xpos == -1 && !isHex) try { number = std::stoull(s, 0, 10); } catch(...) {}
		else try { number = std::stoull(s, 0, 16); } catch(...) {}
		return number;
	}

	virtual bool getBigEndian() { return _isBigEndian; }

	virtual void copyFile(std::string source, std::string dest);
	virtual int32_t getRandomNumber(int32_t min, int32_t max);

	virtual void memcpyBigEndian(char* to, char* from, const uint32_t& length);
	virtual void memcpyBigEndian(uint8_t* to, uint8_t* from, const uint32_t& length);
	virtual void memcpyBigEndian(int32_t& to, std::vector<uint8_t>& from);
	virtual void memcpyBigEndian(std::vector<uint8_t>& to, int32_t& from);
	virtual std::string getHexString(const std::vector<char>& data);
	virtual std::string getHexString(const std::vector<uint8_t>& data);
	virtual std::string getHexString(int32_t number, int32_t width = -1);
	virtual std::vector<char> getBinary(std::string hexString);
	virtual std::vector<uint8_t> getUBinary(std::string hexString);
	virtual std::string getSSLError(int32_t errorNumber);
	virtual std::string getSSLCertVerificationError(int32_t errorNumber);
	virtual int32_t userID(std::string username);
	virtual int32_t groupID(std::string groupname);

private:
	bool _isBigEndian;
	std::map<char, int32_t> _hexMap;

	virtual void checkEndianness();
};

#endif /* HELPERFUNCTIONS_H_ */
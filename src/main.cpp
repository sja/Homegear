/* Copyright 2013-2016 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#include "GD/GD.h"
#include "Monitor.h"
#ifndef __aarch64__
#include "DeathHandler.h"
#endif
#include "CLI/CLIClient.h"
#include "ScriptEngine/ScriptEngineClient.h"
#include "UPnP/UPnP.h"
#include "MQTT/Mqtt.h"
#include "homegear-base/BaseLib.h"
#include "homegear-base/HelperFunctions/HelperFunctions.h"
#include "../config.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <execinfo.h>
#include <signal.h>
#include <wait.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/prctl.h> //For function prctl
#include <sys/sysctl.h> //For BSD systems
#include <malloc.h>

#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>

#include <gcrypt.h>

void startMainProcess();
void startUp();

GCRY_THREAD_OPTION_PTHREAD_IMPL;

Monitor _monitor;
std::mutex _shuttingDownMutex;
bool _reloading = false;
bool _fork = false;
bool _monitorProcess = false;
pid_t _mainProcessId = 0;
bool _startAsDaemon = false;
bool _startUpComplete = false;
bool _shutdownQueued = false;
bool _disposing = false;
std::shared_ptr<std::function<void(int32_t, std::string)>> _errorCallback;

void exitHomegear(int exitCode)
{
	if(GD::familyController) GD::familyController->disposeDeviceFamilies();
	if(GD::bl->db)
	{
		//Finish database operations before closing modules, otherwise SEGFAULT
		GD::bl->db->dispose();
		GD::bl->db.reset();
	}
    if(GD::familyController) GD::familyController->dispose();
    if(GD::licensingController) GD::licensingController->dispose();
    _monitor.stop();
    exit(exitCode);
}

void startRPCServers()
{
	for(int32_t i = 0; i < GD::serverInfo.count(); i++)
	{
		BaseLib::Rpc::PServerInfo settings = GD::serverInfo.get(i);
		std::string info = "Starting XML RPC server " + settings->name + " listening on " + settings->interface + ":" + std::to_string(settings->port);
		if(settings->ssl) info += ", SSL enabled";
		else GD::bl->rpcPort = settings->port;
		if(settings->authType != BaseLib::Rpc::ServerInfo::Info::AuthType::none) info += ", authentication enabled";
		info += "...";
		GD::out.printInfo(info);
		GD::rpcServers[i].start(settings);
	}
	if(GD::rpcServers.size() == 0)
	{
		GD::out.printCritical("Critical: No RPC servers are running. Terminating Homegear.");
		exitHomegear(1);
	}

}

void stopRPCServers(bool dispose)
{
	GD::out.printInfo( "(Shutdown) => Stopping RPC servers");
	for(std::map<int32_t, RPC::Server>::iterator i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i)
	{
		i->second.stop();
		if(dispose) i->second.dispose();
	}
	GD::bl->rpcPort = 0;
	//Don't clear map!!! Server is still accessed i. e. by the event handler!
}

void sigchld_handler(int32_t signalNumber)
{
	try
	{
		pid_t pid;
		int status;

		while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
		{
			int32_t exitStatus = WEXITSTATUS(status);
			int32_t signal = -1;
			bool coreDumped = false;
			if(WIFSIGNALED(status))
			{
				signal = WTERMSIG(status);
				if(WCOREDUMP(status)) coreDumped = true;
			}

			GD::out.printInfo("Info: Process with id " + std::to_string(pid) + " ended.");
			if(pid == _mainProcessId)
			{
				_mainProcessId = 0;
				bool stop = false;
				if(signal != -1)
				{
					if(signal == SIGTERM || signal == SIGINT || signal == SIGQUIT || (signal == SIGKILL && !_monitor.killedProcess())) stop = true;
					if(signal == SIGKILL && !_monitor.killedProcess()) GD::out.printWarning("Warning: SIGKILL (signal 9) used to stop Homegear. Please shutdown Homegear properly to avoid database corruption.");
					if(coreDumped) GD::out.printError("Error: Core was dumped.");
				}
				else stop = true;
				if(stop)
				{
					GD::out.printInfo("Info: Homegear exited with exit code " + std::to_string(exitStatus) + ". Stopping monitor process.");
					exit(0);
				}

				GD::out.printError("Homegear was terminated. Restarting (1)...");
				_monitor.suspend();
				_fork = true;
			}
			else if(GD::scriptEngineServer) GD::scriptEngineServer->processKilled(pid, exitStatus, signal, coreDumped);
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void terminate(int32_t signalNumber)
{
	try
	{
		if(signalNumber == SIGTERM)
		{
			if(_monitorProcess)
			{
				if(_mainProcessId != 0) kill(_mainProcessId, SIGTERM);
				else exit(0);
				return;
			}

			_shuttingDownMutex.lock();
			if(!_startUpComplete)
			{
				_shutdownQueued = true;
				_shuttingDownMutex.unlock();
				return;
			}
			if(GD::bl->shuttingDown)
			{
				_shuttingDownMutex.unlock();
				return;
			}
			GD::out.printMessage("(Shutdown) => Stopping Homegear (Signal: " + std::to_string(signalNumber) + ")");
			GD::bl->shuttingDown = true;
			_shuttingDownMutex.unlock();
			if(GD::scriptEngineServer) GD::scriptEngineServer->homegearShuttingDown(); //Needs to be called before familyController->homegearShuttingDown()
			if(GD::familyController) GD::familyController->homegearShuttingDown();
			_disposing = true;
			if(_startAsDaemon)
			{
				GD::out.printInfo("(Shutdown) => Stopping CLI server");
				if(GD::cliServer) GD::cliServer->stop();
			}
			if(GD::bl->settings.enableUPnP())
			{
				GD::out.printInfo("Stopping UPnP server...");
				GD::uPnP->stop();
			}
#ifdef EVENTHANDLER
			if(GD::eventHandler) GD::eventHandler->dispose();
#endif
			stopRPCServers(true);
			GD::rpcServers.clear();
			GD::out.printInfo( "(Shutdown) => Stopping Event handler");

			if(GD::mqtt && GD::mqtt->enabled())
			{
				GD::out.printInfo( "(Shutdown) => Stopping MQTT client");;
				GD::mqtt->stop();
			}
			GD::out.printInfo( "(Shutdown) => Stopping RPC client");;
			if(GD::rpcClient) GD::rpcClient->dispose();
			GD::out.printInfo( "(Shutdown) => Closing physical interfaces");
			if(GD::familyController) GD::familyController->physicalInterfaceStopListening();
			GD::out.printInfo("(Shutdown) => Stopping script engine server...");
			GD::scriptEngineServer->stop();
			GD::out.printMessage("(Shutdown) => Saving device families");
			if(GD::familyController) GD::familyController->save(false);
			GD::out.printMessage("(Shutdown) => Disposing device families");
			if(GD::familyController) GD::familyController->disposeDeviceFamilies();
			GD::out.printMessage("(Shutdown) => Disposing database");
			if(GD::bl->db)
			{
				//Finish database operations before closing modules, otherwise SEGFAULT
				GD::bl->db->dispose();
				GD::bl->db.reset();
			}
			GD::out.printMessage("(Shutdown) => Disposing family modules");
			GD::familyController->dispose();
			GD::out.printMessage("(Shutdown) => Disposing licensing modules");
			if(GD::licensingController) GD::licensingController->dispose();
			GD::bl->fileDescriptorManager.dispose();
			_monitor.stop();
			GD::out.printMessage("(Shutdown) => Shutdown complete.");
			if(_startAsDaemon)
			{
				fclose(stdout);
				fclose(stderr);
			}
			gnutls_global_deinit();
			gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
			gcry_control(GCRYCTL_TERM_SECMEM);
			gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
			exit(0);
		}
		else if(signalNumber == SIGHUP)
		{
			if(_monitorProcess)
			{
				if(_mainProcessId != 0) kill(_mainProcessId, SIGHUP);
				return;
			}

			_shuttingDownMutex.lock();
			GD::out.printInfo("Info: SIGHUP received... Reloading...");
			if(!_startUpComplete)
			{
				GD::out.printError("Error: Cannot reload. Startup is not completed.");
				_shuttingDownMutex.unlock();
				return;
			}
			_startUpComplete = false;
			_shuttingDownMutex.unlock();
			if(GD::bl->settings.changed())
			{
				if(GD::bl->settings.enableUPnP())
				{
					GD::out.printInfo("Stopping UPnP server");
					GD::uPnP->stop();
				}
				stopRPCServers(false);
				if(GD::mqtt->enabled())
				{
					GD::out.printInfo( "(Shutdown) => Stopping MQTT client");;
					GD::mqtt->stop();
				}
				if(GD::familyController) GD::familyController->physicalInterfaceStopListening();
				//Binding fails sometimes with "address is already in use" without waiting.
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				GD::out.printMessage("Reloading settings...");
				GD::bl->settings.load(GD::configPath + "main.conf");
				GD::clientSettings.load(GD::bl->settings.clientSettingsPath());
				GD::serverInfo.load(GD::bl->settings.serverSettingsPath());
				GD::mqtt->loadSettings();
				if(GD::mqtt->enabled())
				{
					GD::out.printInfo("Starting MQTT client");;
					GD::mqtt->start();
				}
				if(GD::familyController) GD::familyController->physicalInterfaceStartListening();
				startRPCServers();
				if(GD::bl->settings.enableUPnP())
				{
					GD::out.printInfo("Starting UPnP server");
					GD::uPnP->start();
				}
			}
			//Reopen log files, important for logrotate
			if(_startAsDaemon)
			{
				if(!std::freopen((GD::bl->settings.logfilePath() + "homegear.log").c_str(), "a", stdout))
				{
					GD::out.printError("Error: Could not redirect output to new log file.");
				}
				if(!std::freopen((GD::bl->settings.logfilePath() + "homegear.err").c_str(), "a", stderr))
				{
					GD::out.printError("Error: Could not redirect errors to new log file.");
				}
			}
			GD::bl->db->hotBackup();
			if(!GD::bl->db->isOpen())
			{
				GD::out.printCritical("Critical: Can't reopen database. Exiting...");
				exit(1);
			}
			GD::out.printInfo("Reloading script engine server...");
			GD::scriptEngineServer->homegearReloading();
			_shuttingDownMutex.lock();
			_startUpComplete = true;
			if(_shutdownQueued)
			{
				_shuttingDownMutex.unlock();
				terminate(SIGTERM);
			}
			_shuttingDownMutex.unlock();
			GD::out.printInfo("Info: Reload complete.");
		}
		else
		{
			if(!_disposing) GD::out.printCritical("Critical: Signal " + std::to_string(signalNumber) + " received. Stopping Homegear...");
			signal(signalNumber, SIG_DFL); //Reset signal handler for the current signal to default
			kill(getpid(), signalNumber); //Generate core dump
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void errorCallback(int32_t level, std::string message)
{
	if(GD::rpcClient) GD::rpcClient->broadcastError(level, message);
}

int32_t getIntInput()
{
	std::string input;
	std::cin >> input;
	int32_t intInput = -1;
	try	{ intInput = std::stoll(input); } catch(...) {}
    return intInput;
}

int32_t getHexInput()
{
	std::string input;
	std::cin >> input;
	int32_t intInput = -1;
	try	{ intInput = std::stoll(input, 0, 16); } catch(...) {}
    return intInput;
}

void getExecutablePath(int argc, char* argv[])
{
	char path[1024];
	if(!getcwd(path, sizeof(path)))
	{
		std::cerr << "Could not get working directory." << std::endl;
		exit(1);
	}
	GD::workingDirectory = std::string(path);
#ifdef KERN_PROC //BSD system
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	size_t cb = sizeof(path);
	int result = sysctl(mib, 4, path, &cb, NULL, 0);
	if(result == -1)
	{
		std::cerr << "Could not get executable path." << std::endl;
		exit(1);
	}
	path[sizeof(path) - 1] = '\0';
	GD::executablePath = std::string(path);
	GD::executablePath = GD::executablePath.substr(0, GD::executablePath.find_last_of("/") + 1);
#else
	int length = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (length < 0)
	{
		std::cerr << "Could not get executable path." << std::endl;
		exit(1);
	}
	if((unsigned)length > sizeof(path))
	{
		std::cerr << "The path the homegear binary is in has more than 1024 characters." << std::endl;
		exit(1);
	}
	path[length] = '\0';
	GD::executablePath = std::string(path);
	GD::executablePath = GD::executablePath.substr(0, GD::executablePath.find_last_of("/") + 1);
#endif

	GD::executableFile = std::string(argc > 0 ? argv[0] : "homegear");
	BaseLib::HelperFunctions::trim(GD::executableFile);
	if(GD::executableFile.empty()) GD::executableFile = "homegear";
	std::pair<std::string, std::string> pathNamePair = BaseLib::HelperFunctions::splitLast(GD::executableFile, '/');
	if(!pathNamePair.second.empty()) GD::executableFile = pathNamePair.second;
}

void initGnuTls()
{
	// {{{ Init gcrypt and GnuTLS
		gcry_error_t gcryResult;
		if((gcryResult = gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread)) != GPG_ERR_NO_ERROR)
		{
			GD::out.printCritical("Critical: Could not enable thread support for gcrypt.");
			exit(2);
		}

		if (!gcry_check_version(GCRYPT_VERSION))
		{
			GD::out.printCritical("Critical: Wrong gcrypt version.");
			exit(2);
		}
		gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
		if((gcryResult = gcry_control(GCRYCTL_INIT_SECMEM, 65536, 0)) != GPG_ERR_NO_ERROR)
		{
			GD::out.printCritical("Critical: Could not allocate secure memory.");
			exit(2);
		}
		gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
		gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

		int32_t gnutlsResult = 0;
		if((gnutlsResult = gnutls_global_init()) != GNUTLS_E_SUCCESS)
		{
			GD::out.printCritical("Critical: Could not initialize GnuTLS: " + std::string(gnutls_strerror(gnutlsResult)));
			exit(2);
		}
	// }}}
}

void setLimits()
{
	struct rlimit limits;
	if(!GD::bl->settings.enableCoreDumps()) prctl(PR_SET_DUMPABLE, 0);
	else
	{
		//Set rlimit for core dumps
		getrlimit(RLIMIT_CORE, &limits);
		limits.rlim_cur = limits.rlim_max;
		GD::out.printInfo("Info: Setting allowed core file size to \"" + std::to_string(limits.rlim_cur) + "\" for user with id " + std::to_string(getuid()) + " and group with id " + std::to_string(getgid()) + '.');
		setrlimit(RLIMIT_CORE, &limits);
		getrlimit(RLIMIT_CORE, &limits);
		GD::out.printInfo("Info: Core file size now is \"" + std::to_string(limits.rlim_cur) + "\".");
	}
#ifdef RLIMIT_RTPRIO //Not existant on BSD systems
	getrlimit(RLIMIT_RTPRIO, &limits);
	limits.rlim_cur = limits.rlim_max;
	GD::out.printInfo("Info: Setting maximum thread priority to \"" + std::to_string(limits.rlim_cur) + "\" for user with id " + std::to_string(getuid()) + " and group with id " + std::to_string(getgid()) + '.');
	setrlimit(RLIMIT_RTPRIO, &limits);
	getrlimit(RLIMIT_RTPRIO, &limits);
	GD::out.printInfo("Info: Maximum thread priority now is \"" + std::to_string(limits.rlim_cur) + "\".");
#endif
}

void printHelp()
{
	std::cout << "Usage: homegear [OPTIONS]" << std::endl << std::endl;
	std::cout << "Option\t\t\tMeaning" << std::endl;
	std::cout << "-h\t\t\tShow this help" << std::endl;
	std::cout << "-u\t\t\tRun as user" << std::endl;
	std::cout << "-g\t\t\tRun as group" << std::endl;
	std::cout << "-c <path>\t\tSpecify path to config file" << std::endl;
	std::cout << "-d\t\t\tRun as daemon" << std::endl;
	std::cout << "-p <pid path>\t\tSpecify path to process id file" << std::endl;
	std::cout << "-s <user> <group>\tSet GPIO settings and necessary permissions for all defined physical devices" << std::endl;
	std::cout << "-r\t\t\tConnect to Homegear on this machine" << std::endl;
	std::cout << "-e <command>\t\tExecute CLI command" << std::endl;
	std::cout << "-o <input> <output>\tConvert old device description file into new format." << std::endl;
	std::cout << "-v\t\t\tPrint program version" << std::endl;
}

void startMainProcess()
{
	try
	{
		_monitor.stop();
		_fork = false;
		_monitorProcess = false;
		_monitor.init();

		pid_t pid, sid;
		pid = fork();
		if(pid < 0)
		{
			exitHomegear(1);
		}
		else if(pid > 0)
		{
			_monitorProcess = true;
			_mainProcessId = pid;
			_monitor.prepareParent();
		}
		else
		{
			//Set process permission
			umask(S_IWGRP | S_IWOTH);

			//Set child processe's id
			sid = setsid();
			if(sid < 0)
			{
				exitHomegear(1);
			}

			_monitor.prepareChild();
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void startDaemon()
{
	try
	{
		pid_t pid, sid;
		pid = fork();
		if(pid < 0)
		{
			exitHomegear(1);
		}
		if(pid > 0)
		{
			exitHomegear(0);
		}

		//Set process permission
		umask(S_IWGRP | S_IWOTH);

		//Set child processe's id
		sid = setsid();
		if(sid < 0)
		{
			exitHomegear(1);
		}

		close(STDIN_FILENO);

		startMainProcess();
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void startUp()
{
	try
	{
		if((chdir(GD::bl->settings.workingDirectory().c_str())) < 0)
		{
			GD::out.printError("Could not change working directory to " + GD::bl->settings.workingDirectory() + ".");
			exitHomegear(1);
		}

    	struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = terminate;

    	//Enable printing of backtraces
		//Use sigaction over signal because of different behavior in Linux and BSD
    	sigaction(SIGHUP, &sa, NULL);
    	sigaction(SIGTERM, &sa, NULL);
#ifdef __aarch64__
    	sigaction(SIGABRT, &sa, NULL);
    	sigaction(SIGSEGV, &sa, NULL);
#else
    	std::unique_ptr<Debug::DeathHandler> deathHandler;
    	if(_startAsDaemon)
    	{
			deathHandler.reset(new Debug::DeathHandler());
			deathHandler->set_append_pid(true);
			deathHandler->set_frames_count(32);
			deathHandler->set_color_output(false);
			deathHandler->set_generate_core_dump(GD::bl->settings.enableCoreDumps());
			deathHandler->set_cleanup(false);
    	}
    	else
    	{
    		sigaction(SIGABRT, &sa, NULL);
    		sigaction(SIGSEGV, &sa, NULL);
    	}
#endif

    	sa.sa_handler = sigchld_handler;
    	sigaction(SIGCHLD, &sa, NULL);

    	if(_startAsDaemon)
		{
			if(!std::freopen((GD::bl->settings.logfilePath() + "homegear.log").c_str(), "a", stdout))
			{
				GD::out.printError("Error: Could not redirect output to log file.");
			}
			if(!std::freopen((GD::bl->settings.logfilePath() + "homegear.err").c_str(), "a", stderr))
			{
				GD::out.printError("Error: Could not redirect errors to log file.");
			}
		}

    	GD::out.printMessage("Starting Homegear...");
    	GD::out.printMessage(std::string("Homegear version ") + VERSION);
    	GD::out.printMessage(std::string("Git commit SHA of libhomegear-base: ") + GITCOMMITSHABASE);
    	GD::out.printMessage(std::string("Git branch of libhomegear-base:     ") + GITBRANCHBASE);
    	GD::out.printMessage(std::string("Git commit SHA of Homegear:         ") + GITCOMMITSHAHOMEGEAR);
    	GD::out.printMessage(std::string("Git branch of Homegear:             ") + GITBRANCHHOMEGEAR);

    	if(GD::bl->settings.memoryDebugging()) mallopt(M_CHECK_ACTION, 3); //Print detailed error message, stack trace, and memory, and abort the program. See: http://man7.org/linux/man-pages/man3/mallopt.3.html
    	if(_monitorProcess)
    	{
    		setLimits();

    		while(_monitorProcess)
    		{
    			std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    			if(_fork)
    			{
    				GD::out.printError("Homegear was terminated. Restarting (2)...");
    				startMainProcess();
    			}
    			_monitor.checkHealth(_mainProcessId);
    		}
    	}

    	initGnuTls();

		if(!GD::bl->io.directoryExists(GD::bl->settings.socketPath()))
		{
			if(!GD::bl->io.createDirectory(GD::bl->settings.socketPath(), S_IRWXU | S_IRWXG))
			{
				GD::out.printCritical("Critical: Directory \"" + GD::bl->settings.socketPath() + "\" does not exist and cannot be created.");
				exit(1);
			}
			uid_t userId = GD::bl->hf.userId(GD::runAsUser);
			gid_t groupId = GD::bl->hf.groupId(GD::runAsGroup);
			if((signed)userId != -1 && (signed)groupId != -1)
			{
				if(chown(GD::bl->settings.socketPath().c_str(), userId, groupId) == -1)
				{
					GD::out.printCritical("Critical: Could not set permissions on directory \"" + GD::bl->settings.socketPath() + "\"");
					exit(1);
				}
			}
		}

    	GD::licensingController->loadModules();

		GD::familyController->loadModules();

    	if(getuid() == 0 && !GD::runAsUser.empty() && !GD::runAsGroup.empty())
    	{
    		uid_t userId = GD::bl->hf.userId(GD::runAsUser);
			gid_t groupId = GD::bl->hf.groupId(GD::runAsGroup);
			if((signed)userId == -1 || (signed)groupId == -1)
			{
				GD::out.printCritical("Could not drop privileges. User name or group name is not valid.");
				exitHomegear(1);
			}
			GD::out.printInfo("Info: Settings up physical interfaces and GPIOs...");
			if(GD::familyController) GD::familyController->physicalInterfaceSetup(userId, groupId);
			BaseLib::Gpio gpio(GD::bl.get());
			gpio.setup(userId, groupId);
			GD::out.printInfo("Info: Dropping privileges to user " + GD::runAsUser + " (" + std::to_string(userId) + ") and group " + GD::runAsGroup + " (" + std::to_string(groupId) + ")");

			if(setgid(groupId) != 0)
			{
				GD::out.printCritical("Critical: Could not drop group privileges.");
				exitHomegear(1);
			}
			if(setuid(userId) != 0)
			{
				GD::out.printCritical("Critical: Could not drop user privileges.");
				exitHomegear(1);
			}

			//Core dumps are disabled by setuid. Enable them again.
			if(GD::bl->settings.enableCoreDumps()) prctl(PR_SET_DUMPABLE, 1);
    	}

    	if(getuid() == 0) GD::out.printWarning("Warning: Running as root. The authors of Homegear recommend running Homegear as user.");
    	else
    	{
    		if(setuid(0) != -1)
			{
				GD::out.printCritical("Critical: Regaining root privileges succeded. Exiting Homegear as this is a security risk.");
				exit(1);
			}
    		GD::out.printInfo("Info: Homegear is (now) running as user with id " + std::to_string(getuid()) + " and group with id " + std::to_string(getgid()) + '.');
    	}

    	setLimits();

    	//Create PID file
    	try
    	{
			if(!GD::pidfilePath.empty())
			{
				int32_t pidfile = open(GD::pidfilePath.c_str(), O_CREAT | O_RDWR, 0666);
				if(pidfile < 0)
				{
					GD::out.printError("Error: Cannot create pid file \"" + GD::pidfilePath + "\".");
				}
				else
				{
					int32_t rc = flock(pidfile, LOCK_EX | LOCK_NB);
					if(rc && errno == EWOULDBLOCK)
					{
						GD::out.printError("Error: Homegear is already running - Can't lock PID file.");
					}
					std::string pid(std::to_string(getpid()));
					int32_t bytesWritten = write(pidfile, pid.c_str(), pid.size());
					if(bytesWritten <= 0) GD::out.printError("Error writing to PID file: " + std::string(strerror(errno)));
					close(pidfile);
				}
			}
		}
		catch(const std::exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(BaseLib::Exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}

		if(!GD::bl->io.directoryExists(GD::bl->settings.tempPath()))
		{
			if(!GD::bl->io.createDirectory(GD::bl->settings.tempPath(), S_IRWXU | S_IRWXG))
			{
				GD::out.printCritical("Critical: Cannot create temp directory \"" + GD::bl->settings.tempPath());
				exit(1);
			}
		}
		std::vector<std::string> tempFiles = GD::bl->io.getFiles(GD::bl->settings.tempPath(), true);
		for(std::vector<std::string>::iterator i = tempFiles.begin(); i != tempFiles.end(); ++i)
		{
			if(!GD::bl->io.deleteFile(GD::bl->settings.tempPath() + *i))
			{
				GD::out.printCritical("Critical: deleting temporary file \"" + GD::bl->settings.tempPath() + *i + "\": " + strerror(errno));
			}
		}

		for(uint32_t i = 0; i < 100; ++i)
		{
			if(BaseLib::HelperFunctions::getTime() < 1000000000000)
			{
				GD::out.printWarning("Warning: Time is in the past. Waiting for ntp to set the time...");
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
			}
			else break;
		}
		if(BaseLib::HelperFunctions::getTime() < 1000000000000)
		{
			GD::out.printCritical("Critical: Time is still in the past. Check that ntp is setup correctly and your internet connection is working. Exiting...");
			exit(1);
		}

		GD::bl->db->init();
    	GD::bl->db->open(GD::bl->settings.databasePath(), GD::bl->settings.databaseSynchronous(), GD::bl->settings.databaseMemoryJournal(), GD::bl->settings.databaseWALJournal(), GD::bl->settings.databasePath() + ".bak");
    	if(!GD::bl->db->isOpen()) exitHomegear(1);

        GD::out.printInfo("Initializing database...");
        if(GD::bl->db->convertDatabase()) exitHomegear(0);
        GD::bl->db->initializeDatabase();

        #ifdef EVENTHANDLER
		GD::eventHandler.reset(new EventHandler());
		#endif
		if(!GD::bl->io.directoryExists(GD::bl->settings.tempPath() + "php"))
		{
			if(!GD::bl->io.createDirectory(GD::bl->settings.tempPath() + "php", S_IRWXU | S_IRWXG))
			{
				GD::out.printCritical("Critical: Cannot create temp directory \"" + GD::bl->settings.tempPath() + "php");
				exit(1);
			}
		}
		GD::out.printInfo("Starting script engine server...");
		GD::scriptEngineServer.reset(new ScriptEngine::ScriptEngineServer());
		if(!GD::scriptEngineServer->start())
		{
			GD::out.printCritical("Critical: Cannot start script engine server. Exiting Homegear.");
			exit(1);
		}

        GD::out.printInfo("Initializing licensing controller...");
        GD::licensingController->init();

        GD::out.printInfo("Loading licensing controller data...");
        GD::licensingController->load();

        GD::out.printInfo("Loading devices...");
        if(BaseLib::Io::fileExists(GD::configPath + "physicalinterfaces.conf")) GD::out.printWarning("Warning: File physicalinterfaces.conf exists in config directory. Interface configuration has been moved to " + GD::bl->settings.familyConfigPath());
        GD::familyController->load(); //Don't load before database is open!

        GD::out.printInfo("Start listening for packets...");
        GD::familyController->physicalInterfaceStartListening();
        if(!GD::familyController->physicalInterfaceIsOpen())
        {
        	GD::out.printCritical("Critical: At least one of the physical devices could not be opened... Exiting...");
        	GD::familyController->physicalInterfaceStopListening();
        	exitHomegear(1);
        }

        GD::out.printInfo("Initializing RPC client...");
        GD::rpcClient->init();

        if(GD::mqtt->enabled())
		{
			GD::out.printInfo("Starting MQTT client...");;
			GD::mqtt->start();
		}

        startRPCServers();

		GD::out.printInfo("Starting CLI server...");
		GD::cliServer.reset(new CLI::Server());
		GD::cliServer->start();

#ifdef EVENTHANDLER
        GD::out.printInfo("Initializing event handler...");
        GD::eventHandler->init();
        GD::out.printInfo("Loading events...");
        GD::eventHandler->load();
#endif
        _shuttingDownMutex.lock();
		_startUpComplete = true;
		if(_shutdownQueued)
		{
			_shuttingDownMutex.unlock();
			terminate(SIGTERM);
		}
		_shuttingDownMutex.unlock();
        GD::out.printMessage("Startup complete. Waiting for physical interfaces to connect.");

        //Wait for all interfaces to connect before setting booting to false
        {
			for(int32_t i = 0; i < 300; i++)
			{
				if(GD::familyController->physicalInterfaceIsOpen())
				{
					GD::out.printMessage("All physical interfaces are connected now.");
					break;
				}
				if(i == 299) GD::out.printError("Error: At least one physical interface is not connected.");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
        }

        if(GD::bl->settings.enableUPnP())
		{
        	GD::out.printInfo("Starting UPnP server...");
        	GD::uPnP->start();
		}

        GD::bl->booting = false;
        GD::familyController->homegearStarted();

		char* inputBuffer = nullptr;
        if(_startAsDaemon)
        {
        	while(true) std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        else
        {
        	rl_bind_key('\t', rl_abort); //no autocompletion
			while((inputBuffer = readline("")) != NULL)
			{
				if(inputBuffer[0] == '\n' || inputBuffer[0] == 0) continue;
				if(strncmp(inputBuffer, "quit", 4) == 0 || strncmp(inputBuffer, "exit", 4) == 0 || strncmp(inputBuffer, "moin", 4) == 0) break;

				add_history(inputBuffer); //Sets inputBuffer to 0

				std::string input(inputBuffer);
				std::cout << GD::cliServer->handleCommand(input);
				free(inputBuffer);
			}
			clear_history();

			terminate(SIGTERM);
        }
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

int main(int argc, char* argv[])
{
    try
    {
    	getExecutablePath(argc, argv);
    	_errorCallback.reset(new std::function<void(int32_t, std::string)>(errorCallback));
    	GD::bl.reset(new BaseLib::Obj(GD::executablePath, _errorCallback.get(), false));
    	GD::out.init(GD::bl.get());

		if(BaseLib::Io::directoryExists(GD::executablePath + "config")) GD::configPath = GD::executablePath + "config/";
		else if(BaseLib::Io::directoryExists(GD::executablePath + "cfg")) GD::configPath = GD::executablePath + "cfg/";
		else GD::configPath = "/etc/homegear/";

    	if(std::string(VERSION) != GD::bl->version())
    	{
    		GD::out.printCritical(std::string("Base library has wrong version. Expected version ") + VERSION + " but got version " + GD::bl->version());
    		exit(1);
    	}

    	for(int32_t i = 1; i < argc; i++)
    	{
    		std::string arg(argv[i]);
    		if(arg == "-h" || arg == "--help")
    		{
    			printHelp();
    			exit(0);
    		}
    		else if(arg == "-c")
    		{
    			if(i + 1 < argc)
    			{
    				std::string configPath = std::string(argv[i + 1]);
    				if(!configPath.empty()) GD::configPath = configPath;
    				if(GD::configPath[GD::configPath.size() - 1] != '/') GD::configPath.push_back('/');
    				i++;
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}
    		}
    		else if(arg == "-p")
    		{
    			if(i + 1 < argc)
    			{
    				GD::pidfilePath = std::string(argv[i + 1]);
    				i++;
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}
    		}
    		else if(arg == "-u")
    		{
    			if(i + 1 < argc)
    			{
    				GD::runAsUser = std::string(argv[i + 1]);
    				i++;
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}
    		}
    		else if(arg == "-g")
    		{
    			if(i + 1 < argc)
    			{
    				GD::runAsGroup = std::string(argv[i + 1]);
    				i++;
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}
    		}
    		else if(arg == "-s")
    		{
    			if(i + 2 < argc)
    			{
    				if(getuid() != 0)
    				{
    					std::cout <<  "Please run Homegear as root to set the device permissions." << std::endl;
    					exit(1);
    				}
    				GD::bl->settings.load(GD::configPath + "main.conf");
    				GD::bl->debugLevel = 3; //Only output warnings.
    				GD::licensingController.reset(new LicensingController());
    				GD::familyController.reset(new FamilyController());
    				GD::licensingController->loadModules();
    				GD::familyController->loadModules();
    				uid_t userId = GD::bl->hf.userId(std::string(argv[i + 1]));
    				gid_t groupId = GD::bl->hf.groupId(std::string(argv[i + 2]));
    				GD::out.printDebug("Debug: User ID set to " + std::to_string(userId) + " group ID set to " + std::to_string(groupId));
    				if((signed)userId == -1 || (signed)groupId == -1)
    				{
    					GD::out.printCritical("Could not setup physical devices. Username or group name is not valid.");
    					GD::familyController->dispose();
    					GD::licensingController->dispose();
    					exit(1);
    				}
    				GD::familyController->physicalInterfaceSetup(userId, groupId);
    				BaseLib::Gpio gpio(GD::bl.get());
    				gpio.setup(userId, groupId);
    				GD::familyController->dispose();
    				GD::licensingController->dispose();
    				exit(0);
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}
    		}
    		else if(arg == "-o")
    		{
    			if(i + 2 < argc)
    			{
    				GD::bl->settings.load(GD::configPath + "main.conf");
    				std::string inputFile(argv[i + 1]);
    				std::string outputFile(argv[i + 2]);
    				BaseLib::DeviceDescription::Devices devices(GD::bl.get(), nullptr, 0);
					std::shared_ptr<HomegearDevice> device = devices.load(inputFile);
					if(!device) exit(1);
					device->save(outputFile);
    				exit(0);
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}
    		}
    		else if(arg == "-d")
    		{
    			_startAsDaemon = true;
    		}
    		else if(arg == "-r")
    		{
#ifndef __aarch64__
    			Debug::DeathHandler deathHandler;
				deathHandler.set_append_pid(true);
				deathHandler.set_frames_count(32);
				deathHandler.set_color_output(false);
				deathHandler.set_generate_core_dump(GD::bl->settings.enableCoreDumps());
				deathHandler.set_cleanup(false);
#endif

    			GD::bl->settings.load(GD::configPath + "main.conf");
    			CLI::Client cliClient;
    			int32_t exitCode = cliClient.start();
    			exit(exitCode);
    		}
    		else if(arg == "-rse")
    		{
#ifndef __aarch64__
    			Debug::DeathHandler deathHandler;
				deathHandler.set_append_pid(true);
				deathHandler.set_frames_count(32);
				deathHandler.set_color_output(false);
				deathHandler.set_generate_core_dump(GD::bl->settings.enableCoreDumps());
				deathHandler.set_cleanup(false);
#endif

    			initGnuTls();
    			setLimits();
    			GD::bl->settings.load(GD::configPath + "main.conf");
    			GD::licensingController.reset(new LicensingController());
    			GD::licensingController->loadModules();
    			GD::licensingController->init();
    			GD::licensingController->load();
    			ScriptEngine::ScriptEngineClient scriptEngineClient;
    			scriptEngineClient.start();
    			GD::licensingController->dispose();
    			exit(0);
    		}
    		else if(arg == "-e")
    		{
    			GD::bl->settings.load(GD::configPath + "main.conf");
    			GD::bl->debugLevel = 3; //Only output warnings.
    			std::stringstream command;
    			if(i + 1 < argc)
    			{
    				command << std::string(argv[i + 1]);
    			}
    			else
    			{
    				printHelp();
    				exit(1);
    			}

    			for(int32_t j = i + 2; j < argc; j++)
    			{
    				std::string element(argv[j]);
    				if(element.find(' ') != std::string::npos) command << " \"" << element << "\"";
    				else command << " " << argv[j];
    			}

    			CLI::Client cliClient;
    			int32_t exitCode = cliClient.start(command.str());
    			exit(exitCode);
    		}
    		else if(arg == "-tc")
    		{
    			GD::bl->threadManager.testMaxThreadCount();
    			std::cout << GD::bl->threadManager.getMaxThreadCount() << std::endl;
    			exit(0);
    		}
    		else if(arg == "-pre")
    		{
    			GD::bl->settings.load(GD::configPath + "main.conf");
    			if(GD::runAsUser.empty()) GD::runAsUser = GD::bl->settings.runAsUser();
				if(GD::runAsGroup.empty()) GD::runAsGroup = GD::bl->settings.runAsGroup();
				uid_t userId = GD::bl->hf.userId(GD::runAsUser);
				gid_t groupId = GD::bl->hf.groupId(GD::runAsGroup);
				if((!GD::runAsUser.empty() && GD::runAsGroup.empty()) || (!GD::runAsGroup.empty() && GD::runAsUser.empty()))
				{
					GD::out.printCritical("Critical: You only provided a user OR a group for Homegear to run as. Please specify both.");
					exit(1);
				}
				std::string currentPath;
    			if(!GD::pidfilePath.empty() && GD::pidfilePath.find('/') != std::string::npos)
    			{
    				currentPath = GD::pidfilePath.substr(0, GD::pidfilePath.find_last_of('/'));
    				if(!currentPath.empty())
    				{
    					if(!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
    					if(chmod(currentPath.c_str(), S_IRWXU | S_IRWXG) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
    					if(chown(currentPath.c_str(), userId, groupId) == -1) std::cerr << "Could not set owner on " << currentPath << std::endl;
    				}
    			}

    			currentPath = GD::bl->settings.databasePath().substr(0, GD::bl->settings.databasePath().find_last_of('/'));
    			if(!currentPath.empty())
				{
					if(!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
					if(chmod(currentPath.c_str(), S_IRWXU | S_IRWXG) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
					if(chown(currentPath.c_str(), userId, groupId) == -1) std::cerr << "Could not set owner on " << currentPath << std::endl;
				}
    			if(BaseLib::Io::fileExists(GD::bl->settings.databasePath()))
    			{
    				if(chmod(GD::bl->settings.databasePath().c_str(), S_IRUSR | S_IWUSR | S_IRGRP) == -1) std::cerr << "Could not set permissions on " << GD::bl->settings.databasePath() << std::endl;
    			}

    			currentPath = GD::bl->settings.scriptPath();
    			if(!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
    			if(chmod(currentPath.c_str(), S_IRWXU | S_IRWXG) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
    			if(chown(currentPath.c_str(), userId, groupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;

    			currentPath = GD::bl->settings.socketPath();
    			if(!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
    			if(chmod(currentPath.c_str(), S_IRWXU | S_IRWXG) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
    			if(chown(currentPath.c_str(), userId, groupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;

    			currentPath = GD::bl->settings.modulePath();
    			if(!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP);
    			if(chmod(currentPath.c_str(), S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
    			if(chown(currentPath.c_str(), userId, groupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;

    			currentPath = GD::bl->settings.logfilePath();
    			if(!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
    			if(chmod(currentPath.c_str(), S_IRWXU | S_IRWXG) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
    			if(chown(currentPath.c_str(), userId, groupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;

    			exit(0);
    		}
    		else if(arg == "-v")
    		{
    			std::cout << "Homegear version " << VERSION << std::endl;
    			std::cout << "Copyright (c) 2013-2016 Sathya Laufer" << std::endl << std::endl;
    			std::cout << "Git commit SHA of libhomegear-base: " << GITCOMMITSHABASE << std::endl;
    			std::cout << "Git branch of libhomegear-base:     " << GITBRANCHBASE << std::endl;
    			std::cout << "Git commit SHA of Homegear:         " << GITCOMMITSHAHOMEGEAR << std::endl << std::endl;
    			std::cout << "Git branch of Homegear:             " << GITBRANCHHOMEGEAR << std::endl << std::endl;
    			std::cout << "PHP (License: PHP License):" << std::endl;
    			std::cout << "This product includes PHP software, freely available from <http://www.php.net/software/>" << std::endl;
    			std::cout << "Copyright (c) 1999-2016 The PHP Group. All rights reserved." << std::endl << std::endl;
    			std::cout << "DeathHandler (License: Simplified BSD License):" << std::endl;
    			std::cout << "Copyright (c) 2012, Samsung R&D Institute Russia. All rights reserved." << std::endl;

    			exit(0);
    		}
    		else
    		{
    			printHelp();
    			exit(1);
    		}
    	}

    	try
    	{
    		// {{{ Get maximum thread count
				std::string output;
				BaseLib::HelperFunctions::exec(GD::executablePath + "/" + GD::executableFile + " -tc", output);
				BaseLib::HelperFunctions::trim(output);
				if(BaseLib::Math::isNumber(output, false)) GD::bl->threadManager.setMaxThreadCount(BaseLib::Math::getNumber(output, false));
			// }}}
		}
		catch(const std::exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(BaseLib::Exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}

    	// {{{ Load settings
			GD::out.printInfo("Loading settings from " + GD::configPath + "main.conf");
			GD::bl->settings.load(GD::configPath + "main.conf");
			if(GD::runAsUser.empty()) GD::runAsUser = GD::bl->settings.runAsUser();
			if(GD::runAsGroup.empty()) GD::runAsGroup = GD::bl->settings.runAsGroup();
			if((!GD::runAsUser.empty() && GD::runAsGroup.empty()) || (!GD::runAsGroup.empty() && GD::runAsUser.empty()))
			{
				GD::out.printCritical("Critical: You only provided a user OR a group for Homegear to run as. Please specify both.");
				exit(1);
			}

			GD::out.printInfo("Loading RPC server settings from " + GD::bl->settings.serverSettingsPath());
			GD::serverInfo.init(GD::bl.get());
			GD::serverInfo.load(GD::bl->settings.serverSettingsPath());
			GD::out.printInfo("Loading RPC client settings from " + GD::bl->settings.clientSettingsPath());
			GD::clientSettings.load(GD::bl->settings.clientSettingsPath());
			GD::mqtt.reset(new Mqtt());
			GD::mqtt->loadSettings();
		// }}}

		if((chdir(GD::bl->settings.workingDirectory().c_str())) < 0)
		{
			GD::out.printError("Could not change working directory to " + GD::bl->settings.workingDirectory() + ".");
			exitHomegear(1);
		}

		GD::licensingController.reset(new LicensingController());
		GD::familyController.reset(new FamilyController());
		GD::bl->db.reset(new DatabaseController());
		GD::rpcClient.reset(new RPC::Client());

    	if(_startAsDaemon) startDaemon();
    	startUp();

        return 0;
    }
    catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	terminate(SIGTERM);

    return 1;
}

/* Copyright 2013-2017 Sathya Laufer
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

#include "IpcClient.h"
#include "GD/GD.h"

#include <sys/un.h>
#include <sys/socket.h>

namespace Ipc
{

IpcClient::IpcClient() : IQueue(GD::bl.get(), 1, 1000)
{
	_fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor);
	_out.init(GD::bl.get());

	_dummyClientInfo.reset(new BaseLib::RpcClientInfo());

	_closed = true;
	_stopped = true;

	_binaryRpc = std::unique_ptr<BaseLib::Rpc::BinaryRpc>(new BaseLib::Rpc::BinaryRpc(GD::bl.get()));
	_rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, false));
	_rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get(), true));
}

IpcClient::~IpcClient()
{
	dispose();
}

void IpcClient::dispose()
{
	try
	{
		if(_disposing) return;
		std::lock_guard<std::mutex> disposeGuard(_disposeMutex);
		_disposing = true;
		stop();
		_rpcResponses.clear();
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

void IpcClient::stop()
{
	try
	{
		if(_stopped) return;
		_stopped = true;
		if(_mainThread.joinable()) _mainThread.join();
		_closed = true;
		stopQueue(0);
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

void IpcClient::connect()
{
	try
	{
		for(int32_t i = 0; i < 2; i++)
		{
			_fileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0));
			if(!_fileDescriptor || _fileDescriptor->descriptor == -1)
			{
				_out.printError("Could not create socket.");
				return;
			}

			if(GD::bl->debugLevel >= 4 && i == 0) std::cout << "Info: Trying to connect..." << std::endl;
			sockaddr_un remoteAddress;
			remoteAddress.sun_family = AF_LOCAL;
			//104 is the size on BSD systems - slightly smaller than in Linux
			if(_socketPath.length() > 104)
			{
				//Check for buffer overflow
				_out.printCritical("Critical: Socket path is too long.");
				return;
			}
			strncpy(remoteAddress.sun_path, _socketPath.c_str(), 104);
			remoteAddress.sun_path[103] = 0; //Just to make sure it is null terminated.
			if(::connect(_fileDescriptor->descriptor, (struct sockaddr*)&remoteAddress, strlen(remoteAddress.sun_path) + 1 + sizeof(remoteAddress.sun_family)) == -1)
			{
				if(i == 0)
				{
					_out.printDebug("Debug: Socket closed. Trying again...");
					//When socket was not properly closed, we sometimes need to reconnect
					std::this_thread::sleep_for(std::chrono::milliseconds(2000));
					continue;
				}
				else
				{
					_out.printError("Could not connect to socket. Error: " + std::string(strerror(errno)));
					return;
				}
			}
			else break;
		}
		_closed = false;

		if(GD::bl->debugLevel >= 4) _out.printMessage("Connected.");
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

void IpcClient::start()
{
	try
	{
		_stopped = false;

		startQueue(0, 5, 0, SCHED_OTHER);

		_socketPath = GD::bl->settings.socketPath() + "homegearIPC.sock";
		if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Socket path is " + _socketPath);

		if(_mainThread.joinable()) _mainThread.join();
		_mainThread = std::thread(&IpcClient::mainThread, this);
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

void IpcClient::mainThread()
{
	try
	{
		connect();

		std::vector<char> buffer(1024);
		int32_t result = 0;
		int32_t bytesRead = 0;
		int32_t processedBytes = 0;
		while(!_stopped)
		{
			if(_closed)
			{
				connect();
				if(_closed || _fileDescriptor->descriptor == -1)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10000));
					continue;
				}
			}

			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 100000;
			fd_set readFileDescriptor;
			FD_ZERO(&readFileDescriptor);
			{
				auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
				fileDescriptorGuard.lock();
				FD_SET(_fileDescriptor->descriptor, &readFileDescriptor);
			}

			result = select(_fileDescriptor->descriptor + 1, &readFileDescriptor, NULL, NULL, &timeout);
			if(result == 0) continue;
			else if(result == -1)
			{
				if(errno == EINTR) continue;
				_out.printMessage("Connection to IPC server closed (1).");
				_closed = true;
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}

			bytesRead = read(_fileDescriptor->descriptor, &buffer[0], 1024);
			if(bytesRead <= 0) //read returns 0, when connection is disrupted.
			{
				_out.printMessage("Connection to IPC server closed (2).");
				_closed = true;
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}

			if(bytesRead > (signed)buffer.size()) bytesRead = buffer.size();

			try
			{
				processedBytes = 0;
				while(processedBytes < bytesRead)
				{
					processedBytes += _binaryRpc->process(&buffer[processedBytes], bytesRead - processedBytes);
					if(_binaryRpc->isFinished())
					{
						std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(_binaryRpc->getData(), _binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request));
						enqueue(0, queueEntry);
						_binaryRpc->reset();
					}
				}
			}
			catch(BaseLib::Rpc::BinaryRpcException& ex)
			{
				_out.printError("Error processing packet: " + ex.what());
				_binaryRpc->reset();
			}
		}
		buffer.clear();
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

void IpcClient::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
{
	try
	{
		if(_disposing) return;
		std::shared_ptr<QueueEntry> queueEntry;
		queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
		if(!queueEntry) return;

		if(queueEntry->isRequest)
		{
			std::string methodName;
			BaseLib::PArray parameters = _rpcDecoder->decodeRequest(queueEntry->packet, methodName);

			if(parameters->size() < 2)
			{
				_out.printError("Error: Wrong parameter count while calling method " + methodName);
				return;
			}
			std::map<std::string, std::function<BaseLib::PVariable(BaseLib::PArray& parameters)>>::iterator localMethodIterator = _localRpcMethods.find(methodName);
			if(localMethodIterator == _localRpcMethods.end())
			{
				_out.printError("Warning: RPC method not found: " + methodName);
				BaseLib::PVariable error = BaseLib::Variable::createError(-32601, ": Requested method not found.");
				sendResponse(parameters->at(0), error);
				return;
			}

			if(GD::bl->debugLevel >= 4) _out.printInfo("Info: Server is calling RPC method: " + methodName);

			BaseLib::PVariable result = localMethodIterator->second(parameters->at(1)->arrayValue);
			if(GD::bl->debugLevel >= 5)
			{
				_out.printDebug("Response: ");
				result->print(true, false);
			}
			sendResponse(parameters->at(0), result);
		}
		else
		{
			BaseLib::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
			if(response->arrayValue->size() < 3)
			{
				_out.printError("Error: Response has wrong array size.");
				return;
			}
			int32_t threadId = response->arrayValue->at(0)->integerValue;
			int32_t packetId = response->arrayValue->at(1)->integerValue;

			{
				std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
				auto responseIterator = _rpcResponses[threadId].find(packetId);
				if(responseIterator != _rpcResponses[threadId].end())
				{
					PIpcResponse element = responseIterator->second;
					if(element)
					{
						element->response = response;
						element->packetId = packetId;
						element->finished = true;
					}
				}
			}
			if(threadId != 0)
			{
				std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
				std::map<int32_t, PRequestInfo>::iterator requestIterator = _requestInfo.find(threadId);
				if(requestIterator != _requestInfo.end()) requestIterator->second->conditionVariable.notify_all();
			}
			else _requestConditionVariable.notify_all();
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

BaseLib::PVariable IpcClient::send(std::vector<char>& data)
{
	try
	{
		int32_t totallySentBytes = 0;
		std::lock_guard<std::mutex> sendGuard(_sendMutex);
		while (totallySentBytes < (signed)data.size())
		{
			int32_t sentBytes = ::send(_fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL);
			if(sentBytes <= 0)
			{
				if(errno == EAGAIN) continue;
				_out.printError("Could not send data to client " + std::to_string(_fileDescriptor->descriptor) + ". Sent bytes: " + std::to_string(totallySentBytes) + " of " + std::to_string(data.size()) + (sentBytes == -1 ? ". Error message: " + std::string(strerror(errno)) : ""));
				return BaseLib::Variable::createError(-32500, "Unknown application error.");
			}
			totallySentBytes += sentBytes;
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
    return BaseLib::PVariable(new BaseLib::Variable());
}

BaseLib::PVariable IpcClient::sendRequest(std::string methodName, BaseLib::PArray& parameters)
{
	try
	{
		int64_t threadId = pthread_self();
		PRequestInfo requestInfo;
		{
			std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
			requestInfo = _requestInfo[threadId];
		}
		int32_t packetId;
		{
			std::lock_guard<std::mutex> packetIdGuard(_packetIdMutex);
			packetId = _currentPacketId++;
		}
		BaseLib::PArray array(new BaseLib::Array{ BaseLib::PVariable(new BaseLib::Variable(threadId)), BaseLib::PVariable(new BaseLib::Variable(packetId)), BaseLib::PVariable(new BaseLib::Variable(parameters)) });
		std::vector<char> data;
		_rpcEncoder->encodeRequest(methodName, array, data);

		PIpcResponse response;
		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			auto result = _rpcResponses[threadId].emplace(packetId, std::make_shared<IpcResponse>());
			if(result.second) response = result.first->second;
		}
		if(!response)
		{
			_out.printError("Critical: Could not insert response struct into map.");
			return BaseLib::Variable::createError(-32500, "Unknown application error.");
		}

		BaseLib::PVariable result = send(data);
		if(result->errorStruct)
		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			_rpcResponses[threadId].erase(packetId);
			if(_rpcResponses[threadId].empty()) _rpcResponses.erase(threadId);
			return result;
		}

		std::unique_lock<std::mutex> waitLock(requestInfo->waitMutex);
		while(!requestInfo->conditionVariable.wait_for(waitLock, std::chrono::milliseconds(10000), [&]{
			return response->finished || _disposing;
		}));

		if(!response->finished || response->response->arrayValue->size() != 3 || response->packetId != packetId)
		{
			_out.printError("Error: No response received to RPC request. Method: " + methodName);
			result = BaseLib::Variable::createError(-1, "No response received.");
		}
		else result = response->response->arrayValue->at(2);

		{
			std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
			_rpcResponses[threadId].erase(packetId);
			if(_rpcResponses[threadId].empty()) _rpcResponses.erase(threadId);
		}

		return result;
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void IpcClient::sendResponse(BaseLib::PVariable& packetId, BaseLib::PVariable& variable)
{
	try
	{
		BaseLib::PVariable array(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{ packetId, variable })));
		std::vector<char> data;
		_rpcEncoder->encodeResponse(array, data);

		send(data);
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

// {{{ RPC methods
// }}}

}

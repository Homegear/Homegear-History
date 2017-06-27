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

IpcClient::IpcClient(std::string socketPath) : IIpcClient(socketPath)
{
	_localRpcMethods.emplace("historySetLogging", std::bind(&IpcClient::setLogging, this, std::placeholders::_1));
}

void IpcClient::onConnect()
{
	try
	{
		bool error = false;

		std::string methodName("registerRpcMethod");
		Ipc::PArray parameters = std::make_shared<Ipc::Array>();
		parameters->push_back(std::make_shared<Ipc::Variable>("historyTest1"));
		parameters->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tArray));
		Ipc::PVariable result = invoke(methodName, parameters);
		if (result->errorStruct)
		{
			error = true;
			GD::out.printCritical("Critical: Could not register RPC method test1: " + result->structValue->at("faultString")->stringValue);
		}

		parameters->at(0)->stringValue = "historyTest2";
		result = invoke(methodName, parameters);
		if (result->errorStruct)
		{
			error = true;
			GD::out.printCritical("Critical: Could not register RPC method test2: " + result->structValue->at("faultString")->stringValue);
		}

		if (error) return;

		GD::out.printInfo("Info: RPC methods successfully registered.");

		GD::history->load();
	}
	catch (const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (Ipc::IpcException& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

// {{{ RPC methods
Ipc::PVariable IpcClient::setLogging(Ipc::PArray& parameters)
{
	try
	{

		return std::make_shared<Ipc::Variable>();
	}
	catch (const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (Ipc::IpcException& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return Ipc::Variable::createError(-32500, "Unknown application error.");
}
// }}}

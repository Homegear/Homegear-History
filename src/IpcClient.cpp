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

IpcClient::IpcClient(std::string socketPath) : IIpcClient(GD::bl.get(), socketPath)
{
	_localRpcMethods.emplace("historyTest1", std::bind(&IpcClient::test1, this, std::placeholders::_1));
	_localRpcMethods.emplace("historyTest2", std::bind(&IpcClient::test2, this, std::placeholders::_1));
}

void IpcClient::registerRpcMethods()
{
	try
	{
		bool error = false;

		std::string methodName("registerRpcMethod");
		BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
		parameters->push_back(std::make_shared<BaseLib::Variable>("historyTest1"));
		parameters->push_back(std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray));
		BaseLib::PVariable result = invoke(methodName, parameters);
		if (result->errorStruct)
		{
			error = true;
			_out.printCritical("Critical: Could not register RPC method test1: " + result->structValue->at("faultString")->stringValue);
		}

		parameters->at(0)->stringValue = "historyTest2";
		result = invoke(methodName, parameters);
		if (result->errorStruct)
		{
			error = true;
			_out.printCritical("Critical: Could not register RPC method test2: " + result->structValue->at("faultString")->stringValue);
		}

		if (!error) _out.printInfo("Info: RPC methods successfully registered.");
	}
	catch (const std::exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (BaseLib::Exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

// {{{ RPC methods
BaseLib::PVariable IpcClient::test1(BaseLib::PArray& parameters)
{
	try
	{
		if (_disposing) return BaseLib::Variable::createError(-1, "Client is disposing.");

		_out.printInfo("Test1 called");

		return BaseLib::PVariable(new BaseLib::Variable(std::string("Test1 ") + std::to_string(BaseLib::HelperFunctions::getTimeSeconds())));
	}
	catch (const std::exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (BaseLib::Exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable IpcClient::test2(BaseLib::PArray& parameters)
{
	try
	{
		if (_disposing) return BaseLib::Variable::createError(-1, "Client is disposing.");

		_out.printInfo("Test2 called");

		return BaseLib::PVariable(new BaseLib::Variable(std::string("Test2 ") + std::to_string(BaseLib::HelperFunctions::getTimeSeconds())));
	}
	catch (const std::exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (BaseLib::Exception& ex)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}

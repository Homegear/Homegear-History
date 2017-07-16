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

		Ipc::PArray parameters = std::make_shared<Ipc::Array>();
		parameters->reserve(2);

		parameters->push_back(std::make_shared<Ipc::Variable>("historySetLogging"));
		parameters->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tArray)); //Outer array
		Ipc::PVariable signature = std::make_shared<Ipc::Variable>(Ipc::VariableType::tArray); //Inner array (= signature)
		signature->arrayValue->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tVoid)); //Return value
		signature->arrayValue->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tInteger64)); //1st parameter
		signature->arrayValue->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tInteger64)); //2nd parameter
		signature->arrayValue->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tString)); //3rd parameter
		signature->arrayValue->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tBoolean)); //4th parameter
		parameters->back()->arrayValue->push_back(signature);

		Ipc::PVariable result = invoke("registerRpcMethod", parameters);
		if (result->errorStruct)
		{
			error = true;
			Ipc::Output::printCritical("Critical: Could not register RPC method historySetLogging: " + result->structValue->at("faultString")->stringValue);
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
		if(parameters->size() != 4) return Ipc::Variable::createError(-1, "Wrong parameter count.");
		if(parameters->at(0)->type != Ipc::VariableType::tInteger || parameters->at(0)->type != Ipc::VariableType::tInteger64) return Ipc::Variable::createError(-1, "Parameter 1 is not of type integer.");
		if(parameters->at(1)->type != Ipc::VariableType::tInteger || parameters->at(1)->type != Ipc::VariableType::tInteger64) return Ipc::Variable::createError(-1, "Parameter 2 is not of type integer.");
		if(parameters->at(2)->type != Ipc::VariableType::tString) return Ipc::Variable::createError(-1, "Parameter 3 is not of type string.");
		if(parameters->at(2)->stringValue.empty()) return Ipc::Variable::createError(-1, "Parameter 3 is an empty string.");
		if(parameters->at(3)->type != Ipc::VariableType::tBoolean) return Ipc::Variable::createError(-1, "Parameter 4 is not of type boolean.");

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

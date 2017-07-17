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

#include "Database.h"
#include "GD/GD.h"

Database::Database(BaseLib::SharedObjects* bl) : IQueue(bl, 1, 100000)
{
	_rpcDecoder = std::unique_ptr<Ipc::RpcDecoder>(new Ipc::RpcDecoder());
	_rpcEncoder = std::unique_ptr<Ipc::RpcEncoder>(new Ipc::RpcEncoder(true));

	startQueue(0, false, 1, 0, SCHED_OTHER);
}

Database::~Database()
{
	stopQueue(0);
	_db.dispose();
}

std::string Database::getTableName(uint64_t peerId, int32_t channel, std::string& variable)
{
	std::string channelString = (channel < 0 ? "n" : std::to_string(channel));
	return "history_" + std::to_string(peerId) + "_" + channelString + "_" + stripNonAlphaNumeric(variable);
}

std::string Database::stripNonAlphaNumeric(const std::string& s)
{
	std::string strippedString;
	strippedString.reserve(s.size());
	for(std::string::const_iterator i = s.begin(); i != s.end(); ++i)
	{
		if(isalpha(*i) || isdigit(*i)) strippedString.push_back(*i);
	}
	return strippedString;
}

//{{{ General
	void Database::open(std::string databasePath, std::string databaseFilename, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal, std::string backupPath, std::string backupFilename)
	{
		_db.init(databasePath, databaseFilename, databaseSynchronous, databaseMemoryJournal, databaseWALJournal, backupPath, backupFilename);
	}

	void Database::hotBackup()
	{
		_db.hotBackup();
	}

	void Database::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
	{
		std::shared_ptr<QueueEntry> queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
		if(!queueEntry) return;
		_db.executeWriteCommand(queueEntry->getEntry());
	}

	void Database::createSavepointSynchronous(std::string& name)
	{
		if(GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Creating savepoint (synchronous) " + name);
		BaseLib::Database::DataRow data;
		_db.executeWriteCommand("SAVEPOINT " + name, data);
	}

	void Database::releaseSavepointSynchronous(std::string& name)
	{
		if(GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Releasing savepoint (synchronous) " + name);
		BaseLib::Database::DataRow data;
		_db.executeWriteCommand("RELEASE " + name, data);
	}

	void Database::createSavepointAsynchronous(std::string& name)
	{
		if(GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Creating savepoint (asynchronous) " + name);
		BaseLib::Database::DataRow data;
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("SAVEPOINT " + name, data);
		enqueue(0, entry);
	}

	void Database::releaseSavepointAsynchronous(std::string& name)
	{
		if(GD::bl->debugLevel > 5) GD::out.printDebug("Debug: Releasing savepoint (asynchronous) " + name);
		BaseLib::Database::DataRow data;
		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("RELEASE " + name, data);
		enqueue(0, entry);
	}
//}}}

//{{{ History
	std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>> Database::getVariables()
	{
		try
		{
			std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>> variables;

			std::shared_ptr<BaseLib::Database::DataTable> result = _db.executeCommand("SELECT name FROM sqlite_master WHERE type='table'");

			for(auto& row : *result)
			{
				if(row.second.at(0)->textValue.empty()) continue;
				std::vector<std::string> fields = BaseLib::HelperFunctions::splitAll(row.second.at(0)->textValue, '_');
				if(fields.size() != 4 || fields.at(1).empty() || fields.at(2).empty() || fields.at(3).empty()) continue;
				uint64_t peerId = BaseLib::Math::getNumber64(fields.at(1));
				int32_t channel = fields.at(2) == "n" ? -1 : BaseLib::Math::getNumber(fields.at(2));
				variables[peerId][channel].emplace(fields.at(3));
			}

			return variables;
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
		return std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>>();
	}

	void Database::deleteVariableTable(uint64_t peerId, int32_t channel, std::string variable)
	{
		std::string tableName = getTableName(peerId, channel, variable);

		_db.executeCommand("DROP INDEX IF EXISTS " + tableName + "Index");
		_db.executeCommand("DROP TABLE IF EXISTS " + tableName);
	}

	void Database::createVariableTable(uint64_t peerId, int32_t channel, std::string variable)
	{
		std::string tableName = getTableName(peerId, channel, variable);

		_db.executeCommand("CREATE TABLE IF NOT EXISTS " + tableName + " (time INTEGER NOT NULL, integerValue INTEGER, floatValue REAL, binaryValue BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS " + tableName + "Index ON " + tableName + " (time)");
	}

	void Database::saveValue(uint64_t peerId, int32_t channel, std::string& variable, Ipc::PVariable value)
	{
		if(variable.empty() || !value) return;

		std::string tableName = getTableName(peerId, channel, variable);

		BaseLib::Database::DataRow data;
		data.push_back(std::make_shared<BaseLib::Database::DataColumn>(Ipc::HelperFunctions::getTime()));
		if(value->type == Ipc::VariableType::tInteger || value->type == Ipc::VariableType::tInteger64)
		{
			data.push_back(std::make_shared<BaseLib::Database::DataColumn>(value->integerValue64));
		}
		else data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

		if(value->type == Ipc::VariableType::tFloat)
		{
			data.push_back(std::make_shared<BaseLib::Database::DataColumn>(value->floatValue));
		}
		else data.push_back(std::make_shared<BaseLib::Database::DataColumn>());

		std::vector<char> encodedValue;
		if(value->type != Ipc::VariableType::tInteger && value->type != Ipc::VariableType::tInteger64 && value->type != Ipc::VariableType::tFloat)
		{
			_rpcEncoder->encodeResponse(value, encodedValue);
		}
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(encodedValue)));

		std::shared_ptr<BaseLib::IQueueEntry> entry = std::make_shared<QueueEntry>("INSERT INTO " + tableName + " VALUES(?, ?, ?, ?)", data);
		enqueue(0, entry);
	}
//}}}

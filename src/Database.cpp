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
	startQueue(0, false, 1, 0, SCHED_OTHER);
}

Database::~Database()
{
	stopQueue(0);
	_db.dispose();
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
	void Database::deleteVariableTable(uint64_t peerId, int32_t channel, std::string variable)
	{
		std::string tableName = "history" + std::to_string(peerId) + "." + std::to_string(channel) + "." + variable;

		_db.executeCommand("DROP INDEX IF EXISTS " + tableName + "Index");
		_db.executeCommand("DROP TABLE IF EXISTS " + tableName);
	}

	void Database::createVariableTable(uint64_t peerId, int32_t channel, std::string variable)
	{
		std::string tableName = "history" + std::to_string(peerId) + "." + std::to_string(channel) + "." + variable;

		_db.executeCommand("CREATE TABLE IF NOT EXISTS " + tableName + " (time INTEGER NOT NULL, integerValue INTEGER, floatValue REAL, binaryValue BLOB)");
		_db.executeCommand("CREATE INDEX IF NOT EXISTS " + tableName + "Index ON " + tableName + " (time)");
	}
//}}}

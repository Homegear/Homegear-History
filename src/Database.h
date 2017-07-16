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

#ifndef DATABASECONTROLLER_H_
#define DATABASECONTROLLER_H_

#include "homegear-base/BaseLib.h"
#include "SQLite3.h"

class Database : public BaseLib::IQueue
{
public:
	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry(std::string command, BaseLib::Database::DataRow& data) { _entry = std::make_shared<std::pair<std::string, BaseLib::Database::DataRow>>(command, data); };
		virtual ~QueueEntry() {};
		std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>>& getEntry() { return _entry; }
	private:
		std::shared_ptr<std::pair<std::string, BaseLib::Database::DataRow>> _entry;
	};

	Database(BaseLib::SharedObjects* bl);
	virtual ~Database();

	// {{{ General
		void open(std::string databasePath, std::string databaseFilename, bool databaseSynchronous, bool databaseMemoryJournal, bool databaseWALJournal, std::string backupPath = "", std::string backupFilename = "");
		void hotBackup();
		bool isOpen() { return _db.isOpen(); }
		void createSavepointSynchronous(std::string& name);
		void releaseSavepointSynchronous(std::string& name);
		void createSavepointAsynchronous(std::string& name);
		void releaseSavepointAsynchronous(std::string& name);
	// }}}

	// {{{ History
		void deleteVariableTable(uint64_t peerId, int32_t channel, std::string variable);
		void createVariableTable(uint64_t peerId, int32_t channel, std::string variable);
	// }}}
protected:
	SQLite3 _db;

	virtual void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);
};

#endif

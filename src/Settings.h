/* Copyright 2013-2017 Sathya Laufer
 *
 * libhomegear-base is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * libhomegear-base is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with libhomegear-base.  If not, see
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

#ifndef HISTORYSETTINGS_H_
#define HISTORYSETTINGS_H_

#include <homegear-base/BaseLib.h>

class Settings
{
public:
	Settings();
	virtual ~Settings() {}
	void load(std::string filename, std::string executablePath);
	bool changed();

	std::string runAsUser() { return _runAsUser; }
	std::string runAsGroup() { return _runAsGroup; }
	int32_t debugLevel() { return _debugLevel; }
	bool memoryDebugging() { return _memoryDebugging; }
	bool enableCoreDumps() { return _enableCoreDumps; };
	std::string workingDirectory() { return _workingDirectory; }
	std::string socketPath() { return _socketPath; }
	std::string logfilePath() { return _logfilePath; }
	std::string historyPath() { return _historyPath; }
	uint32_t secureMemorySize() { return _secureMemorySize; }
	bool databaseSynchronous() { return _databaseSynchronous; }
	bool databaseMemoryJournal() { return _databaseMemoryJournal; }
	bool databaseWALJournal() { return _databaseWALJournal; }
	std::string databasePath() { return _databasePath; }
	std::string databaseBackupPath() { return _databaseBackupPath; }
	uint32_t databaseMaxBackups() { return _databaseMaxBackups; }
private:
	std::string _executablePath;
	std::string _path;
	int32_t _lastModified = -1;

	std::string _runAsUser;
	std::string _runAsGroup;
	int32_t _debugLevel = 3;
	bool _memoryDebugging = false;
	bool _enableCoreDumps = true;
	std::string _workingDirectory;
	std::string _socketPath;
	std::string _logfilePath;
	std::string _historyPath;
	uint32_t _secureMemorySize = 65536;
	bool _databaseSynchronous = true;
	bool _databaseMemoryJournal = false;
	bool _databaseWALJournal = true;
	std::string _databasePath;
	std::string _databaseBackupPath;
	uint32_t _databaseMaxBackups = 2;

	void reset();
};
#endif

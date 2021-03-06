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

#ifndef GD_H_
#define GD_H_

#include "../IpcClient.h"
#include "../Settings.h"
#include "../Database.h"

#include <homegear-base/BaseLib.h>

class GD
{
public:
	static std::unique_ptr<BaseLib::SharedObjects> bl;
	static BaseLib::Output out;
	static std::string runAsUser;
	static std::string runAsGroup;
	static std::string configPath;
	static std::string pidfilePath;
	static std::string workingDirectory;
	static std::string executablePath;
	static std::string executableFile;
	static int64_t startingTime;
	static std::unique_ptr<IpcClient> ipcClient;
	static std::unique_ptr<Database> db;
	static Settings settings;

	virtual ~GD() {}
private:
	//Non public constructor
	GD();
};

#endif /* GD_H_ */

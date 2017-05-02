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

#ifndef HISTORY_H_
#define HISTORY_H_

#include <homegear-base/BaseLib.h>

#include <rrd.h>

class History
{
public:
	History();

	void load();
private:
	typedef std::string VariableName;
	typedef int32_t Channel;
	typedef uint64_t PeerId;
	typedef std::set<VariableName> Variables;
	typedef std::unordered_map<Channel, Variables> Channels;
	typedef std::unordered_map<PeerId, Channels> Peers;

	std::mutex _variablesMutex;
	Peers _variables;

	void createRrdFile(uint64_t peerId, int32_t channel, std::string variable, uint32_t sampleInterval, uint32_t heartbeat, uint32_t aggregatedValuesDay, uint32_t aggregatedValuesWeek, uint32_t aggregatedValuesMonth, uint32_t aggregatedValuesYear, uint32_t years);
};

#endif
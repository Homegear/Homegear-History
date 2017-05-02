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

#include "History.h"
#include "GD/GD.h"

History::History()
{
}

void History::load()
{
	try
	{
		BaseLib::PArray parameters = BaseLib::PArray(new BaseLib::Array{ std::make_shared<BaseLib::Variable>("history-rrd") });
		BaseLib::PVariable data = GD::ipcClient->invoke("getData", parameters);
		if(data->errorStruct)
		{
			GD::out.printError("Error: Error retrieving data from Homegear: " + data->structValue->at("faultString")->stringValue);
			return;
		}

		std::lock_guard<std::mutex> variablesGuard(_variablesMutex);
		_variables.clear();

		for(auto& element : *data->structValue)
		{
			if(element.second->type != BaseLib::VariableType::tStruct || element.second->structValue->size() != 3) continue;
			auto idIterator = element.second->structValue->find("id");
			auto channelIterator = element.second->structValue->find("channel");
			auto variableIterator = element.second->structValue->find("variable");
			auto rrdIterator = element.second->structValue->find("rrdData");
			if(idIterator == element.second->structValue->end() || channelIterator == element.second->structValue->end() || variableIterator == element.second->structValue->end() || rrdIterator == element.second->structValue->end()) continue;

			_variables[idIterator->second->integerValue64][channelIterator->second->integerValue].emplace(variableIterator->second->stringValue);

			uint32_t sampleInterval = 300;
			uint32_t heartbeat = 86400;
			uint32_t aggregatedValuesDay = 1;
			uint32_t aggregatedValuesWeek = 3;
			uint32_t aggregatedValuesMonth = 12;
			uint32_t aggregatedValuesYear = 72;
			uint32_t years = 1;

			auto parameterIterator = rrdIterator->second->structValue->find("INTERVAL");
			if(parameterIterator != rrdIterator->second->structValue->end())
			{
				sampleInterval = parameterIterator->second->integerValue;
				if(sampleInterval == 0) sampleInterval = 1;
			}

			auto parameterIterator = rrdIterator->second->structValue->find("HEARTBEAT");
			if(parameterIterator != rrdIterator->second->structValue->end())
			{
				heartbeat = parameterIterator->second->integerValue;
				if(heartbeat == 0) sampleInterval = 1;
			}

			auto parameterIterator = rrdIterator->second->structValue->find("AGGREGATED_VALUES_DAY");
			if(parameterIterator != rrdIterator->second->structValue->end())
			{
				aggregatedValuesDay = parameterIterator->second->integerValue;
				if(aggregatedValuesDay == 0) aggregatedValuesDay = 1;
			}

			auto parameterIterator = rrdIterator->second->structValue->find("AGGREGATED_VALUES_WEEK");
			if(parameterIterator != rrdIterator->second->structValue->end())
			{
				aggregatedValuesWeek = parameterIterator->second->integerValue;
				if(aggregatedValuesWeek == 0) aggregatedValuesWeek = 1;
			}

			auto parameterIterator = rrdIterator->second->structValue->find("AGGREGATED_VALUES_MONTH");
			if(parameterIterator != rrdIterator->second->structValue->end())
			{
				aggregatedValuesMonth = parameterIterator->second->integerValue;
				if(aggregatedValuesMonth == 0) aggregatedValuesMonth = 1;
			}

			auto parameterIterator = rrdIterator->second->structValue->find("AGGREGATED_VALUES_YEAR");
			if(parameterIterator != rrdIterator->second->structValue->end())
			{
				aggregatedValuesYear = parameterIterator->second->integerValue;
				if(aggregatedValuesYear == 0) aggregatedValuesYear = 1;
			}

			auto parameterIterator = rrdIterator->second->structValue->find("YEARS");
			if(parameterIterator != rrdIterator->second->structValue->end())
			{
				years = parameterIterator->second->integerValue;
				if(years == 0) years = 1;
			}

			createRrdFile(idIterator->second->integerValue64, channelIterator->second->integerValue, variableIterator->second->stringValue, sampleInterval, heartbeat, aggregatedValuesDay, aggregatedValuesWeek, aggregatedValuesMonth, aggregatedValuesYear, years);
		}
	}
	catch (const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void History::createRrdFile(uint64_t peerId, int32_t channel, std::string variable, uint32_t sampleInterval, uint32_t heartbeat, uint32_t aggregatedValuesDay, uint32_t aggregatedValuesWeek, uint32_t aggregatedValuesMonth, uint32_t aggregatedValuesYear, uint32_t years)
{
	try
	{
		uint32_t rowsDay = 86400 / (aggregatedValuesDay * sampleInterval);
		uint32_t rowsWeek = 604800 / (aggregatedValuesWeek * sampleInterval);
		uint32_t rowsMonth = 2678400 / (aggregatedValuesMonth * sampleInterval);
		uint32_t rowsYear = 31622400 / (aggregatedValuesYear * sampleInterval) * years;

		//We need: Sample interval, heartbeat, (data source type), values per day, per week, per month, per year
		std::vector<char*> rrdParameters;
		rrdParameters.reserve(18);
		rrdParameters.push_back((char*)"create"); //Source: PHP module source code. Value doesn't seem to matter.
		std::string filename = std::to_string(peerId) + '.' + std::to_string(channel) + '.' + variable + ".rrd";
		BaseLib::HelperFunctions::stripNonAlphaNumeric(filename);
		filename = GD::settings.historyPath() + filename;
		rrdParameters.push_back((char*)filename.data());
		std::string step = "--step=" + std::to_string(sampleInterval);
		rrdParameters.push_back((char*)step.data());
		rrdParameters.push_back((char*)"--no-overwrite");
		std::string dataSource = "DS:" + (variable.size() > 19 ? variable.substr(0, 19) : variable) + ":GAUGE:" + std::to_string(heartbeat) + ":U:U";
		rrdParameters.push_back((char*)dataSource.data());
		std::string averageDay = "RRA:AVERAGE:0.5:" + std::to_string(aggregatedValuesDay) + ':' + std::to_string(rowsDay);
		rrdParameters.push_back((char*)averageDay.data());
		std::string averageWeek = "RRA:AVERAGE:0.5:" + std::to_string(aggregatedValuesWeek) + ':' + std::to_string(rowsWeek);
		rrdParameters.push_back((char*)averageWeek.data());
		std::string averageMonth = "RRA:AVERAGE:0.5:" + std::to_string(aggregatedValuesMonth) + ':' + std::to_string(rowsMonth);
		rrdParameters.push_back((char*)averageMonth.data());
		std::string averageYear = "RRA:AVERAGE:0.5:" + std::to_string(aggregatedValuesYear) + ':' + std::to_string(rowsYear);
		rrdParameters.push_back((char*)averageYear.data());
		std::string maxDay = "RRA:MAX:0.5:" + std::to_string(aggregatedValuesDay) + ':' + std::to_string(rowsDay);
		rrdParameters.push_back((char*)maxDay.data());
		std::string maxWeek = "RRA:MAX:0.5:" + std::to_string(aggregatedValuesWeek) + ':' + std::to_string(rowsWeek);
		rrdParameters.push_back((char*)maxWeek.data());
		std::string maxMonth = "RRA:MAX:0.5:" + std::to_string(aggregatedValuesMonth) + ':' + std::to_string(rowsMonth);
		rrdParameters.push_back((char*)maxMonth.data());
		std::string maxYear = "RRA:MAX:0.5:" + std::to_string(aggregatedValuesYear) + ':' + std::to_string(rowsYear);
		rrdParameters.push_back((char*)maxYear.data());
		std::string minDay = "RRA:MIN:0.5:" + std::to_string(aggregatedValuesDay) + ':' + std::to_string(rowsDay);
		rrdParameters.push_back((char*)minDay.data());
		std::string minWeek = "RRA:MIN:0.5:" + std::to_string(aggregatedValuesWeek) + ':' + std::to_string(rowsWeek);
		rrdParameters.push_back((char*)minWeek.data());
		std::string minMonth = "RRA:MIN:0.5:" + std::to_string(aggregatedValuesMonth) + ':' + std::to_string(rowsMonth);
		rrdParameters.push_back((char*)minMonth.data());
		std::string minYear = "RRA:MIN:0.5:" + std::to_string(aggregatedValuesYear) + ':' + std::to_string(rowsYear);
		rrdParameters.push_back((char*)minYear.data());
		rrdParameters.push_back(0); //Probably unnecessary
		int result = rrd_create(rrdParameters.size() - 1, rrdParameters.data());
		if(result == -1) GD::out.printError("Error: Could not create RRD file \"" + filename + "\": " + std::string(rrd_get_error()));
	}
	catch (const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

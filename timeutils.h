/**
 * @file
 *  This file is part of UTILS
 *
 * UTILS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * UTILS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with UTILS.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @copyright 2013-2014 Technische Universitaet Muenchen
 * @author Sebastian Rettenberger <rettenbs@in.tum.de>
 */

#ifndef UTILS_TIMEUTILS_H_
#define UTILS_TIMEUTILS_H_

#include <ctime>
#include <string>

/**
 * A collection of useful utility functions
 */
namespace utils
{

/**
 * A collection of usefull time functions
 */
class TimeUtils
{
public:
	/**
	 * Formats a string using strftime
	 *
	 * Taken from http://stackoverflow.com/questions/7935483/c-function-to-format-time-t-as-stdstring-buffer-length
	 *
	 * @return A copy of formatString, with all %k replaced with the time information
	 */
	static std::string timeAsString(const std::string& formatString, time_t time)
	{
	    const struct tm *timeinfo = localtime(&time);

	    std::string buffer;
	    buffer.resize(formatString.size()*2);
	    size_t len = strftime(&buffer[0], buffer.size(), formatString.c_str(), timeinfo);
	    while (len == 0) {
	        buffer.resize(buffer.size()*2);
	        len = strftime(&buffer[0], buffer.size(), formatString.c_str(), timeinfo);
	    }
	    buffer.resize(len);
	    return buffer;
	}
};

}

#endif /* UTILS_TIMEUTILS_H_ */

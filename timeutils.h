/**
 * @file
 *  This file is part of UTILS
 *
 *  For conditions of distribution and use, please see the copyright
 *  notice in the file 'COPYING' at the root directory of this package
 *  and the copyright notice at https://github.com/TUM-I5/utils
 *
 * @copyright 2013 Technische Universitaet Muenchen
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

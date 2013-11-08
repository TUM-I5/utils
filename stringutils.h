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

#ifndef UTILS_STRINGUTILS_H
#define UTILS_STRINGUTILS_H

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>
#include <string>

/**
 * A collection of useful utility functions
 */
namespace utils
{

/**
 * A collection of useful string functions based on std::string
 */
class StringUtils
{
public:
	/**
	 * Replaces from in str with to
	 *
	 * Taken from http://stackoverflow.com/questions/3418231/c-replace-part-of-a-string-with-another-string
	 */
	static bool replace(std::string& str, const std::string& from, const std::string& to) {
		size_t start_pos = str.find(from);
		if(start_pos == std::string::npos)
			return false;
		str.replace(start_pos, from.length(), to);
		return true;
	}

	/**
	 * Converts arbitrary datatypes (all datatypes which support the << stream
	 * operator) into std::string
	 */
	template<typename T>
	static std::string toString(T value)
	{
		std::ostringstream ss;
		ss << value;
		return ss.str();
	}

	/**
	 * Trims from start
	 *
	 * Taken from http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
	 */
	static std::string &ltrim(std::string &s) {
	        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	        return s;
	}

	/**
	 * Trims from end
	 *
	 * Taken from http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
	 */
	static std::string &rtrim(std::string &s) {
	        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	        return s;
	}

	/**
	 * Trims from both ends
	 *
	 * Taken from http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
	 */
	static std::string &trim(std::string &s) {
	        return ltrim(rtrim(s));
	}
};

}

#endif /* UTILS_STRINGUTILS_H */

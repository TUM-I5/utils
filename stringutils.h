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

#ifndef UTILS_STRINGUTILS_H
#define UTILS_STRINGUTILS_H

#include <algorithm>
#include <functional>
#include <cctype>
#include <cstring>
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
	 * Converts strings to arbitrary datatypes (using the << stream operator)
	 */
	template<typename T>
	static T parse(const std::string &str)
	{
		T result;
		std::istringstream(str) >> result;

		return result;
	}

	/**
	 * Converts null terminated string to upper case
	 */
	static void toUpper(char* s)
	{
		for (size_t i = 0; s[i]; i++)
			s[i] = toupper(s[i]);
	}

	/**
	 * Converts std::string to upper case
	 */
	static void toUpper(std::string &str)
	{
		for (size_t i = 0;  str[i]; i++)
			str[i] = toupper(str[i]);
	}

	/**
	 * Converts null terminated string to lower case
	 */
	static void toLower(char* s)
	{
		for (size_t i = 0; s[i]; i++)
			s[i] = tolower(s[i]);
	}

	/**
	 * Converts std::string to lower case
	 */
	static void toLower(std::string &str)
	{
		for (size_t i = 0; str[i]; i++)
			str[i] = tolower(str[i]);
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

	/**
	 * Join vector
	 *
	 * Taken from http://dracoater.blogspot.com/2010/03/joining-vector-and-splitting-string-in.html
	 */
	template<typename T>
	static std::string join(const std::vector<T> &v, const std::string &token) {
		std::ostringstream result;
		for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); i++){
			if (i != v.begin())
				result << token;
			result << *i;
		}

		return result.str();
	}
};

template<> inline
std::string StringUtils::parse(const std::string &str)
{
	return str;
}

}

#endif /* UTILS_STRINGUTILS_H */

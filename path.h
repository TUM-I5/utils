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
 * @copyright 2014 Technische Universitaet Muenchen
 * @author Sebastian Rettenberger <rettenbs@in.tum.de>
 */

#ifndef UTILS_PATH_H
#define UTILS_PATH_H

#include <string>

#include "utils/stringutils.h"

namespace utils
{

/**
 * Manipulates file/directory names and paths
 */
class Path
{
private:
	std::string m_path;

public:
	Path()
	{
	}

	Path(const char* path)
		: m_path(path)
	{
		init();
	}

	Path(const std::string &path)
		: m_path(path)
	{
		init();
	}

	/**
	 * @return The string representing the current path
	 */
	operator std::string() const
	{
		return m_path;
	}

	/**
	 * @return The basename of the path
	 */
	std::string basename() const
	{
		const size_t lastSlash = m_path.find_last_of(separators());
		if (lastSlash == std::string::npos)
			return m_path;

		return m_path.substr(lastSlash+1);
	}

	/**
	 * @return The directory name of the path
	 */
	std::string dirname() const
	{
		const size_t lastSlash =  m_path.find_last_of(separators());
		if (lastSlash == std::string::npos)
			return "";

		return m_path.substr(0, lastSlash);
	}

	/**
	 * @return The directory of the path
	 */
	Path dir() const
	{
		return Path(dirname());
	}

	/**
	 * Joins two paths
	 *
	 * @param other The path that should be appended (has to be relative)
	 * @return A path where other is appended to the current path
	 */
	Path operator+(const Path& other) const
	{
		if (m_path.empty())
			return other;
		if (other.m_path.empty())
			return *this;

		return Path(m_path + SEPARATOR + other.m_path);
	}

public:
	static const char* separator()
	{
		static const std::string sep(1, SEPARATOR);
		return sep.c_str();
	}

	/**
	 * @return A string containing all possible separators
	 */
	static const char* separators()
	{
#if __unix__
		return "/";
#else // __unix__
		return "\\/";
#endif // __unix__
	}

private:
	void init()
	{
		// Remove trailing separator
		if (StringUtils::endsWith(m_path, separator()))
			StringUtils::replaceLast(m_path, separator(), "");
	}

public:
#ifdef __unix__
	static const char SEPARATOR = '/';
#else // __unix__
	static const char SEPARATOR = '\\';
#endif // __unix__

};

}

#endif // UTILS_PATH_H

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

#ifndef UTILS_ENV_H
#define UTILS_ENV_H

#include <cstdlib>

#include "utils/stringutils.h"

namespace utils
{

/**
 * Function(s) to handle environment variables
 */
class Env
{
public:
	template<typename T>
	static T get(const char* name, T defaultVal)
	{
		char* value = getenv(name);

		if (!value)
			return defaultVal;

		return StringUtils::parse<T>(value);
	}
};

}

#endif // UTILS_ENV_H

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

#ifndef UTILS_MATHUTILS_H
#define UTILS_MATHUTILS_H

namespace utils
{

/**
 * A collection of useful math functions
 */
class MathUtils
{
public:
	/**
	 * Computes the greatest common divisor of a and b
	 *
	 * @param a
	 * @param b
	 * @return
	 */
	template<typename T>
	static T gcd(T a, T b)
	{
		T c = a % b;
		while(c != 0) {
			a = b;
			b = c;
			c = a % b;
		}
		return b;
	}
};

}

#endif // UTILS_ARGS_H

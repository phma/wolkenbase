/******************************************************/
/*                                                    */
/* roof.cpp - detect edge of roof                     */
/*                                                    */
/******************************************************/
/* Copyright 2020 Pierre Abbat.
 * This file is part of Wolkenbase.
 *
 * Wolkenbase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wolkenbase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wolkenbase. If not, see <http://www.gnu.org/licenses/>.
 */
#include "shape.h"

/* To detect a roof edge:
 * 1. Pick a point in the column.
 * 2. Find all points within 1 m of the point.
 * 3. For each point, multiply its horizontal distance by the cis of its
 *    bearing times the numbers 1 to 8, and add them to eight accumulators.
 * 4. If the odd harmonics are stronger than the even harmonics, and there are
 *    enough points, then you have detected a roof edge.
 * 5. Repeat until you have checked enough points or detected a roof edge.
 *
 * If the cells containing roof edges form a closed curve, the interior of the
 * curve is roof. For each roof or edge cell, search in the six directions for
 * a non-roof cell. Set the paraboloid size so that it goes down by 1 meter
 * in the longest direction.
 */

/******************************************************/
/*                                                    */
/* scan.h - scan the cloud                            */
/*                                                    */
/******************************************************/
/* Copyright 2020-2022 Pierre Abbat.
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
#include "tile.h"
#define M_SQRT7 2.6457513110645905905016

extern double minHyperboloidSize,maxSlope,thickness;

void scanCylinder(Eisenstein cylAddress);
void postscanCylinder(Eisenstein cylAddress);

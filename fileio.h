/******************************************************/
/*                                                    */
/* fileio.h - file I/O                                */
/*                                                    */
/******************************************************/
/* Copyright 2020,2022 Pierre Abbat.
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
#ifndef FILEIO_H
#define FILEIO_H
#include <string>

std::string noExt(std::string fileName);
std::string extension(std::string fileName);
std::string baseName(std::string fileName);
void deleteFile(std::string fileName);

#ifdef XYZ
int readCloud(std::string &inputFile,double inUnit,int flags);
#endif
#endif

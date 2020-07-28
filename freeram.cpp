/******************************************************/
/*                                                    */
/* freeram.cpp - free RAM                             */
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
#include "config.h"
#include "freeram.h"

#ifdef HAVE_SYS_SYSINFO_H
// Linux
#include <sys/sysinfo.h>

double freeRam()
/* mem_unit appeared in 2.3.23, which was released in 1999 or 2000.
 * Linux 2.2 is long obsolete.
 */
{
  struct sysinfo info;
  sysinfo(&info);
  return (double)info.freeram*info.mem_unit;
}

#endif


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
#include <cmath>
#include "config.h"
#include "freeram.h"
using namespace std;

#ifdef HAVE_PROC_MEMINFO
// Linux
#include <fstream>
#include <string>

double freeRam()
/* This returns the available RAM, which includes the cache and is
 * much greater than the free RAM from sysinfo.
 */
{
  ifstream meminfo("/proc/meminfo");
  string line,numstr;
  double available;
  while (meminfo.good())
  {
    getline(meminfo,line);
    if (line.find("MemAvailable:")==0)
    {
      numstr=line.substr(13);
      available=stod(numstr);
      break;
    }
  }
  return available*1024;
}

#else
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

#else
#ifdef HAVE_SYS_SYSCTL_H
// BSD. Linux has sys/sysctl.h, but it is deprecated.
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <unistd.h>

double freeRam()
{
  vmstats vms;
  size_t vms_size=sizeof(vms);
  double ret;
  int rval=sysctlbyname("vm.vmstats",&vms,&vms_size,nullptr,0);
  if (rval)
    ret=NAN;
  else
    ret=vms.v_free_count*(double)getpagesize();
  return ret;
}

#endif
#endif
#endif

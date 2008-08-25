#!/usr/bin/env python
# WinKexec: kexec for Windows
# Copyright (C) 2008 John Stumpo
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import sys

MacroList = []

def MakeHeader(prefix, macros):
  hdr = ('%sifndef REVISION_HEADER\n%sdefine REVISION_HEADER\n\n' %
    (prefix, prefix))
  for key, value in macros:
    hdr += '%sdefine %s %s\n' % (prefix, key, value)
  hdr += '\n%sendif\n' % prefix
  return hdr

for arg in sys.argv[1:]:
  key, equals, deps = arg.partition('=')
  highestrev = 0
  for dep in deps.split(','):
    try:
      rev = int(os.popen("svn info '%s' | grep ^Revision | egrep -o '[0-9]+'" %
        dep.replace("'", "'\"'\"'"), 'r').read())
    except ValueError:
      rev = 0
    if rev > highestrev:
      highestrev = rev
  MacroList.append(('%s_REVISION' % key, '%d' % highestrev))
  MacroList.append(('%s_REVISION_STR' % key, '"%d"' % highestrev))

HeaderContent = MakeHeader('#', MacroList)
NshContent = MakeHeader('!', MacroList)

try:
  OldHeaderContent = open('Revision.h', 'r').read()
except IOError:
  OldHeaderContent = None

try:
  OldNshContent = open('Revision.nsh', 'r').read()
except IOError:
  OldNshContent = None

if HeaderContent != OldHeaderContent:
  open('Revision.h', 'w').write(HeaderContent)

if NshContent != OldNshContent:
  open('Revision.nsh', 'w').write(NshContent)
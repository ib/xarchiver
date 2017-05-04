/*
 *  Copyright (C) Ingo Brückl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef XARCHIVER_PARSER_H
#define XARCHIVER_PARSER_H

#define GRAB_ITEM(item, cond)                              \
do                                                         \
{                                                          \
  while (*line && (*line == ' ' || *line == '\t')) line++; \
  item = line;                                             \
  while (*line && (cond)) line++;                          \
  if (*line) *line++ = 0;                                  \
}                                                          \
while (0)

#define NEXT_ITEM(item) GRAB_ITEM(item, *line != ' ' && *line != '\t' && *line != '\n')
#define LAST_ITEM(item) GRAB_ITEM(item, *line != '\n')
#define SKIP_ITEM NEXT_ITEM(line)

#endif

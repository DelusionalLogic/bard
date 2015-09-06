//Copyright (C) 2015 Jesper Jensen
//    This file is part of bard.
//
//    bard is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    bard is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with bard.  If not, see <http://www.gnu.org/licenses/>.
#ifndef ALIGN_H
#define ALIGN_H

static const char* const AlignStr[] = {
	"%{l}",
	"%{c}",
	"%{r}",
};

enum Align {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT,
};

#define ALIGN_NUM ((int)ALIGN_RIGHT+1)
#define ALIGN_FIRST ((int)ALIGN_LEFT)
#define ALIGN_LAST ((int)ALIGN_RIGHT)

#endif

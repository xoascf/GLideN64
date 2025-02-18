/*
 * Texture Filtering
 * Version:  1.0
 *
 * Copyright (C) 2007  Hiroshi Morii   All Rights Reserved.
 * Email koolsmoky(at)users.sourceforge.net
 * Web   http://www.3dfxzone.it/koolsmoky
 *
 * this is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * this is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define DBG_LEVEL 80

#include "TxDbg.h"
#include <stdarg.h>
#include <stdio.h>

#ifdef OS_ANDROID

#include <stdlib.h>
#include <android/log.h>

TxDbg::TxDbg()
{
	_level = DBG_LEVEL;
}

TxDbg::~TxDbg()
{}


void
TxDbg::output(const int level, const wchar_t *format, ...)
{
	if (level > _level)
		return;

	char fmt[2048];
	wcstombs(fmt, format, 2048);

	va_list ap;
	va_start(ap, format);
	__android_log_vprint(ANDROID_LOG_DEBUG, "GLideN64", fmt, ap);
	va_end(ap);
}

#else // OS_ANDROID
TxDbg::TxDbg()
{
	_level = DBG_LEVEL;

	if (!_dbgfile)
		_dbgfile = fopen("glidenhq.dbg", "w");
}

TxDbg::~TxDbg()
{
	if (_dbgfile) {
		fclose(_dbgfile);
		_dbgfile = 0;
	}

	_level = DBG_LEVEL;
}

void
TxDbg::output(const int level, const wchar_t *format, ...)
{
#if 0
	if (level > _level)
		return;

	va_list args;
	wchar_t newformat[4095];

	va_start(args, format);
	tx_swprintf(newformat, 4095, wst("%d:\t"), level);
	wcscat(newformat, format);
	vfwprintf(_dbgfile, newformat, args);
	fflush(_dbgfile);
	va_end(args);
#endif
}
#endif // OS_ANDROID

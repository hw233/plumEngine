/*
Copyright (C) 2017, Zhang Baofeng.

BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

You can contact the author via email : zhangbaofeng@joygame.cc
This is the common defines.
*/

#pragma once

namespace common{

	typedef int int32;
	typedef unsigned int uint32;
	typedef unsigned long long uint64;
	typedef long long int64;
	typedef short int16;
	typedef unsigned short uint16;
	typedef char int8;
	typedef unsigned char uint8;

	typedef uint8 t_proto;

#define __FooCrash() (*(int*)0)++;

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)	{if (p) {delete p; p = NULL;}}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)	{if (p) {delete[] p; p = NULL;}}
#endif

#ifndef ASSERT
#define ASSERT(exp) {assert(exp);}
#endif

#ifdef _WIN32
#define snprintf _snprintf
#endif

#define PLUMAPI extern

}

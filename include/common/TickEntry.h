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
This is list entry wraps the real ticks.
*/

#pragma once
#include "list.h"
#include "commdef.h"

namespace common
{
	class CTick;
	class CNewTick;
	// linux的list必须要做的entry，用以实现可以让list容纳不同类型的元素
	class stTickEntry
	{
	public:

		list_head TickListHead;		// liuxe list head
		uint64 uNextTickTime;		// 下次执行tick的时间
		bool bIsValid;
		CNewTick* pTick;

		static stTickEntry* CreateObj()	{ return new stTickEntry; }
		void Release() { delete this; }
		void Init()
		{
			uNextTickTime = 0;
			bIsValid = false;
			pTick = NULL;
			INIT_LIST_HEAD(&TickListHead);
		}

	private:
		stTickEntry() { Init(); }
		~stTickEntry() {};
	};


}
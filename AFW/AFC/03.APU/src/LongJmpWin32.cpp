
#include "LongJmpWin32.hpp"

#if defined(MSVC)

#if (_MSC_VER >= 1300)
#pragma warning( disable : 4733 )
#endif

FOCP_BEGIN();

APU_API __declspec(naked) int32 FocpSetJmp(CJmpBuf pBuf)
{
	__asm
	{
		//get return address
		pop ecx

		//get pointer to jmp_buf
		pop eax

		//fill jmp_buf
		mov edx, fs:[0]
		mov dword ptr[eax], ebp
		mov dword ptr[eax+4], ebx
		mov dword ptr[eax+8], edi
		mov dword ptr[eax+0ch], esi
		mov dword ptr[eax+10h], esp
		mov dword ptr[eax+14h], ecx
		mov dword ptr[eax+18h], edx
		xor eax,eax

		//fake setjmp arg for caller to pop
		sub esp, 4

		//return
		jmp ecx
	}
}

APU_API __declspec(naked) void FocpLongJmp(CJmpBuf pBuf, int32 nRet)
{
	__asm
	{
		//discard return address
		pop eax

		//get pointer to jmp_buf
		pop ecx

		//get return value
		pop eax

		//Restore registers
		mov ebp, dword ptr[ecx]
		mov ebx, dword ptr[ecx+4]
		mov edi, dword ptr[ecx+8]
		mov esi, dword ptr[ecx+0ch]
		mov esp, dword ptr[ecx+10h]
		mov edx, dword ptr[ecx+18h]
		mov ecx, dword ptr[ecx+14h]
		mov fs:[0], edx

		// fill return value
		//mov eax, eax

		//fake setjmp arg for caller to pop
		sub esp, 4

		//return
		jmp ecx
	}
}

FOCP_END();

#endif

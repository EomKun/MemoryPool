// main.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"

/*-------------------------------------------------------------------------------------------*/
//
// 락프리 모델 테스트 프로그램 제작
//
// - MemoryPool, Lockfree Stack, Lockfree Queue
//
// 여러개의 스레드에서 일정수량의 Alloc 과 Free 를 반복적으로 함
// 모든 데이터는 0x0000000055555555 으로 초기화 되어 있음.
//
// 사전에 100,000 개의 st_TEST_DATA  데이터를 메모리풀에 넣어둠. 
//  모든 데이터는 Data - 0x0000000055555555 / Cound - 0 초기화.
//
/*-------------------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------------------------------*/
// 락프리 메모리풀
/*-------------------------------------------------------------------------------------------*/
CMemoryPool<st_TEST_DATA>	g_MemoryPool;



/*-------------------------------------------------------------------------------------------*/
// 프로파일링 변수들
/*-------------------------------------------------------------------------------------------*/
long						lInTPS = 0;
long						lOutTPS = 0;

long						lInCounter = 0;
long						lOutCounter = 0;



/*-------------------------------------------------------------------------------------------*/
// 쓰레드 핸들
/*-------------------------------------------------------------------------------------------*/
HANDLE		hThread[dfTHREAD_MAX];
DWORD		dwThreadID;



/*===========================================================================================*/



/*-------------------------------------------------------------------------------------------*/
// 락프리 메모리풀 테스트 쓰레드
/*-------------------------------------------------------------------------------------------*/
unsigned __stdcall MemoryPoolThread(void *pParam)
{
	st_TEST_DATA *pDataArray[dfTHREAD_ALLOC];

	while (1){
		///////////////////////////////////////////////////////////////////////////////////////
		// Alloc (10000개)
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			pDataArray[iCnt] = g_MemoryPool.Alloc();
			InterlockedIncrement((long *)&lOutCounter);
		}

		///////////////////////////////////////////////////////////////////////////////////////
		// 0x0000000055555555 이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555555 != pDataArray[iCnt]->lData) ||
				(0 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// Interlocked + 1 (Data + 1 / Count + 1)
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedIncrement64((LONG64 *)&pDataArray[iCnt]->lData);
			InterlockedIncrement64((LONG64 *)&pDataArray[iCnt]->lCount);
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// 약간 대기
		///////////////////////////////////////////////////////////////////////////////////////
		Sleep(dfSLEEP);


		///////////////////////////////////////////////////////////////////////////////////////
		// 여전히 0x000000005555556, 1이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555556 != pDataArray[iCnt]->lData) ||
				(1 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// Interlocked - 1 (Data - 1 / Count - 1)
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedDecrement64((LONG64 *)&pDataArray[iCnt]->lData);
			InterlockedDecrement64((LONG64 *)&pDataArray[iCnt]->lCount);
		}


		///////////////////////////////////////////////////////////////////////////////////////
		// 약간 대기
		///////////////////////////////////////////////////////////////////////////////////////
		Sleep(dfSLEEP);


		///////////////////////////////////////////////////////////////////////////////////////
		// 0x0000000055555555 / 0 이 맞는지 확인
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((0x0000000055555555 != pDataArray[iCnt]->lData) ||
				(0 != pDataArray[iCnt]->lCount))
				CCrashDump::Crash();
		}



		///////////////////////////////////////////////////////////////////////////////////////
		// Free
		///////////////////////////////////////////////////////////////////////////////////////
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			g_MemoryPool.Free(pDataArray[iCnt]);
			InterlockedIncrement((long *)&lInCounter);
		}
	}
}


void			InitMemoryPool()
{
	st_TEST_DATA *pDataArray[dfDATA_MAX];

	///////////////////////////////////////////////////////////////////////////////////////////
	// Alloc (10000개)
	// 데이터 생성(확보)
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
		pDataArray[iCnt] = g_MemoryPool.Alloc();


	///////////////////////////////////////////////////////////////////////////////////////////
	// iData = 0x0000000055555555 셋팅
	// lCount = 0 셋팅
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
	{
		pDataArray[iCnt]->lData = 0x0000000055555555;
		pDataArray[iCnt]->lCount = 0;
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// 2. 메모리풀에 넣음(반환)
	///////////////////////////////////////////////////////////////////////////////////////////
	for (int iCnt = 0; iCnt < dfDATA_MAX; iCnt++)
		g_MemoryPool.Free(pDataArray[iCnt]);
}


void			TestMemoryPool()
{
	for (int iCnt = 0; iCnt < dfTHREAD_MAX; iCnt++)
	{
		hThread[iCnt] = (HANDLE)_beginthreadex(
			NULL,
			0,
			MemoryPoolThread,
			(LPVOID)0,
			0,
			(unsigned int *)&dwThreadID
			);
	}

	while (1)
	{
		lInTPS = lInCounter;
		lOutTPS = lOutCounter;

		lInCounter = 0;
		lOutCounter = 0;

		system("cls");
		wprintf(L"=====================================================================\n");
		wprintf(L"                        MemoryPool Testing...                        \n");
		wprintf(L"=====================================================================\n\n");

		wprintf(L"---------------------------------------------------------------------\n\n");
		wprintf(L"Alloc TPS		: %ld\n", lOutTPS);
		wprintf(L"Free  TPS		: %ld\n", lInTPS);
		wprintf(L"Alloc TPS		: %ld\n", g_MemoryPool.GetAllocCount());
		wprintf(L"---------------------------------------------------------------------\n\n\n");
		if (g_MemoryPool.GetAllocCount() > (dfTHREAD_MAX * dfTHREAD_ALLOC))
			CCrashDump::Crash();

		Sleep(999);
	}

}

void			TestLockfreeStack()
{

}

void			TestLockfreeQueue()
{

}

int				_tmain(int argc, _TCHAR* argv[])
{
	CCrashDump::CCrashDump();

	char		chSelectModel;

	do
	{
		system("cls");

		wprintf(L"=====================================================================\n");
		wprintf(L"                         Lockfree Model Test                         \n");
		wprintf(L"=====================================================================\n\n");

		wprintf(L"1. Lockfree MemoryPool\n\n");
		wprintf(L"2. Lockfree Stack\n\n");
		wprintf(L"3. Lockfree Queue\n\n");

		wprintf(L"=====================================================================\n");

		scanf_s("%c", &chSelectModel, sizeof(chSelectModel));
		fflush(stdin);

		switch (chSelectModel)
		{
		case '1':
			InitMemoryPool();
			TestMemoryPool();
			break;

		case '2':
			TestLockfreeStack();
			break;

		case '3':
			TestLockfreeQueue();
			break;

		default:
			continue;
			break;
		}

	} while (0);

	return 0;
}
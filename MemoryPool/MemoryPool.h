/*---------------------------------------------------------------

procademy MemoryPool.

메모리 풀 클래스.
특정 데이타(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

- 사용법.

procademy::CMemoryPool<DATA> MemPool(300, FALSE);
DATA *pData = MemPool.Alloc();

pData 사용

MemPool.Free(pData);


!.	아주 자주 사용되어 속도에 영향을 줄 메모리라면 생성자에서
Lock 플래그를 주어 페이징 파일로 복사를 막을 수 있다.
아주 중요한 경우가 아닌이상 사용 금지.



주의사항 :	단순히 메모리 사이즈로 계산하여 메모리를 할당후 메모리 블록을 리턴하여 준다.
클래스를 사용하는 경우 클래스의 생성자 호출 및 클래스정보 할당을 받지 못한다.
클래스의 가상함수, 상속관계가 전혀 이뤄지지 않는다.
VirtualAlloc 으로 메모리 할당 후 memset 으로 초기화를 하므로 클래스정보는 전혀 없다.


----------------------------------------------------------------*/
#ifndef __MEMORYPOOL_H__
#define __MEMORYPOOL_H__

#include <assert.h>
#include <new.h>

template <class DATA>
class CMemoryPool
{
private:

	/* **************************************************************** */
	// 각 블럭 앞에 사용될 노드 구조체.
	/* **************************************************************** */
	struct st_BLOCK_NODE
	{
		st_BLOCK_NODE()
		{
			stpNextBlock = NULL;
		}

		st_BLOCK_NODE *stpNextBlock;
	};



public:

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 최대 블럭 개수.
	//				(bool) 메모리 Lock 플래그 - 중요하게 속도를 필요로 한다면 Lock.
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CMemoryPool(int iBlockNum, bool bLockFlag = false)
	{
		//////////////////////////////////////////////////////////////////////
		// 초기화
		//////////////////////////////////////////////////////////////////////
		_iBlockCount = iBlockNum;
		_iAllocCount = 0;
		_bLockflag = bLockFlag;

		//////////////////////////////////////////////////////////////////////
		// 블록이 부족할 경우 동적 할당
		//////////////////////////////////////////////////////////////////////
		if (0 == iBlockNum)
		{
			_bDynamicflag = true;

			_pTopNode = nullptr;
		}

		//////////////////////////////////////////////////////////////////////
		// 지정된 갯수만큼 블록 생성
		//////////////////////////////////////////////////////////////////////
		else
		{
			_bDynamicflag = false;

			//////////////////////////////////////////////////////////////////////
			// 블록 사이즈
			//////////////////////////////////////////////////////////////////////
			int iBlockSize = sizeof(st_BLOCK_NODE) + sizeof(DATA);

			//////////////////////////////////////////////////////////////////////
			// 블록 할당 후 연결
			//////////////////////////////////////////////////////////////////////
			_pTopNode = (st_BLOCK_NODE *)malloc(iBlockSize * iBlockNum);

			//////////////////////////////////////////////////////////////////////
			// 페이징 복사 금지
			//////////////////////////////////////////////////////////////////////
			if (true == bLockFlag)
				VirtualLock((LPVOID)_pTopNode, iBlockSize);

			st_BLOCK_NODE *pFreeNode = _pTopNode;

			for (int iCnt = 0; iCnt < iBlockNum; iCnt++)
			{
				pFreeNode->stpNextBlock = (st_BLOCK_NODE *)(((char *)pFreeNode) + iBlockSize);
				pFreeNode = pFreeNode->stpNextBlock;
			}

			pFreeNode->stpNextBlock = NULL;
		}
	}

	virtual	~CMemoryPool()
	{
		free(_pTopNode);
	}


	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA	*Alloc(bool bPlacementNew = true)
	{
		st_BLOCK_NODE *pAllocNode;

		if (_iAllocCount == _iBlockCount)
		{
			if (_bDynamicflag)
			{
				pAllocNode = (st_BLOCK_NODE *)malloc(sizeof(st_BLOCK_NODE) + sizeof(DATA));
				_iBlockCount++;
			}

			else				return nullptr;
		}

		else
		{
			pAllocNode = _pTopNode;
			_pTopNode = _pTopNode->stpNextBlock;
		}

		DATA *pData = (DATA *)(pAllocNode + 1);

		_iAllocCount++;


		return pData;
	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA *pData)
	{
		st_BLOCK_NODE *pReturnNode = ((st_BLOCK_NODE *)pData) - 1;

		pReturnNode->stpNextBlock = _pTopNode;
		_pTopNode = pReturnNode;

		_iAllocCount--;

		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int		GetAllocCount(void) { return _iAllocCount; }


	//////////////////////////////////////////////////////////////////////////
	// 전체 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 전체 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int		GetBlockCount(void) { return _iBlockCount; }

private:
	//////////////////////////////////////////////////////////////////////////
	// MemoryPool의 Top
	//////////////////////////////////////////////////////////////////////////
	st_BLOCK_NODE *_pTopNode;

	//////////////////////////////////////////////////////////////////////////
	// Lockflag
	//////////////////////////////////////////////////////////////////////////
	bool _bLockflag;

	//////////////////////////////////////////////////////////////////////////
	// 블록 동적 생성 모드
	//////////////////////////////////////////////////////////////////////////
	bool _bDynamicflag;

	//////////////////////////////////////////////////////////////////////////
	// 현재 할당중인 블록 수
	//////////////////////////////////////////////////////////////////////////
	int _iAllocCount;


	//////////////////////////////////////////////////////////////////////////
	// 전체 블록 수
	//////////////////////////////////////////////////////////////////////////
	int _iBlockCount;
};


#endif
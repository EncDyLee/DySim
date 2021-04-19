#include "WorkloadInterface.h"

void HostFtlModule::ReadOperation(InitCmd& initCmd)
{

	OcAddr lastAddr;
	
	for (Int32 i = 0; i < initCmd.size; i++)
	{
		Int32 puNo = Int32((initCmd.lba + i) / LBA_PER_PU);
		Int64 nextLba = (initCmd.lba + i) % LBA_PER_PU;
		Int64 nextVba = mapTab[puNo].GetVirtualSectorAddr(nextLba);
		if (nextVba == INV_VALUE)
		{
			OcAddr preWrAddr;
			preWrAddr = OcAddr(puNo, GetHostOpenChunk(puNo).index, chkTab[puNo].GetWp(GetHostOpenChunk(puNo).index));
			preWrAddr.vectLbaMap.push_back(nextLba);

			NewWriteUpdate(puNo, nextLba, GetHostOpenChunk(puNo));
			PushHostCmd(initCmd.time, Opcode::WRITE, preWrAddr, 1);

			continue;
		}
		else
		{
			OcAddr nextAddr(puNo, Int32(nextVba / LBA_PER_CHUNK), (nextVba % LBA_PER_CHUNK));

			if (lastAddr.vectLbaMap.empty())
				lastAddr = nextAddr;
			else if ((lastAddr.pu != nextAddr.pu) || (lastAddr.chunk != nextAddr.chunk))
			{
				PushHostCmd(initCmd.time, Opcode::READ, lastAddr, lastAddr.vectLbaMap.size());
				lastAddr.vectLbaMap.clear();

				lastAddr = nextAddr;
			}

			lastAddr.vectLbaMap.push_back(nextLba);
		}
	}

	if (lastAddr.vectLbaMap.size() > 0)
		PushHostCmd(initCmd.time, Opcode::READ, lastAddr, lastAddr.vectLbaMap.size());


}

void HostFtlModule::WriteOperation(InitCmd& initCmd)
{
	
	OcAddr lastAddr;
	for (Int32 i = 0; i < initCmd.size; i++)
	{
		Int32 puNo = Int32((initCmd.lba + i) / LBA_PER_PU);
		Int64 nextLba = (initCmd.lba + i) % LBA_PER_PU;
		OverWriteUpdate(puNo, nextLba);

		OcAddr nexAddr(puNo, GetHostOpenChunk(puNo).index, chkTab[puNo].GetWp(GetHostOpenChunk(puNo).index));
		NewWriteUpdate(puNo, nextLba, GetHostOpenChunk(puNo));

		if (lastAddr.vectLbaMap.empty())
			lastAddr = nexAddr;
		else if ((lastAddr.pu != nexAddr.pu) || (lastAddr.chunk != nexAddr.chunk))
		{
			PushHostCmd(initCmd.time, Opcode::WRITE, lastAddr, lastAddr.vectLbaMap.size());
			lastAddr.vectLbaMap.clear();
			lastAddr = nexAddr;
		}

		lastAddr.vectLbaMap.push_back(nextLba);
	}

	PushHostCmd(initCmd.time, Opcode::WRITE, lastAddr, lastAddr.vectLbaMap.size());
}

void HostFtlModule::GcEraseOperation(Int64 time, Int32 puNo)
{
	ListedIndex temp = chkTab[puNo].invList.front();
	chkTab[puNo].invList.pop_front();

	PushHostCmd(time, Opcode::ERASE, OcAddr(puNo, temp.index, INV_VALUE), 1);

	chkTab[puNo].SetEc(temp.index);
	chkTab[puNo].SetWp(temp.index, 0);
	chkTab[puNo].SetInv(temp.index, 0);
	
	if (ocIf.GetChunkStat(puNo, temp.index).wli > ECC_BIT_MAX)
	{
		chkTab[puNo].badList.push_back(temp);
		wlIf->SetEraseChunk(puNo, temp.index);
		
		printf("Host bad push pu %d chunk %d\n", puNo, temp.index);

		//printf("\nCURWV:%lld MB, pu %d, bad %d, badCnt %d \n",(totalWriteVolume/1024), puNo, temp.index, chkTab[puNo].badList.size());
		//printf("%lld\n", (totalWriteVolume / 1024));
		//if (chkTab[puNo].badList.size() > MIN_SYS_BLOCK_CNT)
		//	IsBadOver = true;
	}
	else
	{
		temp.value = ocIf.GetChunkStat(puNo, temp.index).wli;

		chkTab[puNo].freeList.push_back(temp);
		chkTab[puNo].freeList.sort(CmpValue);

		wlIf->SetEraseChunk(puNo, temp.index);
	}
}

void HostFtlModule::GcCopyOperation(Int64 time, Int32 puNo)
{
	Int32 victimChk = GetGcVictimChunk(puNo);
	Int32 validCnt = LBA_PER_CHUNK - chkTab[puNo].GetInv(victimChk);

	vector<Int64> victimLba;
	Int64 vba = Int64(victimChk) * LBA_PER_CHUNK;
	while (validCnt > 0)
	{
		Int64 lba = mapTab[puNo].GetLogicalSectorAddr(vba++);
		if (lba != INV_VALUE)
		{
			victimLba.push_back(lba);
			validCnt--;
		}
	}

	OcAddr lastAddr;
	for (Int32 i = 0; i < victimLba.size(); i++)
	{
		OverWriteUpdate(puNo, victimLba[i]);

		OcAddr nexAddr(puNo, GetGcOpenChunk(puNo).index, chkTab[puNo].GetWp(GetGcOpenChunk(puNo).index));
		NewWriteUpdate(puNo, victimLba[i], GetGcOpenChunk(puNo));

		if (lastAddr.vectLbaMap.empty())
			lastAddr = nexAddr;
		else if (lastAddr.chunk != nexAddr.chunk)
		{
			PushHostCmd(time, Opcode::COPY, lastAddr, lastAddr.vectLbaMap.size());
			lastAddr.vectLbaMap.clear();

			lastAddr = nexAddr;
		}

		lastAddr.vectLbaMap.push_back(victimLba[i]);
	}

	PushHostCmd(time, Opcode::COPY, lastAddr, lastAddr.vectLbaMap.size());

}

void HostFtlModule::ChunkMngOperation(InitCmd& initCmd)
{
	for (Int32 pu = 0; pu < PU_PER_GROUP; pu++)
	{
		if ((!chkTab[pu].invList.empty()))
			GcEraseOperation(initCmd.time, pu);
		else if (chkTab[pu].freeList.size() < GC_TRIGGER_FREE_CHUNK_LIMIT_CNT)
			GcCopyOperation(initCmd.time, pu);
		
		if ((!chkTab[pu].freeList.empty() && !chkTab[pu].dataList.empty())
			&& ((chkTab[pu].freeList.back().value - chkTab[pu].dataList.front().value) > WL_THRESHOLD)
			&& (wlIf->CheckTriggerCondition(pu, chkTab[pu].freeList.back().index, chkTab[pu].dataList.front().index)))
		{
			WlOperation(initCmd.time, pu);
		}
	}
}

Int32 HostFtlModule::GetGcVictimChunk(Int32 puNo)
{
	Int32 victimChk = INV_VALUE;
	Int32 topInvCnt = 0;

	for (auto iter = chkTab[puNo].dataList.begin(); iter != chkTab[puNo].dataList.end(); iter++)
	{
		if (chkTab[puNo].GetInv(iter->index) > topInvCnt)
		{
			topInvCnt = chkTab[puNo].GetInv(iter->index);
			victimChk = iter->index;
		}
	}
	if (topInvCnt == 0)
		ErrMsg("GC 대상 블록 선정 실패\n");

	return victimChk;
}

ListedIndex& HostFtlModule::GetHostOpenChunk(Int32 pu)
{
	if (chkTab[pu].hostOpenChk.index == INV_VALUE) //Open block check
	{
		if (chkTab[pu].freeList.empty())
			ErrMsg("WR 호스트가 사용가능한 빈 청크가 없음\n");

		chkTab[pu].hostOpenChk = chkTab[pu].freeList.front();
		chkTab[pu].freeList.pop_front();

		while(ocIf.GetChunkStat(pu, chkTab[pu].hostOpenChk.index).wli > ECC_BIT_MAX)
		{
			if (chkTab[pu].freeList.empty())
				ErrMsg("WR 호스트가 사용가능한 빈 청크가 없음\n");

			printf("Host bad push pu %d chunk %d\n", pu, chkTab[pu].hostOpenChk.index);
			chkTab[pu].badList.push_back(chkTab[pu].hostOpenChk);

			chkTab[pu].hostOpenChk = chkTab[pu].freeList.front();
			chkTab[pu].freeList.pop_front();
		}
	}

	return chkTab[pu].hostOpenChk;
}

ListedIndex& HostFtlModule::GetGcOpenChunk(Int32 pu)
{
	if (chkTab[pu].gcOpenChk.index == INV_VALUE) //Open block check
	{
		if (chkTab[pu].freeList.empty())
			ErrMsg("WR 호스트가 사용가능한 빈 청크가 없음\n");

		chkTab[pu].gcOpenChk = chkTab[pu].freeList.front();
		chkTab[pu].freeList.pop_front();

		while (ocIf.GetChunkStat(pu, chkTab[pu].gcOpenChk.index).wli > ECC_BIT_MAX)
		{
			if (chkTab[pu].freeList.empty())
				ErrMsg("WR 호스트가 사용가능한 빈 청크가 없음\n");

			printf("Host bad push pu %d chunk %d\n", pu, chkTab[pu].gcOpenChk.index);
			chkTab[pu].badList.push_back(chkTab[pu].gcOpenChk);

			chkTab[pu].gcOpenChk = chkTab[pu].freeList.front();
			chkTab[pu].freeList.pop_front();
		}
	}

	return chkTab[pu].gcOpenChk;
}

ListedIndex& HostFtlModule::GetWlOpenChunk(Int32 pu)
{
	if (chkTab[pu].wlOpenChk.index == INV_VALUE) //Open block check
	{
		if (chkTab[pu].freeList.empty())
			ErrMsg("WR 호스트가 사용가능한 빈 청크가 없음\n");

		chkTab[pu].wlOpenChk = chkTab[pu].freeList.back();
		chkTab[pu].freeList.pop_back();


		while (ocIf.GetChunkStat(pu, chkTab[pu].wlOpenChk.index).wli > ECC_BIT_MAX)
		{
			if (chkTab[pu].freeList.empty())
				ErrMsg("WR 호스트가 사용가능한 빈 청크가 없음\n");

			printf("Host bad push pu %d chunk %d\n", pu, chkTab[pu].wlOpenChk.index);
			chkTab[pu].badList.push_back(chkTab[pu].wlOpenChk);

			chkTab[pu].wlOpenChk = chkTab[pu].freeList.front();
			chkTab[pu].freeList.pop_front();
		}
	}

	return chkTab[pu].wlOpenChk;
}

void HostFtlModule::OverWriteUpdate(Int32 pu, Int64 lba)
{
	Int64 vba = mapTab[pu].GetVirtualSectorAddr(lba);
	if (vba != INV_VALUE) //Over write check
	{
		Int32 oldChkNo = Int32(vba / LBA_PER_CHUNK);
		mapTab[pu].SetVtolMap(vba, INV_VALUE);
		chkTab[pu].SetInv(oldChkNo);

		if (chkTab[pu].GetInv(oldChkNo) == LBA_PER_CHUNK) //Inv block update
		{
			for (auto iter = chkTab[pu].dataList.begin(); iter != chkTab[pu].dataList.end(); iter++)
			{
				if (iter->index == oldChkNo)
				{
					chkTab[pu].invList.push_back(*iter);
					chkTab[pu].dataList.erase(iter);
					break;
				}
			}
		}
	}

}

void HostFtlModule::NewWriteUpdate(Int32 pu, Int64 lba, ListedIndex &openChk)
{
	Int64 newAddr = (Int64(openChk.index) * LBA_PER_CHUNK) + chkTab[pu].GetWp(openChk.index);;

	totalWriteVolume++;

	mapTab[pu].SetVtolMap(newAddr, lba);
	mapTab[pu].SetLtovMap(lba, newAddr);

	chkTab[pu].SetWp(openChk.index);
	if (chkTab[pu].GetWp(openChk.index) == LBA_PER_CHUNK)
	{
		wlIf->SetCloseChunk(pu, openChk.index);

		chkTab[pu].dataList.push_back(openChk);
		openChk.index = INV_VALUE;
		openChk.value = INV_VALUE;
	}
}




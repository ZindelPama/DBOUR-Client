#include "precomp_dboclient.h"
#include "JudgeDump.h"

// core
#include "NtlDebug.h"

// shared
#include "NtlPacketUG.h"
#include "NtlPacketGU.h"
#include "NtlPacketAU.h"
#include "NtlPacketUA.h"
#include "NtlPacketUC.h"
#include "NtlPacketCU.h"
#include "NtlPacketTU.h"
#include "NtlPacketUT.h"
#include "NtlPacketUtil.h"

// simulation
#include "NtlSLDef.h"
#include "NtlSLGlobal.h"
#include "NtlSob.h"
#include "NtlSobAvatar.h"
#include "NtlSobManager.h"
#include "NtlSLEventFunc.h"
#include "NtlSLLogic.h"

// cleint
#include "DumpCommand.h"
#include "DboGlobal.h"

//Network
#include "..\Server\NtlNetwork\NtlPacket.h"
namespace
{
#define dBUFFER_SIZE		2048

	char acBuffer[dBUFFER_SIZE] = "";
}


//////////////////////////////////////////////////////////////////////////
//			CJudgeDefaultDump
//////////////////////////////////////////////////////////////////////////

const char* CJudgeDefaultDump::GetDump_Message(const WCHAR*  pwcMessage)
{
	::WideCharToMultiByte(GetACP(), 0, pwcMessage, -1, acBuffer, dBUFFER_SIZE, NULL, NULL);
	return acBuffer;
}


//////////////////////////////////////////////////////////////////////////
//			CJudgeActionDump
//////////////////////////////////////////////////////////////////////////

char* CJudgeActionDump::GetDump_SendPacket(void* pPacket)
{
	sNTLPACKETHEADER *pHeader = (sNTLPACKETHEADER*)pPacket;
	RwUInt16 wOpCode = pHeader->wOpCode;

	// avooo's command : �ʹ� ���� ��Ŷ�� ����� �Ǿ� �ּ�ó�� �ϰڽ��ϴ�

	const char *pOpString = NtlGetPacketName(wOpCode);
	
	switch(wOpCode)
	{
			case UG_CHAR_MOVE:
			{
			sUG_CHAR_MOVE *pCharMove = (sUG_CHAR_MOVE*)pPacket;
			sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(UG_CHAR_MOVE) => Avatar , curr pos(%f, %f, %f), dir(%f, %f), dirflag(%d)\n\n",
			pCharMove->vCurLoc.x, pCharMove->vCurLoc.y, pCharMove->vCurLoc.z,
			pCharMove->vCurDir.x, pCharMove->vCurDir.z,
			pCharMove->byMoveDirection);
			return acBuffer;
			}
			case UG_CHAR_DEST_MOVE:
			{
			sUG_CHAR_DEST_MOVE *pCharDestMove = (sUG_CHAR_DEST_MOVE*)pPacket;
			sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(UG_CHAR_DEST_MOVE) => Avatar , dest pos(%f, %f, %f)\n\n",
			pCharDestMove->vDestLoc.x, pCharDestMove->vDestLoc.y, pCharDestMove->vDestLoc.z);
			return acBuffer;
			}
			case UG_CHAR_CHANGE_DIRECTION_ON_FLOATING:
			{
			sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(UG_CHAR_CHANGE_DIRECTION_ON_FLOATING) => Avatar \n\n");
			return acBuffer;
			}
			case UG_CHAR_JUMP:
			{
			sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(UG_CHAR_JUMP) => Avatar\n\n");
			return acBuffer;
			}
			case UG_CHAR_JUMP_END:
			{
			sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(UG_CHAR_JUMP_END) => Avatar\n\n");
			return acBuffer;
			}
			case UG_CHAR_TOGG_SITDOWN:
			{
			sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(UG_CHAR_TOGG_SITDOWN) => Avatar\n\n");
			return acBuffer;
			}
			case UG_CHAR_CHARGE:
			{
			sUG_CHAR_CHARGE *pJumpEnd = (sUG_CHAR_CHARGE*)pPacket;
			sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(UG_CHAR_CHARGE, %d ) => Avatar\n\n", pJumpEnd->bCharge);
			return acBuffer;
			}
			case UG_ITEM_MOVE_REQ:
			{
			sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(UG_ITEM_MOVE_REQ) => Avatar \n\n");
			return acBuffer;
			}
			case UG_CHAR_TOGG_FIGHTING:
			{
			sUG_CHAR_TOGG_FIGHTING *pFightingMode = (sUG_CHAR_TOGG_FIGHTING*)pPacket;
			sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(UG_CHAR_TOGG_FIGHTING) => flag(%d)\n\n", pFightingMode->bFightMode);
			return acBuffer;
			}
	case UC_CHARACTER_DEL_REQ:
	{
		sUC_CHARACTER_DEL_REQ * pCharDel = (sUC_CHARACTER_DEL_REQ *)pPacket;
		
		sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(UC_CHARACTER_DEL_REQ) => CHARID (%d)\n\n", pCharDel->charId);
		return acBuffer;

	}
		default:
		{
			if (wOpCode != (UG_CHAR_MOVE || UG_CHAR_MOVE_SYNC || 1))
			{
				sprintf_s(acBuffer, dBUFFER_SIZE, "Send Net(%s) => Net op code \n\n", pOpString);
				return acBuffer;
			}
		}
	}

	return NULL;
}

char* CJudgeActionDump::GetDump_RecvPacket(void* pPacket)
{
	sNTLPACKETHEADER *pHeader = (sNTLPACKETHEADER*)pPacket;
	RwUInt16 wOpCode = pHeader->wOpCode;
	const char *pOpString = NtlGetPacketName(wOpCode);
	CNtlPacket *rpacket = (CNtlPacket*)pPacket;
	switch (wOpCode)
	{
	case AU_LOGIN_RES:
	{
	/*  WORD		wResultCode;
		WCHAR		awchUserId[NTL_MAX_SIZE_USERID_UNICODE + 1];
		BYTE		abyAuthKey[NTL_MAX_SIZE_AUTH_KEY];
		ACCOUNTID	accountId;
		SERVERFARMID		lastServerFarmId;
		DWORD				dwAllowedFunctionForDeveloper;
		BYTE				byServerInfoCount;
		sSERVER_INFO		aServerInfo[DBO_MAX_CHARACTER_SERVER_COUNT];*/

		sAU_LOGIN_RES *pLogin = (sAU_LOGIN_RES*)pPacket;
	

		sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s) => result(%d) accountID(%d) username(%s) devFuncs(%d) ipForCharServer(%s) portForCharServer(%d)\n\n"
			, pOpString, pLogin->wResultCode, pLogin->accountId, pLogin->awchUserId, pLogin->dwAllowedFunctionForDeveloper, 
			pLogin->aServerInfo->szCharacterServerIP, pLogin->aServerInfo->wCharacterServerPortForClient);

		//printf("Packet_[%u] Header[", pHeader->wOpCode);

		//int i = 0;
		//printf("0x%02X", rpacket->GetPacketHeader()->bEncrypt);
		//printf(":");
		//printf("0x%02X", rpacket->GetPacketHeader()->wPacketLen);
		//printf(":");
		//printf("0x%02X", rpacket->GetPacketHeader()->bySequence);
		//printf(":");
		//printf("0x%02X", rpacket->GetPacketHeader()->byChecksum);
		//printf(":");
		//printf("] Data[");
		//for (i = 0; i < rpacket->GetPacketDataSize(); i++)
		//{
		//	if (i > 0) printf(":");
		//	printf("0x%02X", rpacket->GetPacketData()[i]);
		//}
		//printf("]\n");

		//printf("Full Packet [");
		//for (i = 0; i < (rpacket->GetHeaderSize() + rpacket->GetPacketLen()); i++)
		//{
		//	if (i > 0) printf(":");
		//	printf("0x%02X", rpacket->GetPacketBuffer()[i]);
		//}
		//printf("]\n");
		return acBuffer;
	}


	case GU_CHAR_JUMP:
	{
		sGU_CHAR_JUMP *pJump = (sGU_CHAR_JUMP*)pPacket;
		CNtlSob *pSobObj = GetNtlSobManager()->GetSobObject(pJump->handle);

		if (pSobObj)
		{
			const WCHAR *pName = Logic_GetName(pSobObj);
			if (pName)
				sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s - %s) => handle(%u), name(%s)\n\n", pOpString, pSobObj->GetClassName(), pJump->handle, pName);
			else
				sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s - %s) => handle(%u) \n\n", pOpString, pSobObj->GetClassName(), pJump->handle);
		}
		else
		{
			sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s) => handle(%u) \n\n", pOpString, pJump->handle);
		}

		return acBuffer;
	}
	case GU_CHAR_JUMP_END:
	{
		sGU_CHAR_JUMP_END *pJumpEnd = (sGU_CHAR_JUMP_END*)pPacket;
		CNtlSob *pSobObj = GetNtlSobManager()->GetSobObject(pJumpEnd->handle);

		if (pSobObj)
		{
			const WCHAR *pName = Logic_GetName(pSobObj);
			if (pName)
				sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s - %s) => handle(%u), name(%s)\n\n", pOpString, pSobObj->GetClassName(), pJumpEnd->handle, pName);
			else
				sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s - %s) => handle(%u) \n\n", pOpString, pSobObj->GetClassName(), pJumpEnd->handle);
		}
		else
		{
			sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s) => handle(%u) \n\n", pOpString, pJumpEnd->handle);
		}

		return acBuffer;
	}
	case GU_CHAR_MOVE:
	{
		sGU_CHAR_MOVE *pCharMove = (sGU_CHAR_MOVE*)pPacket;

		if (pCharMove->handle == GetDboGlobal()->GetTargetSerial())
		{
			sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(GU_CHAR_MOVE) => handle(%u) , curr pos(%f, %f, %f), dir(%f, %f), dirflag(%d)\n\n",
				pCharMove->handle,
				pCharMove->vCurLoc.x, pCharMove->vCurLoc.y, pCharMove->vCurLoc.z,
				pCharMove->vCurDir.x, pCharMove->vCurDir.z,
				pCharMove->byMoveDirection);
			return acBuffer;
		}
	}
	case GU_CHAR_DEST_MOVE:
	{
		sGU_CHAR_DEST_MOVE *pCharDestMove = (sGU_CHAR_DEST_MOVE*)pPacket;
		if (GetDumpCmdManager()->IsOutputRecvPacketTarget(pCharDestMove->handle))
		{
			CNtlSob *pSobObj = GetNtlSobManager()->GetSobObject(pCharDestMove->handle);
			if (pSobObj)
			{
				RwUInt8 byServerState = 0, byClientState = 0;
				CNtlSLEventGenerator::SobGetState(pSobObj->GetSerialID(), byServerState, byClientState);
				sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s, %s) => handle(%u), dest count (%d), dest pos(%f, %f, %f), server state(%s), client state(%d) \n\n",
					pOpString,
					pSobObj->GetClassName(),
					pSobObj->GetSerialID(),
					pCharDestMove->byDestLocCount,
					pCharDestMove->avDestLoc[0].x,
					pCharDestMove->avDestLoc[0].y,
					pCharDestMove->avDestLoc[0].z,
					NtlGetCharStateString(byServerState), byClientState);
				return acBuffer;
			}
		}
	}
	case GU_ITEM_MOVE_RES:
	{

		sGU_ITEM_MOVE_RES *pItem = (sGU_ITEM_MOVE_RES*)pPacket;

		sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(GU_ITEM_MOVE_RES) => Req place \n\n");
		return acBuffer;
	}
	case GU_AVATAR_ATTRIBUTE_UPDATE:
	{
		sGU_AVATAR_ATTRIBUTE_UPDATE *pAttributeUpdate = (sGU_AVATAR_ATTRIBUTE_UPDATE*)pPacket;

		sprintf_s(acBuffer, "Recv Net(GU_UPDATE_ATTRIBUTE_UPDATE) => AttributeTotalCount %d  Flexible Field[0] %d \n\n", pAttributeUpdate->byAttributeTotalCount, pAttributeUpdate->abyFlexibleField[0]);

		return acBuffer;

	}
	case GU_WAR_FOG_INFO:
	{
		sGU_WAR_FOG_INFO *pWarInfo = (sGU_WAR_FOG_INFO*)pPacket;
		char* acBuffer2 = "";

		sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(War_Fog_Info) => %d %d %d %d %d %d %d %d %d %d %d\n\n", pWarInfo->abyWarFogInfo[0], pWarInfo->abyWarFogInfo[1], pWarInfo->abyWarFogInfo[2], pWarInfo->abyWarFogInfo[3], pWarInfo->abyWarFogInfo[4], pWarInfo->abyWarFogInfo[5], pWarInfo->abyWarFogInfo[6], pWarInfo->abyWarFogInfo[7], pWarInfo->abyWarFogInfo[8], pWarInfo->abyWarFogInfo[9], pWarInfo->abyWarFogInfo[10]);
		return acBuffer;
	}

	case GU_CHAR_ACTION_ATTACK:
	{
		sGU_CHAR_ACTION_ATTACK *pActionAttack = (sGU_CHAR_ACTION_ATTACK*)pPacket;

		if (GetDumpCmdManager()->IsOutputRecvPacketTarget(pActionAttack->hSubject))
		{
			const char *pOpString = NtlGetPacketName(wOpCode);
			CNtlSob *pSobObj = GetNtlSobManager()->GetSobObject(pActionAttack->hSubject);
			sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s - %s : attresult %d, chainflag %d, attacksequence %d, shift(%f, %f, %f), tick time(%u)  \n\n", pOpString, pSobObj->GetClassName(),
				pActionAttack->byAttackResult, pActionAttack->bChainAttack, pActionAttack->byAttackSequence,
				pActionAttack->vShift.x, pActionAttack->vShift.y, pActionAttack->vShift.z, GetTickCount());
			return acBuffer;
		}
	}
	case GU_UPDATE_CHAR_STATE:
	{

		sGU_UPDATE_CHAR_STATE *pState = (sGU_UPDATE_CHAR_STATE*)pPacket;

		const char *pOpString = NtlGetPacketName(wOpCode);
		CNtlSob *pSobObj = GetNtlSobManager()->GetSobObject(pState->handle);
		if (pSobObj)
		{
			sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s - %s  ): State:%s ConditionFlag:%u StateTime:%u CurLoc:x%f y%f z%f \n\n", pOpString, pSobObj->GetClassName(), NtlGetCharStateString(pState->sCharState.sCharStateBase.byStateID),
				pState->sCharState.sCharStateBase.dwConditionFlag, pState->sCharState.sCharStateBase.dwStateTime,
				pState->sCharState.sCharStateBase.vCurLoc.x, pState->sCharState.sCharStateBase.vCurLoc.y, pState->sCharState.sCharStateBase.vCurLoc.z);
			return acBuffer;
		}
	}

	default:
	{
		//char buf[1024];

		//sprintf( buf, "Packet_[%u] Header[", pHeader->wOpCode);

		//int i = 0;
		//sprintf(buf, "0x%02X", rpacket->GetPacketHeader()->bEncrypt);
		//sprintf(buf, ":");
		//sprintf(buf, "0x%02X", rpacket->GetPacketHeader()->wPacketLen);
		//sprintf(buf, ":");
		//sprintf(buf, "0x%02X", rpacket->GetPacketHeader()->bySequence);
		//sprintf(buf, ":");
		//sprintf(buf, "0x%02X", rpacket->GetPacketHeader()->byChecksum);
		//sprintf(buf, ":");
		//sprintf(buf, "] Data[");
		//for (i = 0; i < rpacket->GetPacketDataSize(); i++)
		//{
		//	if (i > 0) sprintf(buf, ":");
		//	sprintf(buf, "0x%02X", rpacket->GetPacketData()[i]);
		//}
		//sprintf(buf, "]\n");

		
			sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s) => Net op code(%d) \n\n", pOpString, wOpCode);
			return acBuffer;
	}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
//			CJudgeSobCreateDestoryDump
//////////////////////////////////////////////////////////////////////////

char* CJudgeSobCreateDestoryDump::GetDump_RecvPacket(void* pPacket)
{
	sNTLPACKETHEADER *pHeader = (sNTLPACKETHEADER*)pPacket;
	RwUInt16 wOpCode = pHeader->wOpCode;
	const char *pOpString = NtlGetPacketName(wOpCode);
	CNtlPacket *rpacket = (CNtlPacket*)pPacket;

	switch (wOpCode)
	{
	case GU_OBJECT_CREATE:
	{
		sGU_OBJECT_CREATE *pObjCreate = (sGU_OBJECT_CREATE*)pPacket;
		sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s) => handle(%u)\n\n", pOpString, pObjCreate->handle);
		return acBuffer;
	}
	case GU_OBJECT_DESTROY:
	{
		sGU_OBJECT_DESTROY *pObjDestroy = (sGU_OBJECT_DESTROY*)pPacket;
		CNtlSob *pSobObj = GetNtlSobManager()->GetSobObject(pObjDestroy->handle);

		if (pSobObj)
		{
			const WCHAR *pName = Logic_GetName(pSobObj);
			if (pName)
				sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s - %s) => handle(%u), name(%s)\n\n", pOpString, pSobObj->GetClassName(), pObjDestroy->handle, pName);
			else
				sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s - %s) => handle(%u) \n\n", pOpString, pSobObj->GetClassName(), pObjDestroy->handle);

			return acBuffer;
		}
		else
		{
			sprintf_s(acBuffer, dBUFFER_SIZE, "Recv Net(%s) => handle(%u) \n\n", pOpString, pObjDestroy->handle);
			return acBuffer;
		}
	}
	default:
	{
		/*	char buf[1024];

			sprintf(buf, "Packet_[%u] Header[", pHeader->wOpCode);

			int i = 0;
			sprintf(buf, "0x%02X", rpacket->GetPacketHeader()->bEncrypt);
			sprintf(buf, ":");
			sprintf(buf, "0x%02X", rpacket->GetPacketHeader()->wPacketLen);
			sprintf(buf, ":");
			sprintf(buf, "0x%02X", rpacket->GetPacketHeader()->bySequence);
			sprintf(buf, ":");
			sprintf(buf, "0x%02X", rpacket->GetPacketHeader()->byChecksum);
			sprintf(buf, ":");
			sprintf(buf, "] Data[");
			for (i = 0; i < rpacket->GetPacketDataSize(); i++)
			{
			if (i > 0) sprintf(buf, ":");
			sprintf(buf, "0x%02X", rpacket->GetPacketData()[i]);
			}
			sprintf(buf, "]\n");

			sprintf_s(acBuffer, dBUFFER_SIZE, "Unknown Packet Recieved %s", buf);
			return acBuffer;*/
	}
	}

	return NULL;
}
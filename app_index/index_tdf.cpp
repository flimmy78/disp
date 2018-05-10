#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>

#include "bintree.h"
#include "wwtiny.h"

#include "QTSStruct.h"
#include "index_supp.h"

void TDF_MARKET_DATA2TinyQuotation(TDF_MARKET_DATA *pi,struct TinyQuotationStruct *po)
{
	po->iStockCode=	atoi(pi->szCode);

        po->nActionDay= pi->nActionDay;
        po->nTime= 	pi->nTime;

        po->nPreClose= 	pi->nPreClose;
        po->nOpen= 	pi->nOpen;
        po->nHigh= 	pi->nHigh;
        po->nLow= 	pi->nLow;
        po->nMatch= 	pi->nMatch;

        for (unsigned i = 0; i < LEVEL_TEN; ++i) {
                po->nAskVol[i]= 	pi->nAskVol[i];
                po->nAskPrice[i]= 	pi->nAskPrice[i];
        }
        for (unsigned i = 0; i < LEVEL_TEN; ++i) {
                po->nBidVol[i]= 	pi->nBidVol[i];
                po->nBidPrice[i]= 	pi->nBidPrice[i];
        }

        po->nNumTrades= 	pi->nNumTrades;
        po->iVolume= 		pi->iVolume;
        po->iTurnover= 		pi->iTurnover;
        po->nTotalBidVol= 	pi->nTotalBidVol;
        po->nTotalAskVol= 	pi->nTotalAskVol;
        po->nWtAvgBidPrice= 	pi->nWeightedAvgBidPrice;
        po->nWtAvgAskPrice=	pi->nWeightedAvgAskPrice;
}


/**
��������
	TAIL_NO_STAT	�����ļ�ĩβ��δͳ��
	WANT_STAT	����nEndTime0��������Ҫͳ��
	-1	��������
**/


int MountTrsData2IndexStatArray(char sFileName[],int nBgnActionDay,
	int nPreT0,int nT0,int nEndTime0,long lItemLen,long *plCurPos)
{
	FILE *fp;
	int iRet;
	char sBuffer[2048];
//	long long *pll=(long long*)sBuffer;
	TDF_TRANSACTION *p=(TDF_TRANSACTION*)(sBuffer+sizeof(long long));
	struct TinyTransactionStruct tt,*pt;

	LISTHEAD *pSXT;

	long lCurPos=*plCurPos;

	struct IndexStatStruct *pIndexStat;

	if((fp=fopen(sFileName,"r"))==NULL){
		printf("error open file %s to read.\n",sFileName);
		return -1;
	}

	if(fseek(fp,lCurPos,SEEK_SET)<0){
		printf("error fseek file=%s,pos=%ld.\n",sFileName,lCurPos);
		return -1;
	}
	while(1){
		if(fread((void*)sBuffer,lItemLen,1,fp)!=1){
			iRet=MY_TAIL_NO_STAT;
			break;
		}

		lCurPos+=lItemLen;

		if(p->nActionDay!=nBgnActionDay){
			printf("error date file=%s,pos=%ld,actionday=%d,calcdate=%d\n",
				sFileName,lCurPos,p->nActionDay,nBgnActionDay);
			return -1;
		}

		if((pIndexStat=GetIndexStat(atoi(p->szCode),sFileName,
			lCurPos,nBgnActionDay,nPreT0,nT0))==NULL) return -1;

		//���˵�������Ϣ
		if(p->chFunctionCode=='C') continue;

/*		if(iStockCode==2320){
			printf("hello my stock trans.\n");
		}
*/
		//��Transaction��������Order���ݣ����ӵ�Merge�ṹ��
		if(AddTransaction2IndexStat(p,nT0,pIndexStat)<0){
			printf("error add transaction to index stat file=%s,pos=%ld\n",
			sFileName,lCurPos);
			return -1;
		}

		TDF_TRANSACTION2TinyTransaction(p,&tt);
/*
		if(tt.nTime==93459180&&tt.nAskOrder==255798){
			printf("find U little cat.\n");
		}
*/
                //�����۸�����ĳɽ�ORDER֮��Ĺ�ϵ
/*               if(AdjustIndexStatS0(pIndexStat,&tt)<0){
			printf("error add index stat s0 file=%s,pos=%ld\n",
				sFileName,lCurPos);
			return -1;
                }
*/
		//��lCurPos,���� nT0, ���ڴ��� nPreT0�Ĳ��֣����뵽
		//S0T������nT0�����ݼӵ�pS1Head,��������������nEndTime0������ֹͣ;
//		if(t.nTime<nPreT0) continue;

		//ʵ�����,ȡ��Ϊ nPreT0<=x<nT0 �����ȥȡ����
		//���,t.nTimeΪ����ʱ��Ҳ�� 15:00:00�������
		if(MY_BELONG_TRANSACTION_T0(p->nTime,nT0)){
//		if(t.nTime<nT0||(t.nTime==MY_CLOSE_MARKET_TIME&&nT0==MY_CLOSE_MARKET_TIME)){
/***
			��дһ������4���ֶ�
			int	nAskOrderSeq;	//����ί�е��ɽ���ţ����ɱ���
			int	nBidOrderSeq;	//��ί�е��ɽ���ţ���ɱ���
			struct TinyOrderStruct *pAskOrder; //��������ָ��
			struct TinyOrderStruct *pBidOrder; //���򶩵�ָ��

***/
			if(InitTinyTransactionField(pIndexStat,&tt)<0) return -1;
			pSXT=&(pIndexStat->S0T);
		}
		else
			pSXT=&(pIndexStat->S1T);

		if((pt=(struct TinyTransactionStruct*)malloc(
			sizeof(struct TinyTransactionStruct)))==NULL){
			printf("error malloc TinyTransactionStruct  file=%s,pos=%ld\n",
				sFileName,lCurPos);
			return -1;
		}
		memcpy((void*)pt,(void*)&tt,sizeof(struct TinyTransactionStruct));
		

		Append2List(pSXT,(LIST*)pt);

		//��������nEndTime0������ֹͣ
		if(p->nTime>nEndTime0){
			iRet=MY_WANT_STAT;
			break;
		}

	}

	fclose(fp);
	*plCurPos=lCurPos;

	return iRet;
}
/*
����ֵ��
	0 �����ļ�ĩβ�������nEndTime0,�Ż�
	-1	��������
*/
int MountQuotation2IndexStatArray(char sFileName[],int nBgnActionDay,
	int nPreT0,int nT0,int nEndTime0,long lItemLen,long *plCurPos)
{
	int iRet;
	FILE *fp;
	long lCurPos=*plCurPos;
	char sBuffer[2048];
//	long long *pll=(long long*)sBuffer;
	TDF_MARKET_DATA *p=(TDF_MARKET_DATA*)(sBuffer+sizeof(long long));
	struct TinyQuotationStruct q,*pt;
	struct IndexStatStruct *pIndexStat;

	LISTHEAD *pSXQ;

	if((fp=fopen(sFileName,"r"))==NULL){
		printf("error open file %s to read.\n",sFileName);
		return -1;
	}

	if(fseek(fp,lCurPos,SEEK_SET)<0){
		printf("error fseek file=%s,pos=%ld.\n",sFileName,lCurPos);
		return -1;
	}
	while(1){
		if(fread((void*)sBuffer,lItemLen,1,fp)!=1){
			iRet=MY_TAIL_NO_STAT;
			break;
		}

		lCurPos+=lItemLen;

		TDF_MARKET_DATA2TinyQuotation(p,&q);

		if((pIndexStat=GetIndexStat(q.iStockCode,sFileName,
			lCurPos,nBgnActionDay,nPreT0,nT0))==NULL) return -1;


		//��������nEndTime0������ֹͣ
		if(q.nTime>=nEndTime0) break;

		//��lCurPos,���� nT0, ���ڴ��� nPreT0�Ĳ��֣����뵽
		//S0T������nT0�����ݼӵ�pS1Head,��������������nEndTime0������ֹͣ;
		if(q.nTime<=nPreT0) continue;

		//ʵ�����,{������������}ȡ��Ϊ nPreT0<x<=nT0 �����ȥȡ����
		//����ʱ�򣬲�ȡ9:30����������飬��ȡǰһ�����飬һ����9��25�ֵ�
		if(q.nTime<=nT0&&q.nTime!=MY_OPEN_MARKET_TIME)
			pSXQ=&(pIndexStat->S0Q);
		else	pSXQ=&(pIndexStat->S1Q);

		if((pt=(struct TinyQuotationStruct*)malloc(
			sizeof(struct TinyQuotationStruct)))==NULL){
			printf("error malloc TinyQuotationStruct  file=%s,pos=%ld\n",
				sFileName,lCurPos);
			return -1;
		}
		memcpy((void*)pt,(void*)&q,sizeof(struct TinyQuotationStruct));

		Append2List(pSXQ,(LIST*)pt);

		//��������nEndTime0������ֹͣ
		if(q.nTime>nEndTime0){
			iRet=MY_WANT_STAT;
			break;
		}

	}

	fclose(fp);
	*plCurPos=lCurPos;

	return iRet;
}

/*
����ֵ��
	0 �����ļ�ĩβ�������nEndTime0,�Ż�
	-1	��������
*/
int MountOrdData2IndexStatArray(char sFileName[],int nBgnActionDay,
	int nPreT0,int nT0,int nEndTime0,long lItemLen,long *plCurPos)
{       
	int iRet;
	FILE *fp;
	long lCurPos=*plCurPos;
	char sBuffer[2048];
//	long long *pll=(long long*)sBuffer;
	TDF_ORDER *p=(TDF_ORDER*)(sBuffer+sizeof(long long));
	struct TinyOrderStruct o,*pOrder,*pTemp;
	struct IndexStatStruct *pIndexStat;

	if((fp=fopen(sFileName,"r"))==NULL){
		printf("error open file %s to read.\n",sFileName);
		return -1;
	}

	if(fseek(fp,lCurPos,SEEK_SET)<0){
		printf("error fseek file=%s,pos=%ld.\n",sFileName,lCurPos);
		return -1;
	}
	while(1){
		if(fread((void*)sBuffer,lItemLen,1,fp)!=1){
			iRet=MY_TAIL_NO_STAT;
			break;
		}
		lCurPos+=lItemLen;

		TDF_ORDER2TinyOrder(p,&o);

		if((pIndexStat=GetIndexStat(o.iStockCode,sFileName,
			lCurPos,nBgnActionDay,nPreT0,nT0))==NULL) return -1;

//		if(iStockCode==2320){
//			printf("hello my stock order.\n");
//		}
		if(SearchBin(pIndexStat->M_ORDER,(void*)&p->nOrder,
			data_search_bintree_order_2,(void**)&pOrder)==FOUND){

			pOrder->iCloseFlag=	1;
			pOrder->nOriOrdPrice=	p->nPrice;
			pOrder->nOriOrdVolume=	p->nVolume;
			pOrder->lOriOrdAmnt=	(long)p->nPrice*p->nVolume/100;

			//��������nEndTime0������ֹͣ
			if(p->nTime>=nEndTime0){
				iRet=MY_WANT_STAT;
				break;    
			}
			continue;
		}

		//������ͷ��ָ�����
		//o.pNext=&(pIndexStat->PreS0M);
		o.pNext=(struct TinyOrderStruct*)&(pIndexStat->S0O);

		if((pTemp=(struct TinyOrderStruct*)malloc(
			sizeof(struct TinyOrderStruct)))==NULL){
			printf("malloc TinyOrderStruct error.\n");
			return -1;
		}
		memcpy((void*)pTemp,(void*)&o,sizeof(struct TinyOrderStruct));

		if(InsertBin(&(pIndexStat->M_ORDER),(void *)pTemp,
			data_search_bintree_stock_code_order,
			assign_insert_bintree_stock_code_order_e)<0){
			printf("insert bin malloc error th.\n");
			return -1;
		}
		//��������nEndTime0������ֹͣ
		if(p->nTime>=nEndTime0){
			iRet=MY_WANT_STAT;
			break;
		}
	}

	fclose(fp);
	*plCurPos=lCurPos;

	return iRet;
}

int IsBusyTime(int iTime)
{
	if(iTime<93500000||(iTime>125900000&&iTime<130200000)||
		iTime>145900000) return true;
	return false;
}
int IsStopTime(int iTime)
{
	if((iTime>113500000&&iTime<125900000)) return true;
	return false;
}

int main(int argc, char *argv[])
{
	FILE *fpD31;
	int iIdleWaitMilli=10,iWriteFlag=1;//,iContinueFlag=false;
	int iShBusyDelay=12000,iShDelay=3000,iSzBusyDelay=12000,iSzDelay=2000;
	int nBgnActionDay,nBgnTime,nPreT0,nT0,nEndTime0;
	int iMktRes,iTraRes,iOrdRes;

	char sCurTime[15],sCalcDate[15],sCalcBgn[15];
	char sDelayStr[256],sCodeStr[512],sD31Name[512],sSourcePath[512],sWorkRoot[512];
	char sMktName[512],sTraName[512],sOrdName[512];

	time_t tBeginTime,tEndTime,tDelTime;
	//��ǰ�ļ�����λ��
	long lMktCurPos=0,lTraCurPos=0,lOrdCurPos=0;

	GetHostTime(sCurTime);

	//���ò���ѡ���Ĭ��ֵ
	strncpy(sCalcDate,sCurTime,8);sCalcDate[8]=0;
	strcpy(sCalcBgn,sCurTime+8);
	strcpy(sSourcePath,	"/stock/work");
//	iMaxWaitSec=20;
	strcpy(sWorkRoot,	"/stock/work");
	strcpy(sCodeStr,"");
	strcpy(sDelayStr,"");

	//��ȡ�����в���
	for (int c; (c = getopt(argc, argv, "d:b:m:s:o:e:t:w:l:?:")) != EOF;){

		switch (c){
		case 'd':strncpy(sCalcDate, optarg,8);sCalcDate[8]=0;	break;
		case 'b':strncpy(sCalcBgn, optarg,6);sCalcBgn[6]=0;	break;
		case 's':strcpy(sSourcePath, optarg);	break;
		case 'o':strcpy(sWorkRoot, optarg);	break;
		case 'e':strcpy(sDelayStr,optarg);	break;
		case 't':iIdleWaitMilli=atoi(optarg);	break;
		case 'w':iWriteFlag=atoi(optarg);
			if(iWriteFlag!=1&&iWriteFlag!=2&&iWriteFlag!=3)iWriteFlag=1;	break;
		case 'l':strcpy(sCodeStr, optarg);	break;
		case '?':
		default:
			printf("Usage: %s \n", argv[0]);
			printf("   [-d calc-date (yyyymmdd =8Bytes def=curdate) ]\n");
			printf("   [-b begin-time (HH24MISS =6Bytes def=curtime) ]\n");
			printf("   [-m max-delay-sec (def=20) ]\n");
			printf("   [-s source-path (def=/stock/work) ]\n");
			printf("   [-o work-root-name (def=/stock/work) ]\n");
			printf("   [-e delay str 'b,i:b:i' ]\n");
			printf("   [-t idlewaitmilli \n");
			printf("   [-w writeflag (1,zbzd 2,ext 3,bin info) \n");
			printf("   [-l codelist (e.g \"000001,603912,002415\") ]\n");
			exit(1);
			break;
		}
	}

	//����delay���������Ϻ��������ܵ�ʱ�Ӳ���
	if(strlen(sDelayStr)!=0){
		char *p0;

		if((p0=strchr(sDelayStr,','))==NULL){
			printf("-e =%s ��ʽ�Ƿ�,��ȷ��ʽ�� '12000,1000:6000,900'.\n",sDelayStr);
			return -1;
		}
		p0++;
		iShBusyDelay=	atoi(sDelayStr);
		iShDelay=	atoi(p0);
	}

	//�����ĸ��ļ���
	sprintf(sMktName,"%s/tdf_mk_%s.dat",sSourcePath,sCalcDate);
	sprintf(sTraName,"%s/tdf_tr_%s.dat",sSourcePath,sCalcDate);
	sprintf(sOrdName,"%s/tdf_or_%s.dat",sSourcePath,sCalcDate);

	sprintf(sD31Name,"%s/d31_t%d_%s.txt",sWorkRoot,iWriteFlag,sCalcDate);

        if((fpD31=fopen(sD31Name,"w"))==NULL){
                printf("error open file %s to write.\n",sD31Name);
                return -1;
        }

	//�õ������
	nBgnActionDay=	atoi(sCalcDate);
	nBgnTime=	atoi(sCalcBgn);
	nT0=		nBgnTime*1000;

	if(nBgnTime==93000)
		nPreT0=iAddMilliSec(nT0,-1000*1800);
	else	nPreT0=iAddMilliSec(nT0,-1000*60);

	while(1){

		tBeginTime=tGetHostTime();

		//Ϊ�˱��ⶩ�����ݳٵ�,������ǰȡ20�룬���������һ�ʣ��ֹ�����ʱ��
		if(IsBusyTime(nT0)){
			nEndTime0=iAddMilliSec(nT0,iSzBusyDelay+20000);
			if(nT0==MY_CLOSE_MARKET_TIME) nEndTime0=MY_CLOSE_MARKET_TIME+1000000;
		}
		else	nEndTime0=iAddMilliSec(nT0,iSzDelay+20000);

			
		//�������ڶ�������
		iOrdRes=MountOrdData2IndexStatArray(sOrdName,nBgnActionDay,nPreT0,nT0, 
			nEndTime0,sizeof(long long)+sizeof(TDF_ORDER),&lOrdCurPos);
		if(iOrdRes<0) return -1;

		if(IsBusyTime(nT0))
			nEndTime0=iAddMilliSec(nT0,iSzBusyDelay);
		else	nEndTime0=iAddMilliSec(nT0,iSzDelay);
			
		//�������ڡ����Ϻ��Ľ�������
		iTraRes=MountTrsData2IndexStatArray(sTraName,nBgnActionDay,nPreT0,nT0,
			nEndTime0,sizeof(long long)+sizeof(TDF_TRANSACTION),&lTraCurPos);
		if(iTraRes<0) return -1;
                          
		
		if(IsBusyTime(nT0))
			nEndTime0=iAddMilliSec(nT0,iShBusyDelay);
		else	nEndTime0=iAddMilliSec(nT0,iShDelay);


		iMktRes=MountQuotation2IndexStatArray(sMktName,nBgnActionDay,nPreT0,nT0,
			nEndTime0,sizeof(long long)+sizeof(TDF_MARKET_DATA),&lMktCurPos);
		if(iMktRes<0) return -1;

//next_step_all:
		//��������ɽ�ʱ�䶼δ�ﵽͳ�Ƶ㣬
		//�������ݵ�����û����ɨ������һ�£�
		if(iTraRes!=MY_WANT_STAT){

			int nCurTime=nGetHostCurTime();
						
			//���ʱ��������ʱ�䣬����
			if(nT0==130001000&&(
				nCurTime>=113000000&&nCurTime<125700000)){
				//����������
				usleep(180*1000000);
				continue;
			}
			//ֻ��Ҫ����ʵ�̻������ط�ʱ�򣬻طų���������������У�
			if(nCurTime>=113000000&&nCurTime<125700000){
				//����������
				usleep(180*1000000);
				continue;
			}
			usleep(iIdleWaitMilli*1000);
			continue;
		}

		int nCur=nGetHostCurTime();
		
		printf("hello world trar=%d,trap=%ld,ordr=%d,ordp=%ld,\tcur=%d,ms=%d.\n",
			iTraRes,lTraCurPos,iOrdRes,lOrdCurPos,nCur/1000,nCur%1000);

		//��һ��ѭ����D31������ͳ�Ƴ���
		if(GenD31StatAll()<0) return -1;

		//��D31�����������ļ�
		if(WriteD31StatAll(fpD31,sCodeStr,iWriteFlag)<0) return -1;

//		printf("hello world tp=%d,tc=%d.\n",nPreT0,nT0);

		nPreT0=nT0;

		//��������ˣ����˳���
		if(nT0>=150000000) break;

		//��������ʱ��ֱ������������У�������13��00��01��
		if(nT0>=113000000&&nT0<125959000)
			nT0=130001000;
		else	//����ʱ��1��ɨ��һ��
			nT0=iAddMilliSec(nT0,1000);

		//����һ��ʱ��˵�ORDER�ϲ�����ǰ���������У�
		//�������ʱ�䳬��nT0�򽫶���������ԭ������
//		if(MoveS1O2M_ORDERAll(nT0)<0) return -1;

		//����һ��ʱ��ε�Transaction�ϲ�����ǰ������
		//���Transactionʱ�����nT0�򣬼���������ԭ������
		//����һ��ʱ��ε�Quotation�ϲ�����ǰ������
//		MoveS1X2S0XAll(nPreT0,nT0);

		if(AddPreT0Data2Ready(nPreT0,nT0)<0) return -1;


		//���϶��رյ�ORDER����ڴ�
		//����Ϊ����������,ֻ�з�æʱ������ٴ���

		tDelTime=tGetHostTime();

		int iAskMaxCnt=0,iCnt=0;	//ASK_MAX���Ľڵ���
		int iBidMaxCnt=0;		//BID_MAX���Ľڵ���
		int iS0OCnt=0;			//M_ORDER���Ľڵ���
		int iFreeS0OCnt=0;		//�ͷŵ���S0O����
/*
		if(!IsBusyTime(nT0)){
			nLastTime=iAddMilliSec(nT0,-1000*60*5);
			pIndexStat=INDEX_HEAD;
			while(pIndexStat!=NULL){

				if(IsStopTime(nT0))
					DeleteCloseOrder(pIndexStat,nLastTime);

				iAskMaxCnt+=	pIndexStat->iAskMaxCnt;
				iBidMaxCnt+=	pIndexStat->iBidMaxCnt;
				iS0OCnt+=	pIndexStat->iS0OCnt;
				iFreeS0OCnt+=	pIndexStat->iFreeS0OCnt;
				iCnt++;

				pIndexStat=pIndexStat->pNext;
			}
		}
*/
		tEndTime=tGetHostTime();

		printf("hello world tp=%d,tc=%d.cost=%ld,%ld\t%d\t%d\t%d\t%d\t%d\n",
			nPreT0,nT0,tEndTime-tBeginTime,tEndTime-tDelTime,
			iCnt,iAskMaxCnt,iBidMaxCnt,iS0OCnt,iFreeS0OCnt);

	}

	fclose(fpD31);

	printf("hello world.\n");
	return 0;
}
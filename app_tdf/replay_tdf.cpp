//////////////////////////////////////////////////////////////////////////
/// @brief    基础API
///           实现不同方式的定阅和取消订阅及快照查询
//////////////////////////////////////////////////////////////////////////
#include "CallBackBase.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include "IoService.h"
#include "TinyThread.h"

#include "wwtiny.h"

#include "d31_item.h"

#include "public.h"

#define MY_DATE_CEIL_LONG 1000000000L

#define MY_AM_MARKET_BEGIN_TIME	90000000
#define MY_AM_MARKET_OPEN_TIME	93000000
#define MY_AM_MARKET_CLOSE_TIME	113000000
#define MY_PM_MARKET_OPEN_TIME	130000000
#define MY_PM_MARKET_CLOSE_TIME	150000000
#define MY_PM_MARKET_STOP_TIME	153000000
#define MY_DAY_END_TIME		240000000


int iWriteFlag=0,iDelayMilSec=100,iMultiTimes=1,iInfoSec=3,iFlag=1;

long lBgnTime=91500000L;
char sDispName[1024],sSourcePath[1024],sWorkRoot[1024],sReplayDate[15];

int iMktCnt=0,iTraCnt=0,iOrdCnt=0,iQueCnt=0,iDiCnt=0;
int iMktTime,iTraTime,iOrdTime,iQueTime,iDiTime;

CallBackBase *pCallBack;

void *MainReplayRunMkt(void *);
void *MainReplayRunTra(void *);
void *MainReplayRunOrd(void *);
void *MainReplayRunQue(void *);

void *MainReplayRunD31(void *);

void *MainReplayRunInfo(void *);


int main(int argc, char *argv[])
{

	string strWork;
	strcpy(sDispName,	"./disp.json");
	strcpy(sSourcePath,	"/stock/work");
	strcpy(sWorkRoot,	"/stock/work");

	for (int c; (c = getopt(argc, argv, "d:r:w:t:m:s:o:i:b:?:")) != EOF;){

		switch (c){
		case 'd':
			strncpy(sReplayDate, optarg,8);sReplayDate[8]=0;
			break;
		case 'r':
			strcpy(sDispName, optarg);
			break;
		case 'w':
			iWriteFlag=atoi(optarg);
			if(iWriteFlag!=1&&iWriteFlag!=2) iWriteFlag=0;
			break;
		case 't':
			iDelayMilSec=atoi(optarg);
			break;
		case 'm':
			iMultiTimes=atoi(optarg);
			if(iMultiTimes<=0) iMultiTimes=1;
			break;
		case 's':
			strcpy(sSourcePath, optarg);
			break;
		case 'o':
			strcpy(sWorkRoot, optarg);
			break;
		case 'i':
			iInfoSec=atoi(optarg);
			break;
		case 'f':
			iFlag=atoi(optarg);
			break;
		case 'b':
			lBgnTime=atoi(optarg);
			if(lBgnTime>=90000000L&&lBgnTime<153000000L)
				break;
			printf("please input correct format -b .\n");
		case '?':
		default:
			printf("Usage: %s \n", argv[0]);
			printf("   [-d replay-date (yyyymmdd) ]\n");
			printf("   [-r disp-name ]\n");
			printf("   [-w (1,writegta,2 writetdf,other nowrite) ]\n");
			printf("   [-t (query delay mil sec def=100ms) ]\n");
			printf("   [-m (multi_times) ]\n");
			printf("   [-s (source-path) ]\n");
			printf("   [-o work-root-name ]\n");
			printf("   [-i (infosec def=3) ]\n");
			printf("   [-f (flag  def=1 mk,or,tr,qu; 2 mkt,ord,tra,que) ]\n");
			printf("   [-b (begintime def=9:00:00:000,format hhmmssNNN) ]\n");
			exit(1);
			break;
		}
	}

	strWork=string(sWorkRoot);

	//初始化刷新数组，以及刷新文件名的全局变量
	InitUserArray(sDispName,&R);

	//刷新一下参数，避免要求disp先启动，才能启动本程序
	RefreshUserArray(sDispName,&R);

	IoService	ios;
	//订阅消息回调类
	CallBackBase CallbackBase(iWriteFlag,sReplayDate,strWork);
	CallbackBase.SetIoService(&ios);

	//启动处理数据服务
	ios.Start();


	//启动一个线程查询数据，并将数据加到工作线程中
	pCallBack=	&CallbackBase;

	pthread_t pthd_mkt;
	pthread_attr_t attr_mkt;

	//加载mkt回放线程
	pthread_attr_init(&attr_mkt);
	pthread_attr_setdetachstate(&attr_mkt, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr_mkt, 1024*512);
	pthread_create(&pthd_mkt, NULL, MainReplayRunMkt, NULL);

	printf("-----------------------------1.\n");
	
	pthread_t pthd_tra;
	pthread_attr_t attr_tra;

	//加载tra回放线程
	pthread_attr_init(&attr_tra);
	pthread_attr_setdetachstate(&attr_tra, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr_tra, 1024*512);
	pthread_create(&pthd_tra, NULL, MainReplayRunTra, NULL);
	
	printf("-----------------------------2.\n");

	pthread_t pthd_ord;
	pthread_attr_t attr_ord;

	//加载ord回放线程
	pthread_attr_init(&attr_ord);
	pthread_attr_setdetachstate(&attr_ord, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr_ord, 1024*512);
	pthread_create(&pthd_ord, NULL, MainReplayRunOrd, NULL);
	
	printf("-----------------------------3.\n");

	pthread_t pthd_que;
	pthread_attr_t attr_que;

	//加载que回放线程
	pthread_attr_init(&attr_que);
	pthread_attr_setdetachstate(&attr_que, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr_que, 1024*512);
	pthread_create(&pthd_que, NULL, MainReplayRunQue, NULL);

	printf("-----------------------------4.\n");

	pthread_t pthd_d31;
	pthread_attr_t attr_d31;

	//加载D31回放线程
	pthread_attr_init(&attr_d31);
	pthread_attr_setdetachstate(&attr_d31, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr_d31, 1024*512);
	pthread_create(&pthd_d31, NULL, MainReplayRunD31, NULL);

	printf("-----------------------------7.\n");

	//加载info回放线程
	pthread_t pthd_info;
	pthread_attr_t attr_info;
	pthread_attr_init(&attr_info);
	pthread_attr_setdetachstate(&attr_info, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr_info, 1024*512);
	pthread_create(&pthd_info, NULL, MainReplayRunInfo, NULL);

	printf("-----------------------------8.\n");


	//循环监视disp规则变化，如果变化则通知刷新
	WatchFileCloseWriteAndLock(sDispName);

	printf("-----------------------------2.\n");

	//正常代码不会运行到这里
	while(1){//主线程不能退出
		sleep(1);
	}

	//退出工作线程
	ios.Stop();

	getchar();

	return 0;
}
/***
//lBgnTime  ：指定回放开始时间位置
//lStartTime: 回放程序启动时间
//lCurTime  : 回放程序当前运行时间
// *plPickTime :原始记录落地时间

#define MY_AM_MARKET_BEGIN_TIME	90000000
#define MY_AM_MARKET_OPEN_TIME	93000000
#define MY_AM_MARKET_CLOSE_TIME	113000000
#define MY_PM_MARKET_OPEN_TIME	130000000
#define MY_PM_MARKET_CLOSE_TIME	150000000
#define MY_PM_MARKET_STOP_TIME	153000000
#define MY_DAY_END_TIME		240000000



关键点：
	判断回放时点lCurTime为何值时，到上午休市？
	判断回放时间点lCurTime为何值时超过了原始记录落地时间？
逻辑：
	lCurTime==>lPickTime
	lConvCur2ReplayTime: 将当前时间转换为回放时间逻辑如下：

	1，通过 指定回放开始时间算出，整个回放全过程需要多少时间；
	2，如果回放程序启动时间，到天结束整个够回放整个需要多长时间，则 直接映射这个时间；
	3，如果回放程序启动时间，不够整个结束时间，则做两个时间段的映射。
**/

//根据, 指定回放开始时间位置, 算出做完本次回放需要多长时间，单位（毫秒）
int GetReplayCostMSec(int nBgnTime)
{
	int iCostMSec=iDiffnTime(MY_PM_MARKET_STOP_TIME,nBgnTime);

	//如果是午盘则直接返回
	if(iCostMSec<=3600*2500) return iCostMSec;

	//如果是 大于  11:30:00:000 整，直接规整为下午一点
	if(iCostMSec<3600*4000) return  3600*2500;

	//如果是早盘，则扣除中间午间时间
	return iCostMSec-3600*1500;
}
//程序启动后，当前还剩余多少毫秒可以使用
int GetReplayDayLeftMSec(int nStartTime)
{
	return iDiffnTime(MY_DAY_END_TIME,nStartTime);
}
//程序启动后，预计程序在什么时间结束回放
int GetReplayEndTime(int nStartTime,int iCostMSec)
{
	int nEndTime=iAddMilliSec(nStartTime,iCostMSec);

	//这里包含了一个特殊点，最迟结束时间，永远不会到达
	if(nEndTime<=MY_DAY_END_TIME) return nEndTime;

	return nEndTime-MY_DAY_END_TIME;
}
//根据计算得出的：当前对应的回放位置（距离：MY_PM_MARKET_STOP_TIME的毫秒数），
//换算出ReplayTime的数值为
int GetReplayTimeByLeftMSec(int iEndLeftMSec)
{
	//如果时间点已经来到了下午，则直接算出时间
	if(iEndLeftMSec<=3600*2500)
		return iAddMilliSec(MY_PM_MARKET_STOP_TIME,-iEndLeftMSec);

	//否则还是上午，则直接多扣一个半小时
	return iAddMilliSec(MY_PM_MARKET_STOP_TIME,-(iEndLeftMSec+3600*1500));
}
//将程序运行当前时间，映射到回放位置上去
int nGetReplayTimeByCur(int nCurTime,int nStartTime,int nEndTime,
	int iCostMSec,int iDayLeftMSec)
{
	int iEndLeftMSec,iDay1EndLeftMSec;

	//一整天够用来回放整份数据
	if(iDayLeftMSec>=iCostMSec){
		iEndLeftMSec=iDiffnTime(nEndTime,nCurTime);
		return GetReplayTimeByLeftMSec(iEndLeftMSec);
	}

	//回放时间一天不够用,跨天，支持跨1天
	//设计回放最长时段为5小时,所以只可能跨一天.
	//剩余为第二天的的毫秒数

	//当前时间比会放开始时间还早了，则跨天了
	if(nCurTime<nStartTime){
		iEndLeftMSec=iDiffnTime(nEndTime,nCurTime);

		return GetReplayTimeByLeftMSec(iEndLeftMSec);
	}

	iDay1EndLeftMSec=iDiffnTime(MY_DAY_END_TIME,nCurTime);

	iEndLeftMSec=iDay1EndLeftMSec+(iCostMSec-iDayLeftMSec);

	return GetReplayTimeByLeftMSec(iEndLeftMSec);

}

void *MainReplayRunMkt(void *)
{
	int iCount=0,nStartTime,nEndTime,nCurTime,nPickTime,nReplayTime,iCostMSec,iDayLeftMSec;
	char sInFileName[1024],sBuffer[10240];
	long lItemLen=0;
	long long *plPickTime=(long long*)sBuffer;

	FILE *fpIn;

	TDF_MARKET_DATA *p=(TDF_MARKET_DATA*)(sBuffer+sizeof(long long));

	if(iFlag==1)
		sprintf(sInFileName,"%s/tdf_mk_%s.dat",sSourcePath,sReplayDate);
	else	sprintf(sInFileName,"%s/tdf_mkt_%s.dat",sSourcePath,sReplayDate);

	if((fpIn=fopen(sInFileName,"r"))==NULL){
		printf("error open file %s to read.\n",sInFileName);
		return NULL;
	}

	lItemLen=(sizeof(TDF_MARKET_DATA))+sizeof(long long);


	//确定开始时间
	nStartTime=	nGetHostCurTime();
	iCostMSec=	GetReplayCostMSec((int)lBgnTime);
	iDayLeftMSec=	GetReplayDayLeftMSec(nStartTime);
	nEndTime=	GetReplayEndTime(nStartTime,iCostMSec);

	while(1){

		//size_t fread ( void *buffer, size_t size, size_t count, FILE *stream) ;
		if(fread((void*)sBuffer,lItemLen,1,fpIn)!=1){
			printf("end of file break.\n");
			break;
		}

		if(p->nTime<lBgnTime)continue;

__delay:
		nCurTime=nGetHostCurTime();
		nPickTime=(int)((*plPickTime)%MY_DATE_CEIL_LONG);
		nReplayTime=nGetReplayTimeByCur(nCurTime,nStartTime,nEndTime,iCostMSec,iDayLeftMSec);

		//如果实盘数据采集的时间picktime
		//比当前系统时间对应的应当回放到的时间点大了，则线程等待
		if(nPickTime>nReplayTime){
			//休眠指定的时长
			usleep(iDelayMilSec*1000);
			goto __delay;
		}

		//发送multi-times次数据
		for(int j=0;j<iMultiTimes;j++){

			UTIL_Time stTime;
			PUTIL_GetLocalTime(&stTime);
			long long lCurTime=PUTIL_SystemTimeToDateTime(&stTime);

			//接收到数据后，先放入本地队列，等待数据处理接口处理
			MySubData *d = new MySubData;
			d->nItemCnt = 1;
			d->cur_time = lCurTime;
			d->data.assign((const char*)p, sizeof(TDF_MARKET_DATA));

			TaskPtr task(new Task(std::bind(&CallBackBase::Deal_Message_TdfMkt,pCallBack, d)));

			pCallBack->m_ios->Post(task);

//			if((++iCount)%50000==0)
//				printf("oz stock rt=%d,ct=%lld cur process count =%d.\n",
//					(int)(p->Time%MY_DATE_CEIL_LONG),lCurTime%MY_DATE_CEIL_LONG,iCount);
			iMktCnt=++iCount;
			iMktTime=p->nTime;
		}
		if(feof(fpIn)) break;
	}

	fclose(fpIn);

	printf("mkt stock rt=%d,ct=%d cur process count =%d.\n",
		(int)p->nTime,nCurTime,iCount);

	return NULL;
}
void *MainReplayRunTra(void *)
{
	int iCount=0,nStartTime,nEndTime,nCurTime,nPickTime,nReplayTime,iCostMSec,iDayLeftMSec;
	char sInFileName[1024],sBuffer[10240];
	long lItemLen=0;
	long long *plPickTime=(long long*)sBuffer;

	FILE *fpIn;

	TDF_TRANSACTION *p=(TDF_TRANSACTION*)(sBuffer+sizeof(long long));

	if(iFlag==1)
		sprintf(sInFileName,"%s/tdf_tr_%s.dat",sSourcePath,sReplayDate);
	else	sprintf(sInFileName,"%s/tdf_tra_%s.dat",sSourcePath,sReplayDate);

	if((fpIn=fopen(sInFileName,"r"))==NULL){
		printf("error open file %s to read.\n",sInFileName);
		return NULL;
	}

	lItemLen=(sizeof(TDF_TRANSACTION))+sizeof(long long);


	//确定开始时间
	nStartTime=	nGetHostCurTime();
	iCostMSec=	GetReplayCostMSec((int)lBgnTime);
	iDayLeftMSec=	GetReplayDayLeftMSec(nStartTime);
	nEndTime=	GetReplayEndTime(nStartTime,iCostMSec);

	while(1){

		//size_t fread ( void *buffer, size_t size, size_t count, FILE *stream) ;
		if(fread((void*)sBuffer,lItemLen,1,fpIn)!=1){
			printf("end of file break.\n");
			break;
		}

		if(p->nTime<lBgnTime)continue;

__delay:
		nCurTime=nGetHostCurTime();
		nPickTime=(int)((*plPickTime)%MY_DATE_CEIL_LONG);
		nReplayTime=nGetReplayTimeByCur(nCurTime,nStartTime,nEndTime,iCostMSec,iDayLeftMSec);

		//如果实盘数据采集的时间picktime
		//比当前系统时间对应的应当回放到的时间点大了，则线程等待
		if(nPickTime>nReplayTime){
			//休眠指定的时长
			usleep(iDelayMilSec*1000);
			goto __delay;
		}

		//发送multi-times次数据
		for(int j=0;j<iMultiTimes;j++){

			UTIL_Time stTime;
			PUTIL_GetLocalTime(&stTime);
			long long lCurTime=PUTIL_SystemTimeToDateTime(&stTime);

			//接收到数据后，先放入本地队列，等待数据处理接口处理
			MySubData *d = new MySubData;
			d->nItemCnt = 1;
			d->cur_time = lCurTime;
			d->data.assign((const char*)p, sizeof(TDF_TRANSACTION));

			TaskPtr task(new Task(std::bind(&CallBackBase::Deal_Message_TdfTra,pCallBack, d)));

			pCallBack->m_ios->Post(task);

//			if((++iCount)%50000==0)
//				printf("oz stock rt=%d,ct=%lld cur process count =%d.\n",
//					(int)(p->Time%MY_DATE_CEIL_LONG),lCurTime%MY_DATE_CEIL_LONG,iCount);
			iTraCnt=++iCount;
			iTraTime=p->nTime;
		}
		if(feof(fpIn)) break;
	}

	fclose(fpIn);

	printf("tra stock rt=%d,ct=%d cur process count =%d.\n",
		(int)p->nTime,nCurTime,iCount);

	return NULL;
}

void *MainReplayRunOrd(void *)
{
	int iCount=0,nStartTime,nEndTime,nCurTime,nPickTime,nReplayTime,iCostMSec,iDayLeftMSec;
	char sInFileName[1024],sBuffer[10240];
	long lItemLen=0;
	long long *plPickTime=(long long*)sBuffer;

	FILE *fpIn;

	TDF_ORDER *p=(TDF_ORDER*)(sBuffer+sizeof(long long));

	if(iFlag==1)
		sprintf(sInFileName,"%s/tdf_or_%s.dat",sSourcePath,sReplayDate);
	else	sprintf(sInFileName,"%s/tdf_ord_%s.dat",sSourcePath,sReplayDate);

	if((fpIn=fopen(sInFileName,"r"))==NULL){
		printf("error open file %s to read.\n",sInFileName);
		return NULL;
	}

	lItemLen=(sizeof(TDF_ORDER))+sizeof(long long);


	//确定开始时间
	nStartTime=	nGetHostCurTime();
	iCostMSec=	GetReplayCostMSec((int)lBgnTime);
	iDayLeftMSec=	GetReplayDayLeftMSec(nStartTime);
	nEndTime=	GetReplayEndTime(nStartTime,iCostMSec);

	while(1){

		//size_t fread ( void *buffer, size_t size, size_t count, FILE *stream) ;
		if(fread((void*)sBuffer,lItemLen,1,fpIn)!=1){
			printf("end of file break.\n");
			break;
		}

		if(p->nTime<lBgnTime)continue;

__delay:
		nCurTime=nGetHostCurTime();
		nPickTime=(int)((*plPickTime)%MY_DATE_CEIL_LONG);
		nReplayTime=nGetReplayTimeByCur(nCurTime,nStartTime,nEndTime,iCostMSec,iDayLeftMSec);

		//如果实盘数据采集的时间picktime
		//比当前系统时间对应的应当回放到的时间点大了，则线程等待
		if(nPickTime>nReplayTime){
			//休眠指定的时长
			usleep(iDelayMilSec*1000);
			goto __delay;
		}

		//发送multi-times次数据
		for(int j=0;j<iMultiTimes;j++){

			UTIL_Time stTime;
			PUTIL_GetLocalTime(&stTime);
			long long lCurTime=PUTIL_SystemTimeToDateTime(&stTime);

			//接收到数据后，先放入本地队列，等待数据处理接口处理
			MySubData *d = new MySubData;
			d->nItemCnt = 1;
			d->cur_time = lCurTime;
			d->data.assign((const char*)p, sizeof(TDF_ORDER));

			TaskPtr task(new Task(std::bind(&CallBackBase::Deal_Message_TdfOrd,pCallBack, d)));

			pCallBack->m_ios->Post(task);

//			if((++iCount)%50000==0)
//				printf("oz stock rt=%d,ct=%lld cur process count =%d.\n",
//					(int)(p->Time%MY_DATE_CEIL_LONG),lCurTime%MY_DATE_CEIL_LONG,iCount);
			iOrdCnt=++iCount;
			iOrdTime=p->nTime;
		}
		if(feof(fpIn)) break;
	}

	fclose(fpIn);

	printf("ord stock rt=%d,ct=%d cur process count =%d.\n",
		(int)p->nTime,nCurTime,iCount);

	return NULL;
}

void *MainReplayRunQue(void *)
{
	int iCount=0,nStartTime,nEndTime,nCurTime,nPickTime,nReplayTime,iCostMSec,iDayLeftMSec;
	char sInFileName[1024],sBuffer[10240];
	long lItemLen=0;
	long long *plPickTime=(long long*)sBuffer;

	FILE *fpIn;

	TDF_ORDER_QUEUE *p=(TDF_ORDER_QUEUE*)(sBuffer+sizeof(long long));

	if(iFlag==1)
		sprintf(sInFileName,"%s/tdf_qu_%s.dat",sSourcePath,sReplayDate);
	else	sprintf(sInFileName,"%s/tdf_que_%s.dat",sSourcePath,sReplayDate);

	if((fpIn=fopen(sInFileName,"r"))==NULL){
		printf("error open file %s to read.\n",sInFileName);
		return NULL;
	}

	lItemLen=(sizeof(TDF_ORDER_QUEUE))+sizeof(long long);


	//确定开始时间
	nStartTime=	nGetHostCurTime();
	iCostMSec=	GetReplayCostMSec((int)lBgnTime);
	iDayLeftMSec=	GetReplayDayLeftMSec(nStartTime);
	nEndTime=	GetReplayEndTime(nStartTime,iCostMSec);

	while(1){

		//size_t fread ( void *buffer, size_t size, size_t count, FILE *stream) ;
		if(fread((void*)sBuffer,lItemLen,1,fpIn)!=1){
			printf("end of file break.\n");
			break;
		}

		if(p->nTime<lBgnTime)continue;

__delay:
		nCurTime=nGetHostCurTime();
		nPickTime=(int)((*plPickTime)%MY_DATE_CEIL_LONG);
		nReplayTime=nGetReplayTimeByCur(nCurTime,nStartTime,nEndTime,iCostMSec,iDayLeftMSec);

		//如果实盘数据采集的时间picktime
		//比当前系统时间对应的应当回放到的时间点大了，则线程等待
		if(nPickTime>nReplayTime){
			//休眠指定的时长
			usleep(iDelayMilSec*1000);
			goto __delay;
		}

		//发送multi-times次数据
		for(int j=0;j<iMultiTimes;j++){

			UTIL_Time stTime;
			PUTIL_GetLocalTime(&stTime);
			long long lCurTime=PUTIL_SystemTimeToDateTime(&stTime);

			//接收到数据后，先放入本地队列，等待数据处理接口处理
			MySubData *d = new MySubData;
			d->nItemCnt = 1;
			d->cur_time = lCurTime;
			d->data.assign((const char*)p, sizeof(TDF_ORDER_QUEUE));

			TaskPtr task(new Task(std::bind(&CallBackBase::Deal_Message_TdfQue,pCallBack, d)));

			pCallBack->m_ios->Post(task);

//			if((++iCount)%50000==0)
//				printf("oz stock rt=%d,ct=%lld cur process count =%d.\n",
//					(int)(p->Time%MY_DATE_CEIL_LONG),lCurTime%MY_DATE_CEIL_LONG,iCount);
			iQueCnt=++iCount;
			iQueTime=p->nTime;
		}
		if(feof(fpIn)) break;
	}

	fclose(fpIn);

	printf("que stock rt=%d,ct=%d cur process count =%d.\n",
		(int)p->nTime,nCurTime,iCount);

	return NULL;
}

void *MainReplayRunD31(void *)
{	
	int iCount=0,nStartTime,nEndTime,nCurTime,nPickTime,nReplayTime,iCostMSec,iDayLeftMSec;
	char sInFileName[1024],sBuffer[10240];
	long lItemLen=0;
	long long *plPickTime=(long long*)sBuffer;

	FILE *fpIn;

	struct D31ItemStruct *p=(struct D31ItemStruct*)(sBuffer+sizeof(long long));

	sprintf(sInFileName,"%s/d31_t3_%s.dat",sSourcePath,sReplayDate);

	if((fpIn=fopen(sInFileName,"r"))==NULL){
		printf("error open file %s to read.\n",sInFileName);
		return NULL;
	}

	lItemLen=(sizeof(struct D31ItemStruct))+sizeof(long long);

	//确定开始时间
	nStartTime=	nGetHostCurTime();
	iCostMSec=	GetReplayCostMSec((int)lBgnTime);
	iDayLeftMSec=	GetReplayDayLeftMSec(nStartTime);
	nEndTime=	GetReplayEndTime(nStartTime,iCostMSec);

	//D31格式专用的两个字段
	int nT0;
	char sTime[15];

	while(1){

		//size_t fread ( void *buffer, size_t size, size_t count, FILE *stream) ;
		if(fread((void*)sBuffer,lItemLen,1,fpIn)!=1){
			printf("end of file break.\n");
			break;
		}

		sFormatTime((time_t)p->nTradeTime,sTime);
		nT0=atoi(sTime+8)*1000;

		if(nT0<lBgnTime)continue;

__delay:
		nCurTime=nGetHostCurTime();
		nPickTime=(int)((*plPickTime)%MY_DATE_CEIL_LONG);
		nReplayTime=nGetReplayTimeByCur(nCurTime,nStartTime,nEndTime,iCostMSec,iDayLeftMSec);

		//如果实盘数据采集的时间picktime
		//比当前系统时间对应的应当回放到的时间点大了，则线程等待
		if(nPickTime>nReplayTime){
			//休眠指定的时长
			usleep(iDelayMilSec*1000);
			goto __delay;
		}

		//发送multi-times次数据
		for(int j=0;j<iMultiTimes;j++){

			UTIL_Time stTime;
			PUTIL_GetLocalTime(&stTime);
			long long lCurTime=PUTIL_SystemTimeToDateTime(&stTime);

			//接收到数据后，先放入本地队列，等待数据处理接口处理
			MySubData *d = new MySubData;
			d->nItemCnt = 1;
			d->cur_time = lCurTime;
			d->data.assign((const char*)p, sizeof(struct D31ItemStruct));

			TaskPtr task(new Task(std::bind(&CallBackBase::Deal_Message_D31Item,pCallBack, d)));

			pCallBack->m_ios->Post(task);

			iDiCnt=++iCount;
			iDiTime=(int)(nT0%MY_DATE_CEIL_LONG);
		}
		if(feof(fpIn)) break;
	}

	fclose(fpIn);

	printf("di stock rt=%d,ct=%d cur process count =%d.\n",
		nT0,nCurTime,iCount);

	return NULL;
}

void *MainReplayRunInfo(void *)
{
	int iCount=0,nCurTime;
	while(1){
		sleep(iInfoSec);

		nCurTime=nGetHostCurTime();

/*		printf("qh=%-9dqht=%-10dth=%-9dtht=%-10dah=%-9daht=%-10d\n\
qz=%-9dqzt=%-10dtz=%-9dtzt=%-10doz=%-9dozt=%-10dt=%lld\n",
			iQhCnt,iQhTime,iThCnt,iThTime,iAhCnt,iAhTime,
			iQzCnt,iQzTime,iTzCnt,iTzTime,iOzCnt,iOzTime,lCurTime/1000);
*/
		if(iCount++%10==0)
			printf("mkt(%-9d)\ttra(%-9d)\tord(%-9d)\tque(%-9d)\tdi(%-9d)\tcurtime\n",
				 iMktTime,iTraTime,iOrdTime,iQueTime,iDiTime);
		printf("%-9d\t%-9d\t%-9d\t%-9d\t%-9d\t%d\n",
			iMktCnt,iTraCnt,iOrdCnt,iQueCnt,iDiCnt,nCurTime/1000);

	}
	return NULL;
}

#include <STDIO.H>    
#include <CSL.H>    
#include <CSL_IRQ.H>    
#include <CSL_CHIP.H>    
#include <CSL_EMIF.H>    
#include <CSL_IRQ.H> 
#include <MATH.H>
#include <yj6701.h>

void emifSend(unsigned short *starCnt,short moveCnt)
{
	unsigned short	frm=0,k=0,row=0,col=0;
	short			i=0,j=0;
	unsigned short	img[225];			//抠图
	unsigned char	emifBuf[256];		//EMIF数据缓存

	g_emifImg		= (unsigned int *)0x033C0000;			//EMIF发送
	memset(emifBuf,0,256);

	//发送辅助数据包
	/*------------------------------------------------------------------------*/
	emifBuf[0] = 0xEB;	emifBuf[1] = 0x90;									//帧头
	/*------------------------------------------------------------------------*/
	emifBuf[2] = g_frameNum/10/256;	emifBuf[3] = g_frameNum/10%256;			//组序号
	/*------------------------------------------------------------------------*/
	emifBuf[4] = g_thresholdMW;												//去噪索引
	/*------------------------------------------------------------------------*/
	emifBuf[5] = g_thresholdBW/256;	emifBuf[6] = g_thresholdBW%256;			//二值化阈值
	/*------------------------------------------------------------------------*/
	emifBuf[7] = g_thresholdSTAR/256; emifBuf[8] = g_thresholdSTAR%256;		//恒星提取数目
	/*------------------------------------------------------------------------*/
	emifBuf[11] = moveCnt%256;												//GEO目标数
	/*------------------------------------------------------------------------*/
	emifBuf[12] = 0;														//小帧序号
	/*------------------------------------------------------------------------*/
	emifBuf[13] = g_thresholdBCK/256;	emifBuf[14] = g_thresholdBCK%256;	//背景预处理
	/*------------------------------------------------------------------------*/
	memcpy(emifBuf+15,(unsigned char *)(&g_frameNumber),2);					//该组帧起始
	/*------------------------------------------------------------------------*/
	memcpy(emifBuf+17,(unsigned char *)g_captureTime[0],7);					//时间码
	memcpy(emifBuf+24,(unsigned char *)g_captureTime[1],7);
	memcpy(emifBuf+31,(unsigned char *)g_captureTime[2],7);
	memcpy(emifBuf+38,(unsigned char *)g_captureTime[3],7);
	memcpy(emifBuf+45,(unsigned char *)g_captureTime[4],7);
	memcpy(emifBuf+52,(unsigned char *)g_controlWord,4);					//控制字
	memcpy(emifBuf+56,(unsigned char *)(g_controlWord+1),4);
	memcpy(emifBuf+60,(unsigned char *)(g_controlWord+2),4);
	memcpy(emifBuf+64,(unsigned char *)(g_controlWord+3),4);
	memcpy(emifBuf+68,(unsigned char *)(g_controlWord+4),4);
	emifBuf[72] = (unsigned char)starCnt[0];								//恒星数
	emifBuf[73] = (unsigned char)starCnt[1];								//恒星数
	emifBuf[74] = (unsigned char)starCnt[2];								//恒星数
	emifBuf[75] = (unsigned char)starCnt[3];								//恒星数
	emifBuf[76] = (unsigned char)starCnt[4];								//恒星数
	/*------------------------------------------------------------------------*/
	emifBuf[253] = 0;
	for(j=0;j<253;j++){
		emifBuf[253] += emifBuf[j];}										//校验和
	/*------------------------------------------------------------------------*/
	emifBuf[254] = 0x09;	emifBuf[255] = 0xD7;							//帧尾
	/*------------------------------------------------------------------------*/
	for(j=0;j<64;j++){
		*g_emifImg = *(unsigned int *)(emifBuf+j*4);}

	//发送恒星
	for(frm=0;frm<5;frm++)			//循环5帧
	{
		for(i=0;i<starCnt[frm];i++)		//循环恒星个数
		{
			k	= 0;
			memset(img,0,450);
			row	= (unsigned short)g_starPosi[frm][i*3];
			col	= (unsigned short)g_starPosi[frm][i*3+1];
			for(j=row-7;j<=row+7;j++)	//图像数据
			{
				if((j>=0) && (j<=1023)){
				if(col < 7)
					memcpy(img+k+(7-col),(unsigned char *)(g_imgOrig[frm]+j*IMG_WID),(8+col)*2);
				else if(col > 1016)
					memcpy(img+k,(unsigned char *)(g_imgOrig[frm]+j*IMG_WID+col-7),(1023-col+8)*2);
				else
					memcpy(img+k,(unsigned char *)(g_imgOrig[frm]+j*IMG_WID+col-7),30);
				k += 15;}
			}
			/*------------------------------------------------------------------------*/
			emifBuf[9] = frm+1;											//帧序号
			/*------------------------------------------------------------------------*/
			emifBuf[10] = starCnt[frm]/256;	emifBuf[11] = starCnt[frm]%256;		//目标总数
			/*------------------------------------------------------------------------*/
			emifBuf[12]=1;												//小帧序号
			/*------------------------------------------------------------------------*/
			emifBuf[13] = i/256;	emifBuf[14] = i%256;				//目标序号
			/*------------------------------------------------------------------------*/
			emifBuf[15] = row/256;										//行--中心
			emifBuf[16] = row%256;
			emifBuf[17] = col/256;										//列--中心
			emifBuf[18] = col%256;
			/*------------------------------------------------------------------------*/
			emifBuf[19]	= *(((unsigned char*)g_starPosi[frm])+i*12+0);			//行--质心
			emifBuf[20] = *(((unsigned char*)g_starPosi[frm])+i*12+1);
			emifBuf[21] = *(((unsigned char*)g_starPosi[frm])+i*12+2);
			emifBuf[22] = *(((unsigned char*)g_starPosi[frm])+i*12+3);
			emifBuf[23]	= *(((unsigned char*)g_starPosi[frm])+i*12+4);			//列--质心
			emifBuf[24] = *(((unsigned char*)g_starPosi[frm])+i*12+5);
			emifBuf[25] = *(((unsigned char*)g_starPosi[frm])+i*12+6);
			emifBuf[26] = *(((unsigned char*)g_starPosi[frm])+i*12+7);
			/*------------------------------------------------------------------------*/
			emifBuf[27] = 0;											//目标类型(0恒星,1运动)
			/*------------------------------------------------------------------------*/
			memcpy(emifBuf+28,img,225);
			/*------------------------------------------------------------------------*/
			emifBuf[253] = 0;
			for(j=0;j<253;j++){
				emifBuf[253] += emifBuf[j];}							//校验和
			/*------------------------------------------------------------------------*/
			for(j=0;j<64;j++){
				*g_emifImg = *(unsigned int *)(emifBuf+j*4);}
			
			for(j=0;j<2000;j++){;}
			
			emifBuf[12]=2;												//小帧序号
			memcpy(emifBuf+28,(unsigned char *)img+225,225);
			/*------------------------------------------------------------------------*/
			emifBuf[253] = 0;
			for(j=0;j<253;j++){
				emifBuf[253] += emifBuf[j];}							//校验和
			/*------------------------------------------------------------------------*/
			for(j=0;j<64;j++){
				*g_emifImg = *(unsigned int *)(emifBuf+j*4);}
		}
	}
	for(frm=0;frm<5;frm++)
	{
		for(i=0;i<moveCnt;i++)		//循环运动目标个数
		{
			k	= 0;
			memset(img,0,450);
			row	= (unsigned short)g_allObjInf[15*i+3*frm];
			col	= (unsigned short)g_allObjInf[15*i+3*frm+1];
			for(j=row-7;j<=row+7;j++)	//图像数据
			{
				if((j>=0) && (j<=1023)){
				if(col < 7)
					memcpy(img+k+col,(unsigned char *)(g_imgOrig[frm]+j*IMG_WID),30-col*2);
				else if(col > 1016)
					memcpy(img+k,(unsigned char *)(g_imgOrig[frm]+j*IMG_WID+col-7),(1023-col+8)*2);
				else
					memcpy(img+k,(unsigned char *)(g_imgOrig[frm]+j*IMG_WID+col-7),30);
				k += 15;}
			}
			
			/*------------------------------------------------------------------------*/
			emifBuf[9] = frm+1;											//帧序号
			/*------------------------------------------------------------------------*/
			emifBuf[10] = moveCnt/256;	emifBuf[11] = moveCnt%256;		//目标总数
			/*------------------------------------------------------------------------*/
			emifBuf[12]=1;												//小帧序号
			/*------------------------------------------------------------------------*/
			emifBuf[13] = i/256;	emifBuf[14] = i%256;				//目标序号
			/*------------------------------------------------------------------------*/
			emifBuf[15] = row/256;										//行--中心
			emifBuf[16] = row%256;
			emifBuf[17] = col/256;										//列--中心
			emifBuf[18] = col%256;
			/*------------------------------------------------------------------------*/
			emifBuf[19]	= *(((unsigned char*)g_allObjInf)+60*i+12*frm);			//行--质心
			emifBuf[20] = *(((unsigned char*)g_allObjInf)+60*i+12*frm+1);
			emifBuf[21] = *(((unsigned char*)g_allObjInf)+60*i+12*frm+2);
			emifBuf[22] = *(((unsigned char*)g_allObjInf)+60*i+12*frm+3);
			emifBuf[23]	= *(((unsigned char*)g_allObjInf)+60*i+12*frm+4);		//列--质心
			emifBuf[24] = *(((unsigned char*)g_allObjInf)+60*i+12*frm+5);
			emifBuf[25] = *(((unsigned char*)g_allObjInf)+60*i+12*frm+6);
			emifBuf[26] = *(((unsigned char*)g_allObjInf)+60*i+12*frm+7);
			/*------------------------------------------------------------------------*/
			emifBuf[27] = 1;											//目标类型(0恒星,1运动)
			/*------------------------------------------------------------------------*/
			memcpy(emifBuf+28,img,225);
			/*------------------------------------------------------------------------*/
			emifBuf[253] = 0;
			for(j=0;j<253;j++){
				emifBuf[253] += emifBuf[j];}							//校验和
			/*------------------------------------------------------------------------*/
			for(j=0;j<64;j++){
				*g_emifImg = *(unsigned int *)(emifBuf+j*4);}
				
			for(j=0;j<2000;j++){;}
			emifBuf[12]=2;												//小帧序号
			memcpy(emifBuf+28,(unsigned char *)img+225,225);
			/*------------------------------------------------------------------------*/
			emifBuf[253] = 0;
			for(j=0;j<253;j++){
				emifBuf[253] += emifBuf[j];}							//校验和
			/*------------------------------------------------------------------------*/
			for(j=0;j<64;j++){
				*g_emifImg = *(unsigned int *)(emifBuf+j*4);}
		}
	}
}


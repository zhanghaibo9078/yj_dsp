#include <stdio.h>
#include "../include/yj6701.h"
#include <string.h>

/*****************************************参数*********************************************/
unsigned short	g_thresholdBW	= 150;								//二值化阈值
unsigned short	g_thresholdMW	= 3;								//去噪阈值
unsigned short	g_thresholdSTAR	= 150;								//恒星目标总数
unsigned short	g_thresholdBCK	= 7;								//背景预处理
unsigned int	g_controlWord[5]= {0,0,0,0,0};						//控制参数
unsigned int	g_captureTime[5][2]= {{0,0},{0,0},{0,0},{0,0},{0,0}};//拍照时间

unsigned int	g_frameNumber	= 0;								//图像的帧计数
unsigned int	g_frameNum		= 0;								//帧号
unsigned int	g_frameNum_r	= 0;								//帧号_r
/******************************************************************************************/
																							
/***************************************配准***********************************************/
unsigned char	*g_regLabelMap = (unsigned char *)REG_LABEL;		//配准连通域图(1024*1024B)
struct star		*g_stars[5]	={  (struct star *)STARS_0,
								(struct star *)STARS_1,
								(struct star *)STARS_2,
								(struct star *)STARS_3,
								(struct star *)STARS_4};		//配准恒星信息
struct tri		*g_tris[5]	={  (struct tri *)TRIS_0,
								(struct tri *)TRIS_1,
								(struct tri *)TRIS_2,
								(struct tri *)TRIS_3,
								(struct tri *)TRIS_4};			//配准三角信息
unsigned char	g_triCnt[5];									//配准三角个数

short			g_offsetR[5]	= {0,0,0,0,0};						//行偏移量
short			g_offsetC[5]	= {0,0,0,0,0};						//列偏移量
/******************************************************************************************/

/***************************************origin*********************************************/
unsigned int 	*g_emifImg		=	(unsigned int   *)0x03000000;	//EMIF接收和发送数据
unsigned short	*g_imgOrig[5]	= { (unsigned short *)ORI_IMG_0,
									(unsigned short *)ORI_IMG_1,
									(unsigned short *)ORI_IMG_2,
									(unsigned short *)ORI_IMG_3,
									(unsigned short *)ORI_IMG_4};	//原始图像存储指针
unsigned short	*g_imgRmb[5]	= { (unsigned short *)RMB_IMG_0,
									(unsigned short *)RMB_IMG_1,
									(unsigned short *)RMB_IMG_2,
									(unsigned short *)RMB_IMG_3,
									(unsigned short *)RMB_IMG_4};	//去除背景后图像存储指针
/******************************************************************************************/

/****************************************star**********************************************/
unsigned short	 g_starSum[5]	=	{0,0,0,0,0};					//恒星目标总数
float			 g_starPosi[5][160*3];								//恒星目标细则(3列*160行)
/******************************************************************************************/

/****************************************move**********************************************/
unsigned short	s_origVal[5][1024];
unsigned char	s_maxIdx[1024];
unsigned char	*g_maxIndex		=	(unsigned char  *)MAX_INDEX;	//最大值索引帧
unsigned char	*g_corImg		=	(unsigned char  *)COR_MAX_IDX;	//去噪后
unsigned short	 g_objNum[5]	=	{0,0,0,0,0};					//每帧图像连通域数(5行1列)
float			*g_objPntLst	=	(float *)OBJ_PNT_LST;			//存储帧号、连通域、及粗略坐标
float			*objPointF[5]	={	(float *)OBJ_PNT_F0,
									(float *)OBJ_PNT_F1,
									(float *)OBJ_PNT_F2,
									(float *)OBJ_PNT_F3,
									(float *)OBJ_PNT_F4};			//第0,1,2,3,4帧图连通域的坐标信息
float			*g_allObjInf	=	(float *)OBJ_MOV_INFO;			//运动目标细则(5行3列的倍数)
unsigned short	 g_moveSum		=	0;								//运动目标总数
/******************************************************************************************/

/****************************************注入**********************************************/
unsigned short	g_injectNum		= 0;								//注入程序中断计数
unsigned short	g_injectNum_r	= 0;								//注入程序中断计数_r
unsigned short	g_injectSum		= 0;								//该程序段的中断总次数
unsigned int	*g_AreaEE		= (unsigned int   *)0x00400000;		//将注入数据暂存的地址
unsigned int	g_lenArea00		= 0;								//00区占字节数
unsigned int	g_lenArea80		= 0;								//80区占字节数
unsigned int	g_stepInject	= 0;								//注入进行到的步骤
/******************************************************************************************/

/**************************************函数声明********************************************/
extern void matchMain(unsigned int curID,unsigned short **img);
extern void star(float starList[5][480], unsigned short *starNum);
extern void move(void);
extern void emifSend(unsigned short *starCnt,short moveCnt);
extern void para2value(unsigned int *para);
/******************************************************************************************/
#ifdef TI_PLATFORM
extern void SetupInterrupts(void);
extern void emifRecvImg(void);
extern void emifRecvEE(void);
extern void beginInject(void);

/****************************************中断**********************************************/
interrupt void FrameIsr4 (void)
{
	if(g_frameNum%10 == 0)							//1组更新一次参数
	{
		g_thresholdBW	= (unsigned short)(*(unsigned int *)0x03080000);	//"0010"二值化阈值
		g_thresholdMW	= (unsigned short)(*(unsigned int *)0x030C0000);	//"0011"去噪索引
		g_thresholdSTAR = (unsigned short)(*(unsigned int *)0x03100000);	//"0100"恒星提取数目
		g_thresholdBCK	= (unsigned short)(*(unsigned int *)0x031C0000);	//"0111"背景预处理
		g_frameNumber	= (unsigned short)(*(unsigned int *)0x03200000);	//"1000"本组帧起始
	}

	g_frameNum++;

	return;
}

interrupt void EEIsr5 (void)
{
	g_injectNum++;

	return;
}
/******************************************************************************************/

void main(void)
{
	*(unsigned int *)EMIF_GBLCTL	= 0x000037DB;	//GBLCTL
	*(unsigned int *)EMIF_CE1		= 0xFFFF3F03;	//CE1
	*(unsigned int *)EMIF_CE0		= 0xFFF10533;	//CE0
	*(unsigned int *)EMIF_CE2		= 0xFFFF3F33;	//CE2
	*(unsigned int *)EMIF_CE3		= 0x21520F21;	//CE3
	*(unsigned int *)EMIF_SDCTL		= 0x03115000;	//SDCTL
	SetupInterrupts();								//设置中断

	memset((unsigned char *)0x00400000,0,IMG_WID*IMG_HEI*2);
	while(1)
	{
		if((g_frameNum_r < g_frameNum) && (g_injectNum == 0))//判断接收到新的图像并且不注入
		{
			emifRecvImg();									//接收图像
			if((g_frameNum_r%2) && (g_frameNum_r != 0))		//配准
			{
				g_controlWord[g_frameNum_r/2%5]		= *(unsigned int *)0x03140000;		//"0101"控制参数
				g_captureTime[g_frameNum_r/2%5][0]	= *(unsigned int *)0x03240000;		//"1001"时间秒值
				g_captureTime[g_frameNum_r/2%5][1]	= *(unsigned int *)0x03280000;		//"1010"时间微秒值
				matchMain(g_frameNum_r/2%5,g_imgRmb);
			}
			g_frameNum_r = g_frameNum;						//更新帧号_r
			if((g_frameNum%10 == 0) && (g_frameNum != 0))	//当接收够5帧图像
			{
				para2value(g_controlWord);
				star(g_starPosi,g_starSum);					//确定恒星目标
				move();										//确定运动目标
				emifSend(g_starSum,g_moveSum);				//发送数据
				if(g_frameNum>=172800)
				{
					g_frameNum = 0;
					g_frameNum_r = 0;
				}
			}
		}
		else if(g_injectNum_r < g_injectNum)
		{
			emifRecvEE();							//将接收到的数据写入SDRAM
			g_injectNum_r++;						//更新中断号_r
			if(g_injectNum == g_injectSum)			//当接收完
			{
				beginInject();
			}
		}
		else
		{
			*(unsigned char *)0x00e00000 = 0x55;
			*(unsigned char *)0x00e00000 = 0xAA;
		}
	}
}
#else
int main(void)
{
	memset((unsigned char *)0x00400000,0,IMG_WID*IMG_HEI*2);
	while(1)
	{
		if((g_frameNum_r < g_frameNum) && (g_injectNum == 0))//判断接收到新的图像并且不注入
		{
			if((g_frameNum_r%2) && (g_frameNum_r != 0))		//配准
			{
				g_controlWord[g_frameNum_r/2%5]		= *(unsigned int *)0x03140000;		//"0101"控制参数
				g_captureTime[g_frameNum_r/2%5][0]	= *(unsigned int *)0x03240000;		//"1001"时间秒值
				g_captureTime[g_frameNum_r/2%5][1]	= *(unsigned int *)0x03280000;		//"1010"时间微秒值
				matchMain(g_frameNum_r/2%5,g_imgRmb);
			}
			g_frameNum_r = g_frameNum;						//更新帧号_r
			if((g_frameNum%10 == 0) && (g_frameNum != 0))	//当接收够5帧图像
			{
				para2value(g_controlWord);
				star(g_starPosi,g_starSum);					//确定恒星目标
				move();										//确定运动目标
				emifSend(g_starSum,g_moveSum);				//发送数据
				if(g_frameNum>=172800)
				{
					g_frameNum = 0;
					g_frameNum_r = 0;
				}
			}
		}
	}

	return 1;
}
#endif

/****************************************
    This is head program
    Auther by duanxiaofeng
*****************************************/
#ifndef GCC_PLATFORM
#define GCC_PLATFORM
#endif

#ifndef YJ6701_H_
#define YJ6701_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <STDIO.H>
#include <string.h>
#include <malloc.h>
/**********************************************************************************/
/*********************内存映射采用map1方式*****************************************/
/********0000 0000 - 0000 FFFF     64KBYTE   程序存储区****************************/
/********0040 0000 - 00FF FFFF     12MBYTE   CE0图像存储区0 ***********************/
/********0100 0000 - 0013 FFFF     04MBYTE   CE0图像存储区1 ***********************/
/********0140 0000 - 0140 FFFF     64KBYTE   CE1程序存储区0 ***********************/
/********0141 0000 - 017F FFFF     04MBYTE   CE1程序存储区1 ***********************/
/********0200 0000 - 02FF FFFF     16MBYTE   CE2图像存储区2 ***********************/
/********0300 0000 - 03FF FFFF     16MBYTE   CE3 EMIF 接口  ***********************/ 
/**********************************************************************************/

/***EMIF***/
#define EMIF_GBLCTL		0x01800000
#define EMIF_CE1		0x01800004
#define EMIF_CE0		0x01800008
#define EMIF_CE2		0x01800010
#define EMIF_CE3		0x01800014
#define EMIF_SDCTL		0x01800018

/***EEPROM***/
#define EE150			0x01500000
#define EE158       	0x01580000

/**图像区ce0 16Mbyte	0x00400000-0x013FFFFF**/
#define ORI_IMG_0		0x00400000		//g_imgOrig[0]
#define ORI_IMG_1		0x00600000		//g_imgOrig[1]
#define ORI_IMG_2		0x00800000		//g_imgOrig[2]
#define ORI_IMG_3		0x00A00000		//g_imgOrig[3]
#define ORI_IMG_4		0x00C00000		//g_imgOrig[4]
#define RMB_IMG_0		0x00E00000		//g_imgRmb[0]
#define RMB_IMG_1		0x01000000		//g_imgRmb[1]
#define RMB_IMG_2		0x01200000		//g_imgRmb[2]

/**图像区ce2 16Mbyte	0x02000000-0x02FFFFFF**/
#define RMB_IMG_3		0x02000000		//g_imgRmb[3]
#define RMB_IMG_4		0x02200000		//g_imgRmb[4]

#define MAX_INDEX		0x02400000		//g_maxIndex
#define COR_MAX_IDX		0x02500000		//g_corImg

#define OBJ_PNT_LST		0x0270B000		//g_objPntLst(最大4*4096/16=1024个连通域)
#define OBJ_PNT_F0		0x0270F000      //objPointF[0]
#define OBJ_PNT_F1		0x02711000      //objPointF[1]
#define OBJ_PNT_F2		0x02713000      //objPointF[2]
#define OBJ_PNT_F3		0x02715000      //objPointF[3]
#define OBJ_PNT_F4		0x02717000      //objPointF[4]
#define OBJ_MOV_INFO	0x02719000		//g_allObjInf(最大2*4096/(5行*3列*4)=68*2个运动目标)
#define	EMIF_OUT		0x0271B000		//emifBuf

#define REG_LABEL		0x02800000		//g_regLabelMap(1024*1024B)
#define STARS_0			0x02900000		//g_stars[0](8B*10=80B)
#define STARS_1			0x02900050		//g_stars[1](8B*10=80B)
#define STARS_2			0x029000A0		//g_stars[2](8B*10=80B)
#define STARS_3			0x029000F0		//g_stars[3](8B*10=80B)
#define STARS_4			0x02900140		//g_stars[4](8B*10=80B)
#define TRIS_0			0x02900190		//g_tris[0](12B*120=1440B)
#define TRIS_1			0x02900730		//g_tris[1](12B*120=1440B)
#define TRIS_2			0x02900CD0		//g_tris[2](12B*120=1440B)
#define TRIS_3			0x02901270		//g_tris[3](12B*120=1440B)
#define TRIS_4			0x02901810		//g_tris[4](12B*120=1440B)

// 图像参数
#define IMG_WID			1024
#define IMG_HEI			1024
#define IMG_FRM			1048576
#define REG_STAR_CNT	8						//配准恒星数目
#define REG_TRI_CNT		56						//配准三角形数目

/*****************************************参数*********************************************/	
extern unsigned short	g_thresholdBW;			//二值化阈值	
extern unsigned short	g_thresholdMW;			//去噪阈值		
extern unsigned short	g_thresholdSTAR;		//恒星目标数
extern unsigned short	g_thresholdBCK;			//背景预处理
extern unsigned int		g_controlWord[5];		//控制参数
extern unsigned int		g_captureTime[5][2];	//拍照时间
	
extern unsigned int		g_frameNumber;			//图像的帧计数
extern unsigned int		g_frameNum;				//帧号	
extern unsigned int		g_frameNum_r;			//帧号_r	

/***************************************配准***********************************************/
// 恒星结构体 Structure for stars
struct star
{
	unsigned short r;							//恒星所在行	Row
	unsigned short c;							//恒星所在列	Column
	unsigned short b;							//恒星亮度		Brightness
	short label;								//恒星代号		Label
};
// 三角形结构体 Structure for triangles
struct tri
{
	unsigned short s[3];						//三角形三边长		Edges of triangle
	short labels[3];							//三角形三个顶点编号	Labels of vertex

};
extern unsigned char	*g_regLabelMap;			//配准连通域图
extern struct star		*g_stars[5];			//配准恒星信息
extern struct tri		*g_tris[5];				//配准三角信息
extern unsigned char	 g_triCnt[5];			//配准三角个数

extern short			 g_offsetR[5];			//行偏移量
extern short			 g_offsetC[5];			//列偏移量

/***************************************origin*********************************************/
extern unsigned int 	*g_emifImg;				//EMIF接收和发送图像数据
extern unsigned short	*g_imgOrig[5];			//原始图像存储指针
extern unsigned short	*g_imgRmb[5];			//去除背景后图像存储指针

/***************************************search*********************************************/

/****************************************star**********************************************/
extern unsigned short	 g_starSum[5];			//恒星目标总数
extern float			 g_starPosi[5][160*3];	//恒星目标细则(3列*160行)

/****************************************move**********************************************/
extern unsigned short	 s_origVal[5][1024];	//缓存区
extern unsigned char	 s_maxIdx[1024];     	//缓存区
extern unsigned char	*g_maxIndex;			//最大值索引帧
extern unsigned char	*g_corImg;				//去噪后	
extern unsigned short	 g_objNum[5];			//每帧图像连通域数(5行1列)	
extern float			*g_objPntLst;			//存储帧号、连通域、及粗略坐标	
extern float			*objPointF[5];			//第0,1,2,3,4帧图连通域的坐标信息
extern float			*g_allObjInf;			//运动目标细则	
extern unsigned short	 g_moveSum;				//运动目标总数

/******************************注入************************************/
extern unsigned short	 g_injectNum;			//注入程序中断计数
extern unsigned short	 g_injectNum_r;			//注入程序中断计数_r
extern unsigned short	 g_injectSum;			//该程序段的中断总次数
extern unsigned int		*g_AreaEE;				//将注入数据暂存的地址
extern unsigned int		 g_lenArea00;			//00区占字节数
extern unsigned int	 	 g_lenArea80;			//80区占字节数
extern unsigned int	 	 g_stepInject;			//注入进行到的步骤
/**********************************************************************/
#ifdef GCC_PLATFORM
extern void init();
extern int readFile(char *str,int frmBeg,unsigned int time[5][2], unsigned int *framePara,unsigned int *control_word);
extern void filter(unsigned short **inImg,unsigned short **outImg,unsigned short cnt,unsigned short window,unsigned short wid,unsigned short hei);
extern void writeImg(void *p,char *str,unsigned int len);
extern void writePosi(int frm);
extern FILE *fWriteHex;
extern void save(unsigned short frameCnt, unsigned short *starCnt, short moveCnt);
#endif
#ifdef __cplusplus
}
#endif

#endif /* YJ6701_H_ */

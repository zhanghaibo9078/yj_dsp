#include <stdio.h>
#include <math.h>
#include "../include/yj6701.h"

//初始化配准要用的缓冲区
//stars[5][10]：结构体，存储待配准星
//tris[5][10]：结构体，存储待配三角形
//triCnt[5]：三角形的个数
void initPara(struct star **stars,struct tri **tris,unsigned char *triCnt)
{
	int i=0,j=0;
	//初始化星点数组
	for (i = 0; i < 5; i++)
	{
		for(j=0;j<REG_STAR_CNT;j++)
		{
			stars[i][j].r = 0;
			stars[i][j].c = 0;
			stars[i][j].b = 0;
			stars[i][j].label = -1;
		}
	}
	//初始化三角形数组
	for (i = 0; i < 5; i++)
	{
		for(j=0;j<REG_TRI_CNT;j++)
		{
			tris[i][j].s[0] = 0;
			tris[i][j].s[1] = 0;
			tris[i][j].s[2] = 0;
			tris[i][j].labels[0] = 0;
			tris[i][j].labels[1] = 0;
			tris[i][j].labels[2] = 0;
		}
		triCnt[i] = 0;
	}
}

//二值化图像
//img 	图像
//map	二值化图像
//th 	阈值 Threshold
void Threshold(unsigned short *img,unsigned char *map,unsigned short th)
{
	int i=0,j=0;
	unsigned short bufImg[IMG_WID];
	unsigned char bufMap[IMG_WID];
	memset(map,0,IMG_FRM);

	for(i=124;i<IMG_HEI-124;i++)
	{
		memcpy(bufImg,img+i*IMG_WID,IMG_WID*2);
		memset(bufMap,0,IMG_WID);
		for(j=124;j<IMG_WID-124;j++)
		{
			if(bufImg[j]>th)
			{
				bufMap[j] = 1;
			}
		}
		memcpy(map+i*IMG_WID,bufMap,IMG_WID);
	}
}

//连通域标记
//参数map：二值化图
//参数img：原图
//参数stars：星表
unsigned char regRegion(unsigned char *map, unsigned short *img,struct star *stars)
{
	unsigned short i=0, j=0, a=0, b=0;						//用于循环
	unsigned short queue[4096],queueCount=0,pointQueue=0;	//queue的指针
	unsigned int r=0,c=0,bright=0;							//坐标及亮度

	for (i = 100; i<IMG_HEI - 100; i++){						//扫描行
		for (j = 100; j<IMG_WID - 100; j++){					//扫描列
			if(map[i*IMG_WID + j]==1)
			{
				map[i*IMG_WID + j] = 0;						//修改连通域号
				pointQueue = 0;								//queue的指针
				memset(queue, 0, 8192);						//queue初始化为0
				queue[pointQueue++] = j;					//第1个像素的列
				queue[pointQueue++] = i;					//第1个像素的行

				for (queueCount = 0; queueCount<pointQueue / 2; queueCount++){							//遍历queue中的第queueCount个
					for (a = queue[queueCount * 2 + 1] - 1; a <= queue[queueCount * 2 + 1] + 1; a++){	//遍历行
						for (b = queue[queueCount * 2] - 1; b <= queue[queueCount * 2] + 1; b++){		//遍历列
							if (map[a*IMG_WID + b] == 1)			//判断是否为1
							{
								map[a*IMG_WID + b] = 0;				//标记
								queue[pointQueue++] = b;			//下一个像素的x
								queue[pointQueue++] = a;			//下一个像素的y
								if (pointQueue == 4096)				//超出则说明有异常
								{
									return 0;
								}
							}
						}
					}
				}
				if (pointQueue>32)
				{
					//要求大于16个像素点的区域有效
					r = 0;	c = 0;	bright = 0;
					//计算平均亮度并排序
					for (queueCount = 0; queueCount < pointQueue / 2; queueCount++){
						a = queue[queueCount * 2 + 1];
						b = queue[queueCount * 2];
						bright += img[a*IMG_WID + b];
						r += a*img[a*IMG_WID + b];
						c += b*img[a*IMG_WID + b];
					}
					r = r / bright;
					c = c / bright;
					bright = bright / queueCount;
					for (a=0; a<REG_STAR_CNT; a++){
						if(stars[a].b<bright){
							memcpy(stars+a+1,stars+a,(REG_STAR_CNT-1-a)*8);
							break;
						}
					}
					if(a<REG_STAR_CNT){
						stars[a].r		= r;
						stars[a].c		= c;
						stars[a].b		= bright;
						stars[a].label	= 1;
					}
				}
			}
		}
	}

	for (a=0; a<REG_STAR_CNT; a++)
	{
		if(stars[a].label!=-1)
		{
			stars[a].label = a;
		}
		else
		{
			return (unsigned char)a;
		}
	}
	return REG_STAR_CNT;
}

//提取星信息 Detect star
//img 	图像（2MB）
//map	二值化图像（1MB）
//stars	指向星信息的指针	Pointer to stars info
unsigned char getStar(unsigned short *img,unsigned short th,unsigned char *map,struct star *stars)
{
	Threshold(img, map, th);
	return regRegion(map,img,stars);
}

//建立三角形 getTri
//参数stars：星
//参数starCnt：星个数
//参数tris：三角形
void getTri(struct star * stars,unsigned char starCnt,struct tri *tris,unsigned char *triCnt)
{
	unsigned char i=0, j=0, k=0, pt=0;
	if(starCnt<3){
		return;
	}
	for(i=0;i<starCnt-2;i++){
		for (j = i+1; j<starCnt; j++){
			for (k = j + 1; k < starCnt; k++)
			{
				tris[pt].s[0] = abs(stars[i].r - stars[j].r) + abs(stars[i].c - stars[j].c);
				tris[pt].s[1] = abs(stars[j].r - stars[k].r) + abs(stars[j].c - stars[k].c);
				tris[pt].s[2] = abs(stars[k].r - stars[i].r) + abs(stars[k].c - stars[i].c);
				tris[pt].labels[0] = stars[i].label;
				tris[pt].labels[1] = stars[j].label;
				tris[pt++].labels[2] = stars[k].label;
			}
		}
	}
	*triCnt = pt;
}

//配准 Registration
//参数ref：基准图像帧号
//参数est：待配准图像帧号
//参数stars：星
//参数triCnt：三角形数
//参数tris：三角形
//参数r：行偏移量
//参数c：列偏移量
void triReg(int ref,int est,struct star **stars,unsigned char *triCnt,struct tri **tris,short *r,short *c)
{
	unsigned char i=0, j=0,map[REG_STAR_CNT*REG_STAR_CNT];
	short max=0,cnt=0;
	int x=0,y=0;
	memset(map, 0, REG_STAR_CNT * REG_STAR_CNT);
	for(i=0;i<triCnt[ref];i++){
		for (j = 0; j < triCnt[est]; j++){
			if (abs(tris[ref][i].s[0] - tris[est][j].s[0]) + abs(tris[ref][i].s[1] - tris[est][j].s[1]) + abs(tris[ref][i].s[2] - tris[est][j].s[2])<9){
				map[tris[ref][i].labels[0]*REG_STAR_CNT+tris[est][j].labels[0]]++;
				map[tris[ref][i].labels[1]*REG_STAR_CNT+tris[est][j].labels[1]]++;
				map[tris[ref][i].labels[2]*REG_STAR_CNT+tris[est][j].labels[2]]++;
			}
		}
	}
	max = 0;
	for (i = 0; i<REG_STAR_CNT; i++){
		for (j = 0; j < REG_STAR_CNT; j++){
			if (map[i*REG_STAR_CNT+j] > max){
				max = map[i*REG_STAR_CNT+j];
			}
		}
	}
	if(max==0){
		return;
	}
	for (i = 0; i<REG_STAR_CNT; i++){
		for (j = 0; j < REG_STAR_CNT; j++){
			if (map[i*REG_STAR_CNT+j] == max){
				cnt++;
				x -= stars[ref][i].r - stars[est][j].r;
				y -= stars[ref][i].c - stars[est][j].c;
			}
		}
	}
	*r = x/cnt;
	*c = y/cnt;
	return;
}

//图像配准的函数入口
//参数curID：当前帧号（0.1.2.3.4）
//参数img：使用的图像
//参数winSize：配准窗口大小
//参数offsetR：行偏移量
//参数offsetC：列偏移量
void matchMain(unsigned int curID,unsigned short **img)
{
	unsigned char starCnt = 0;
	if(curID==0)
	{
		initPara(g_stars,g_tris,g_triCnt);
	}
	starCnt = getStar(img[curID],g_thresholdBW,g_regLabelMap,g_stars[curID]);
	getTri(g_stars[curID],starCnt,g_tris[curID],g_triCnt+curID);

	if(curID>0)
	{
		triReg(curID-1, curID, g_stars, g_triCnt,g_tris, g_offsetR+curID,g_offsetC+curID);
		g_offsetR[curID] += g_offsetR[curID-1];
		g_offsetC[curID] += g_offsetC[curID-1];
	}
}

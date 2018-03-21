#include <stdio.h>
#include <math.h>
#include "../include/yj6701.h"

/********************************获取最大值帧索引************************************/
void getMaxIndex(void)	//1560ms
{
	int				j = 0, row = 0, col = 0, max = 0, index = 0;
	float			mean = 0, realVal = 0;

	memset(g_maxIndex, 0, IMG_FRM);

	for (row = 1; row<IMG_HEI; row++)
	{
		//取滤除后图像值
		for (j = 0; j<5; j++)
		{
			memset(s_origVal[j], 0, 2048);
			//只有行有效
			if (((g_offsetR[j] <= 0) && (row + g_offsetR[j] >= 0)) || ((g_offsetR[j]>0) && (row + g_offsetR[j]<IMG_WID)))
			{
				if (g_offsetC[j] >= 0)
				{
					memcpy(s_origVal[j], g_imgRmb[j] + (row + g_offsetR[j])*IMG_WID + g_offsetC[j], 2048 - g_offsetC[j] * 2);
				}
				else
				{
					memcpy(s_origVal[j] - g_offsetC[j], g_imgRmb[j] + (row + g_offsetR[j])*IMG_WID, 2048 + g_offsetC[j] * 2);
				}
			}
		}

		memset(s_maxIdx, 0, 1024);
		for (col = 0; col<IMG_WID; col++)
		{
			//求最大值和最大值帧索引（1-5）
			max = 0;
			index = 0;
			mean = 0;
			for (j = 0; j<5; j++)
			{
				mean += s_origVal[j][col];
				if (s_origVal[j][col] > max)
				{
					max = s_origVal[j][col];
					index = j + 1;
				}
			}
			//背景均值
			mean = (mean - max) / 4.0;
			//真实值=最大值-均值
			realVal = max - mean;
			//除数不能为0
			if (mean>0)
			{
				if ((realVal > 3.8 * mean) && (realVal > 2.0 * g_thresholdBCK))
				{
					s_maxIdx[col] = index;
				}
			}
			else
			{
				//最大值-均值大于2.0*背景噪声，最大值帧索引才有效
				if (realVal > 2.0 * g_thresholdBCK)
				{
					s_maxIdx[col] = index;
				}
			}
		}
		//最大值帧索引应保持原形，因为求质心过程要用到
		memcpy(g_maxIndex + row*IMG_WID, s_maxIdx, 1024);
	}
	return;
}

/*************************去除最大值索引帧不合格点子程序*****************************/
void clear(void)	//610ms
{
	unsigned short	i = 0, j = 0;
	unsigned char	val = 0, sum = 0, val_r = 0;
	unsigned short	sum_r = 0;

	if (g_thresholdMW > 8)		{g_thresholdMW = 8;}
	if (g_thresholdMW < 1)		{g_thresholdMW = 1;}
	//初始化去噪后缓存
	memset(g_corImg, 0, IMG_FRM);

	for (i = 1; i<IMG_HEI - 1; i++)
	{
		for (j = 1; j<IMG_WID - 1; j++)
		{
			//取帧索引
			val = g_maxIndex[i*IMG_WID + j];
			if (val == 0) { sum_r = 0; }
			else
			{
				//目标超过25*25认为图像不正常
				if (val != val_r) { val_r = val;	sum_r = 0; }
				else { sum_r++;	if (sum_r > 25) { break; } }

				//求8邻域计数
				sum = 0;
				if (g_maxIndex[(i - 1)*IMG_WID + j - 1] == val)		{sum++;}
				if (g_maxIndex[(i - 1)*IMG_WID + j] == val)			{sum++;}
				if (g_maxIndex[(i - 1)*IMG_WID + j + 1] == val)		{sum++;}
				if (g_maxIndex[i	  *IMG_WID + j - 1] == val)		{sum++;}
				if (g_maxIndex[i	  *IMG_WID + j + 1] == val)		{sum++;}
				if (g_maxIndex[(i + 1)*IMG_WID + j - 1] == val)		{sum++;}
				if (g_maxIndex[(i + 1)*IMG_WID + j] == val)			{sum++;}
				if (g_maxIndex[(i + 1)*IMG_WID + j + 1] == val)		{sum++;}
				if (sum >= g_thresholdMW) { g_corImg[i*IMG_WID + j] = val; }
			}
		}
	}
}

/********************************连通域**********************************************/
unsigned short region(void)
{
	unsigned short regionCount = 0;					//连通域计数
	unsigned short minX,maxX,minY,maxY;				//连通域的边界坐标
	unsigned short i=0,j=0,a=0,b=0,queueCount=0;
	unsigned char  m;
	unsigned short flag = 6;						//连通域号(6,7,8,9...)
	unsigned short pointQueue = 0;					//queue的指针
	unsigned short queue[2048];
	
	//清空连通域缓存
	memset(g_objNum,0,10);
	memset(g_objPntLst,0,4*4096);

	//求取连通域
	for(i=1; i<IMG_HEI-1; i++)						//扫描行
	{
		for(j=1; j<IMG_WID-1; j++)					//扫描列
		{
			m = g_corImg[i*IMG_WID+j];			

			if((m<=5)&&(m>0))						//如果为1,2,3,4,5
			{
				g_corImg[i*IMG_WID+j] = flag;		//修改连通域号
				minX = maxX = j;					//将该点坐标暂时作为x边界值
				minY = maxY = i;					//将该点坐标暂时作为y边界值

				pointQueue = 0;						//queue的指针
				memset(queue,0,4096);				//queue初始化为0
				queue[pointQueue++] = j;			//第1个像素的x
				queue[pointQueue++] = i;			//第1个像素的y

				for(queueCount=0;queueCount<pointQueue/2;queueCount++)				//遍历queue中的第queueCount个
				{
					for(a=queue[queueCount*2+1]-1;a<=queue[queueCount*2+1]+1;a++)	//遍历行
					{
						for(b=queue[queueCount*2]-1;b<=queue[queueCount*2]+1;b++)	//遍历列
						{
							if(g_corImg[a*IMG_WID+b] == m)			//判断是否为m
							{
								g_corImg[a*IMG_WID+b] = flag;		//标记
								queue[pointQueue++] = b;			//下一个像素的x
								queue[pointQueue++] = a;			//下一个像素的y
								if (b < minX)						//如果小于minX
								{
									minX = b;						//修改minX
								}
								else if (b > maxX)					//如果大于maxX
								{
									maxX = b;						//修改maxX
								}
								else { b = b; }
								
								if (a < minY)						//如果小于minY
								{
									minY = a;						//修改minY
								}
								else if (a > maxY)					//如果大于maxY
								{
									maxY = a;						//修改maxY
								}
								else { a = a; }
								if(pointQueue==2048)
								{
									return regionCount;
								}
							}
						}
					}
				}
				g_objPntLst[regionCount*4] = m;						//帧号(1.2.3.4.5)
				g_objPntLst[regionCount*4+1] = flag;				//连通域标号(6,7,8,9...)
				g_objPntLst[regionCount*4+2] = (float)(minY+maxY)/2;//行
				g_objPntLst[regionCount*4+3] = (float)(minX+maxX)/2;//列
				g_objNum[m-1]++;									//每帧连通域计数+1
				regionCount++;										//连通域计数+1
				if(regionCount >= 768)
				{
					return regionCount;
				}
				flag++;	if (flag == 255) { flag = 6; }				//控制范围
			}
		}
	}

	return regionCount;
}

/********************************运动轨迹********************************************/
/*******************************参数说明********************************************
	objNum：记录每帧图像的连通域数						——输入
	objPointList：存储帧号、连通域、及粗略坐标			——输入
	allObjInf：每最大连通域数*5行3列,要输出的矩阵
			   (每一行是一帧信息,0.1列为坐标,2列为帧号)	——输出
				①1.2有	②0.1有	③3.4有	④0.2有
				0.	○	○	○	○	○----①
				1.	×	○	○	○	○----①
				2.	○	×	○	○	○----③
				3.	○	○	×	○	○----②
				4.	○	○	○	×	○----①
				5.	○	○	○	○	×----①
				6.	×	×	○	○	○----③
				7.	○	×	×	○	○----③
				8.	○	○	×	×	○----②
				9.	○	○	○	×	×----②
				10.	×	○	×	○	○----③
				11.	×	○	○	×	○----①
				12.	×	○	○	○	×----①
				13.	○	×	○	×	○----④
				14.	○	×	○	○	×----④
				15.	○	○	×	○	×----②
************************************************************************************/
unsigned short movepath(unsigned short regionCount)
{
	float			err=3.3;						//阈值
	unsigned short	Vr = 30;						//设置运动速度的门限
	unsigned short	curObjNum = 0,preObjNum = 0;	//allObjInf的目标数
	unsigned short	i=0,j=0,k=0,obj=0;				//用于循环
	float			a=0,b=0,c=0,d=0,e=0,f=0;		//第0,1,2,3,4帧图连通域的坐标信息;abcdef存放3个点的行列值;ab为中间点;cd为前面点;ef为后面点
	unsigned short	p[5]={0,0,0,0,0};					//存放数据时寻址
	float curDisErr = 0,curDP = 0,curDPErr =0,minDisErr=0,minDPErr=0;	//判断是否满足条件
	float dis01=0,dis02=0,dis03=0,dis12=0,dis13=0,dis14=0,dis23=0,dis24=0,dis34=0;
	float r01[2]={0,0},r02[2]={0,0},r03[2]={0,0},r12[2]={0,0},r13[2]={0,0},r14[2]={0,0},r23[2]={0,0},r24[2]={0,0},r34[2]={0,0};
	unsigned char is_continue = 1;							//是否继续执行下一个for循环

	memset(g_allObjInf,0,4096*2);
	#ifdef TI_PLATFORM
	memset(objPointF[0],0,8192*5);
	#endif
	
	for(i=0;i<regionCount;i++)
	{
		j = (int)g_objPntLst[i*4]-1;
		objPointF[j][p[j]*2] = g_objPntLst[i*4+2];
		objPointF[j][p[j]*2+1] = g_objPntLst[i*4+3];
		p[j]++;
	}

	//第1种:1.2有-----------------------------------------------------------------------------------
	for(i=0;i<g_objNum[2];i++){									//遍历第2帧的连通域坐标
		for(j=0;j<g_objNum[1];j++)								//遍历第1帧的连通域坐标
		{
			//-------------------------------------12-------------------------------------------------
			a = objPointF[2][i*2];
			b = objPointF[2][i*2+1];
			c = objPointF[1][j*2];
			d = objPointF[1][j*2+1];
			if((fabs(a+1)>0.0000001) && (fabs(b+1)>0.0000001) && (fabs(c+1)>0.0000001) && (fabs(d+1)>0.0000001)){
			dis12 = sqrt((a-c)*(a-c) + (b-d)*(b-d));			//2->1距离
			r12[0] = a-c;										//2->1向量
			r12[1] = b-d;										//2->1向量
			if(dis12<=Vr)
			{
				//-------------------------------------23-------------------------------------------------
				minDisErr = Vr;	minDPErr = Vr;	is_continue = 1;
				for(k=0;k<g_objNum[3];k++)
				{
					e = objPointF[3][k*2];
					f = objPointF[3][k*2+1];
					if((fabs(e+1)>0.0000001) && (fabs(f+1)>0.0000001)){
					dis23 = sqrt((e-a)*(e-a) + (f-b)*(f-b));	//3->2距离
					r23[0] = e-a;								//3->2向量
					r23[1] = f-b;								//3->2向量

					curDP = (r12[0]*r23[0]+r12[1]*r23[1])/(dis12*dis23);	//cosθ
					curDPErr = 1-curDP;										//1-cosθ
					curDisErr = abs(dis12-dis23);							//距离偏差绝对值
				
					if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
					{
						minDisErr = curDisErr;	minDPErr = curDPErr;
						for(obj=curObjNum*5;obj<(curObjNum+1)*5;obj++){	//g_allObjInf((curObjNum-1)*5+1:curObjNum*5,3) = curObjNum;
							g_allObjInf[obj*3+2] = curObjNum+1;}
						g_allObjInf[(curObjNum*5+1)*3]		= objPointF[1][j*2];
						g_allObjInf[(curObjNum*5+1)*3+1]	= objPointF[1][j*2+1];
						g_allObjInf[(curObjNum*5+2)*3]		= objPointF[2][i*2];
						g_allObjInf[(curObjNum*5+2)*3+1]	= objPointF[2][i*2+1];
						g_allObjInf[(curObjNum*5+3)*3]		= objPointF[3][k*2];
						g_allObjInf[(curObjNum*5+3)*3+1]	= objPointF[3][k*2+1];
						objPointF[1][j*2]	= -1;
						objPointF[1][j*2+1]	= -1;
						objPointF[2][i*2]	= -1;
						objPointF[2][i*2+1]	= -1;
						objPointF[3][k*2]	= -1;
						objPointF[3][k*2+1]	= -1;
						curObjNum++;
						if(curObjNum>39)	return curObjNum;
						is_continue			= 0;
					}}
				}
				//-------------------------------------24-------------------------------------------------
				if(is_continue != 0){			//不继续就跳过
				minDisErr = Vr;	minDPErr = Vr;
				for(k=0;k<g_objNum[4];k++)
				{
					e = objPointF[4][k*2];
					f = objPointF[4][k*2+1];
					if((fabs(e+1)>0.0000001) && (fabs(f+1)>0.0000001)){
					dis24 = sqrt((e-a)*(e-a) + (f-b)*(f-b));	//4->2距离
					r24[0] = e-a;								//4->2向量
					r24[1] = f-b;								//4->2向量

					curDP = (r12[0]*r24[0]+r12[1]*r24[1])/(dis12*dis24);	//cosθ
					curDPErr = 1-curDP;										//1-cosθ
					curDisErr = abs(dis12-dis24/2);							//距离偏差绝对值
				
					if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
					{
						minDisErr = curDisErr;	minDPErr = curDPErr;
						for(obj=curObjNum*5;obj<(curObjNum+1)*5;obj++){	//g_allObjInf((curObjNum-1)*5+1:curObjNum*5,3) = curObjNum;
							g_allObjInf[obj*3+2] = curObjNum+1;}
						g_allObjInf[(curObjNum*5+1)*3]		= objPointF[1][j*2];
						g_allObjInf[(curObjNum*5+1)*3+1]	= objPointF[1][j*2+1];
						g_allObjInf[(curObjNum*5+2)*3]		= objPointF[2][i*2];
						g_allObjInf[(curObjNum*5+2)*3+1]	= objPointF[2][i*2+1];
						g_allObjInf[(curObjNum*5+4)*3]		= objPointF[4][k*2];
						g_allObjInf[(curObjNum*5+4)*3+1]	= objPointF[4][k*2+1];
						objPointF[1][j*2]	= -1;
						objPointF[1][j*2+1]	= -1;
						objPointF[2][i*2]	= -1;
						objPointF[2][i*2+1]	= -1;
						objPointF[4][k*2]	= -1;
						objPointF[4][k*2+1]	= -1;
						curObjNum++;
						if(curObjNum>39)	return curObjNum;
					}}
				}}
			}
		}}}
	
	for(i=0;i<curObjNum;i++)
	{
		//---------------------------------01-12-----------------------------------------
		minDisErr = Vr;	minDPErr = Vr;
		a = g_allObjInf[(i*5+1)*3];		//1坐标
		b = g_allObjInf[(i*5+1)*3+1];
		e = g_allObjInf[(i*5+2)*3];		//2坐标
		f = g_allObjInf[(i*5+2)*3+1];
		for(j=0;j<g_objNum[0];j++)								//遍历第0帧的连通域坐标
		{
			c		= objPointF[0][j*2];						//0坐标
			d		= objPointF[0][j*2+1];
			if((fabs(c+1)>0.0000001) && (fabs(d+1)>0.0000001)){
			dis01	= sqrt((a-c)*(a-c) + (b-d)*(b-d));			//1->0距离
			dis12	= sqrt((e-a)*(e-a) + (f-b)*(f-b));			//2->1距离
			r01[0]	= a-c;										//1->0向量
			r01[1]	= b-d;										//1->0向量
			r12[0]	= e-a;										//2->1向量
			r12[1]	= f-b;										//2->1向量
			
			curDP = (r01[0]*r12[0]+r01[1]*r12[1])/(dis01*dis12);//cosθ
			curDPErr = 1-curDP;									//1-cosθ
			curDisErr = abs(dis01-dis12);						//距离偏差绝对值
			
			if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
			{
				minDPErr=curDPErr;
				minDisErr=curDisErr;
				g_allObjInf[(i*5)*3]	= objPointF[0][j*2];
				g_allObjInf[(i*5)*3+1]	= objPointF[0][j*2+1];
				objPointF[0][j*2]		= -1;
				objPointF[0][j*2+1]		= -1;
			}}
		}
		
		//--------------------------------23-34------------------------------------------
		minDisErr = Vr;	minDPErr = Vr;
		a = g_allObjInf[(i*5+3)*3];		//3坐标
		b = g_allObjInf[(i*5+3)*3+1];
		c = g_allObjInf[(i*5+2)*3];		//2坐标
		d = g_allObjInf[(i*5+2)*3+1];
		if((fabs(a) > 0.0000001) && (fabs(b) > 0.0000001)){					//没有3说明有4就跳过
		for(j=0;j<g_objNum[4];j++)				//遍历第4帧的连通域坐标
		{
			e		= objPointF[4][j*2];					//4坐标
			f		= objPointF[4][j*2+1];
			if((fabs(e+1)>0.0000001) && (fabs(f+1)>0.0000001)){
			dis23	= sqrt((a-c)*(a-c) + (b-d)*(b-d));			//3->2距离
			dis34	= sqrt((e-a)*(e-a) + (f-b)*(f-b));			//4->3距离
			r23[0]	= a-c;										//3->2向量
			r23[1]	= b-d;										//3->2向量
			r34[0]	= e-a;										//4->3向量
			r34[1]	= f-b;										//4->3向量

			curDP = (r34[0]*r23[0]+r34[1]*r23[1])/(dis34*dis23);//cosθ
			curDPErr = 1-curDP;									//1-cosθ
			curDisErr = abs(dis34-dis23);						//距离偏差绝对值

			if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
			{
				minDPErr=curDPErr;
				minDisErr=curDisErr;
				g_allObjInf[(i*5+4)*3]		= objPointF[4][j*2];
				g_allObjInf[(i*5+4)*3+1]	= objPointF[4][j*2+1];
				objPointF[4][j*2]			= -1;
				objPointF[4][j*2+1]			= -1;
			}}
		}}
	}
	preObjNum = curObjNum;
	
	//第2种:0.1有-----------------------------------------------------------------------------------
	for(i=0;i<g_objNum[1];i++){									//遍历第1帧的连通域坐标
		for(j=0;j<g_objNum[0];j++)								//遍历第0帧的连通域坐标
		{
			//-------------------------------------01-------------------------------------------------
			a = objPointF[1][i*2];
			b = objPointF[1][i*2+1];
			c = objPointF[0][j*2];
			d = objPointF[0][j*2+1];
			if((fabs(a+1)>0.0000001) && (fabs(b+1)>0.0000001) && (fabs(c+1)>0.0000001) && (fabs(d+1)>0.0000001)){
			dis01 = sqrt((a-c)*(a-c) + (b-d)*(b-d));			//1->0距离
			r01[0] = a-c;										//1->0向量
			r01[1] = b-d;										//1->0向量
			if(dis01<=Vr)
			{
				//-------------------------------------12-------------------------------------------------
				minDisErr = Vr;	minDPErr = Vr;	is_continue = 1;
				for(k=0;k<g_objNum[2];k++)
				{
					e = objPointF[2][k*2];
					f = objPointF[2][k*2+1];
					if((fabs(e+1)>0.0000001) && (fabs(f+1)>0.0000001)){
					dis12 = sqrt((e-a)*(e-a) + (f-b)*(f-b));	//2->1距离
					r12[0] = e-a;								//2->1向量
					r12[1] = f-b;								//2->1向量

					curDP = (r01[0]*r12[0]+r01[1]*r12[1])/(dis01*dis12);	//cosθ
					curDPErr = 1-curDP;										//1-cosθ
					curDisErr = abs(dis01-dis12);							//距离偏差绝对值
				
					if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
					{
						minDisErr = curDisErr;	minDPErr = curDPErr;
						for(obj=curObjNum*5;obj<(curObjNum+1)*5;obj++){	//g_allObjInf((curObjNum-1)*5+1:curObjNum*5,3) = curObjNum;
							g_allObjInf[obj*3+2] = curObjNum+1;}
						g_allObjInf[(curObjNum*5+0)*3]		= objPointF[0][j*2];
						g_allObjInf[(curObjNum*5+0)*3+1]	= objPointF[0][j*2+1];
						g_allObjInf[(curObjNum*5+1)*3]		= objPointF[1][i*2];
						g_allObjInf[(curObjNum*5+1)*3+1]	= objPointF[1][i*2+1];
						g_allObjInf[(curObjNum*5+2)*3]		= objPointF[2][k*2];
						g_allObjInf[(curObjNum*5+2)*3+1]	= objPointF[2][k*2+1];
						objPointF[0][j*2]	= -1;
						objPointF[0][j*2+1]	= -1;
						objPointF[1][i*2]	= -1;
						objPointF[1][i*2+1]	= -1;
						objPointF[2][k*2]	= -1;
						objPointF[2][k*2+1]	= -1;
						curObjNum++;
						if(curObjNum>39)	return curObjNum;
						is_continue			= 0;
					}}
				}
				//-------------------------------------13-------------------------------------------------(没有2)
				if(is_continue != 0){			//不继续就跳过
				minDisErr = Vr;	minDPErr = Vr;
				for(k=0;k<g_objNum[3];k++)
				{
					e = objPointF[3][k*2];
					f = objPointF[3][k*2+1];
					if((fabs(e+1)>0.0000001) && (fabs(f+1)>0.0000001)){
					dis13 = sqrt((e-a)*(e-a) + (f-b)*(f-b));	//3->1距离
					r13[0] = e-a;								//3->1向量
					r13[1] = f-b;								//3->1向量

					curDP = (r01[0]*r13[0]+r01[1]*r13[1])/(dis01*dis13);	//cosθ
					curDPErr = 1-curDP;										//1-cosθ
					curDisErr = abs(dis01-dis13/2);							//距离偏差绝对值
				
					if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
					{
						minDisErr = curDisErr;	minDPErr = curDPErr;
						for(obj=curObjNum*5;obj<(curObjNum+1)*5;obj++){	//g_allObjInf((curObjNum-1)*5+1:curObjNum*5,3) = curObjNum;
							g_allObjInf[obj*3+2] = curObjNum+1;}
						g_allObjInf[(curObjNum*5+0)*3]		= objPointF[0][j*2];
						g_allObjInf[(curObjNum*5+0)*3+1]	= objPointF[0][j*2+1];
						g_allObjInf[(curObjNum*5+1)*3]		= objPointF[1][i*2];
						g_allObjInf[(curObjNum*5+1)*3+1]	= objPointF[1][i*2+1];
						g_allObjInf[(curObjNum*5+3)*3]		= objPointF[3][k*2];
						g_allObjInf[(curObjNum*5+3)*3+1]	= objPointF[3][k*2+1];
						objPointF[0][j*2]	= -1;
						objPointF[0][j*2+1]	= -1;
						objPointF[1][i*2]	= -1;
						objPointF[1][i*2+1]	= -1;
						objPointF[3][k*2]	= -1;
						objPointF[3][k*2+1]	= -1;
						curObjNum++;
						if(curObjNum>39)	return curObjNum;
						is_continue			= 0;
					}}
				}
				//-------------------------------------14-------------------------------------------------(没有2.3)
				if(is_continue != 0){	//不继续就跳过
				minDisErr = Vr;	minDPErr = Vr;
				for(k=0;k<g_objNum[4];k++)
				{
					e = objPointF[4][k*2];
					f = objPointF[4][k*2+1];
					if((fabs(e+1)<0.0000001) || (fabs(f+1)<0.0000001))	{;}	else{
					dis14 = sqrt((e-a)*(e-a) + (f-b)*(f-b));	//3->1距离
					r14[0] = e-a;								//3->1向量
					r14[1] = f-b;								//3->1向量

					curDP = (r01[0]*r14[0]+r01[1]*r14[1])/(dis01*dis14);	//cosθ
					curDPErr = 1-curDP;										//1-cosθ
					curDisErr = abs(dis01-dis14/3);							//距离偏差绝对值
				
					if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
					{
						minDisErr = curDisErr;	minDPErr = curDPErr;
						for(obj=curObjNum*5;obj<(curObjNum+1)*5;obj++){	//g_allObjInf((curObjNum-1)*5+1:curObjNum*5,3) = curObjNum;
							g_allObjInf[obj*3+2] = curObjNum+1;}
						g_allObjInf[(curObjNum*5+0)*3]		= objPointF[0][j*2];
						g_allObjInf[(curObjNum*5+0)*3+1]	= objPointF[0][j*2+1];
						g_allObjInf[(curObjNum*5+1)*3]		= objPointF[1][i*2];
						g_allObjInf[(curObjNum*5+1)*3+1]	= objPointF[1][i*2+1];
						g_allObjInf[(curObjNum*5+4)*3]		= objPointF[4][k*2];
						g_allObjInf[(curObjNum*5+4)*3+1]	= objPointF[4][k*2+1];
						objPointF[0][j*2]	= -1;
						objPointF[0][j*2+1]	= -1;
						objPointF[1][i*2]	= -1;
						objPointF[1][i*2+1]	= -1;
						objPointF[4][k*2]	= -1;
						objPointF[4][k*2+1]	= -1;
						curObjNum++;
						if(curObjNum>39)	return curObjNum;
						is_continue			= 0;
					}}
				}}}
			}}
		}}
	
	for(i=preObjNum;i<curObjNum;i++)
	{
		minDisErr = Vr;	minDPErr = Vr;
		a = g_allObjInf[(i*5+2)*3];				//2坐标
		b = g_allObjInf[(i*5+2)*3+1];
		if((fabs(a)<0.0000001) || (fabs(b)<0.0000001))				//没有2
		{
			a = g_allObjInf[(i*5+3)*3];			//3坐标
			b = g_allObjInf[(i*5+3)*3+1];
			if((fabs(a)>0.0000001) && (fabs(b)>0.0000001)){	//没有3
			//---------------------------------13-34-----------------------------------------
			c = g_allObjInf[(i*5+1)*3];			//1坐标
			d = g_allObjInf[(i*5+1)*3+1];
			for(j=0;j<g_objNum[4];j++)								//遍历第4帧的连通域坐标
			{
				e		= objPointF[4][j*2];						//4坐标
				f		= objPointF[4][j*2+1];
				if((fabs(e+1)>0.0000001) && (fabs(f+1)>0.0000001)){
				dis13	= sqrt((a-c)*(a-c) + (b-d)*(b-d));			//3->1距离
				dis34	= sqrt((e-a)*(e-a) + (f-b)*(f-b));			//4->3距离
				r13[0]	= a-c;										//3->1向量
				r13[1]	= b-d;										//3->1向量
				r34[0]	= e-a;										//4->3向量
				r34[1]	= f-b;										//4->3向量
			
				curDP = (r13[0]*r34[0]+r13[1]*r34[1])/(dis13*dis34);//cosθ
				curDPErr = 1-curDP;									//1-cosθ
				curDisErr = abs(dis13/2-dis34);						//距离偏差绝对值
			
				if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
				{
					minDPErr=curDPErr;
					minDisErr=curDisErr;
					g_allObjInf[(i*5+4)*3]		= objPointF[4][j*2];
					g_allObjInf[(i*5+4)*3+1]	= objPointF[4][j*2+1];
					objPointF[4][j*2]			= -1;
					objPointF[4][j*2+1]			= -1;
				}}
			}}
		}
	}
	preObjNum = curObjNum;
	
	//第3种:3.4有-----------------------------------------------------------------------------------
	for(i=0;i<g_objNum[3];i++){									//遍历第3帧的连通域坐标
		for(j=0;j<g_objNum[4];j++)								//遍历第4帧的连通域坐标
		{
			//-------------------------------------34-------------------------------------------------
			a = objPointF[3][i*2];
			b = objPointF[3][i*2+1];
			e = objPointF[4][j*2];
			f = objPointF[4][j*2+1];
			if((fabs(a+1)>0.0000001) && (fabs(b+1)>0.0000001) && (fabs(e+1)>0.0000001) && (fabs(f+1)>0.0000001)){
			dis34 = sqrt((e-a)*(e-a) + (f-b)*(f-b));			//4->3距离
			r34[0] = e-a;										//4->3向量
			r34[1] = f-b;										//4->3向量
			if(dis34<=Vr)
			{
				//-------------------------------------23-------------------------------------------------
				minDisErr = Vr;	minDPErr = Vr;	is_continue = 1;
				for(k=0;k<g_objNum[2];k++)
				{
					c = objPointF[2][k*2];
					d = objPointF[2][k*2+1];
					if((fabs(c+1)>0.0000001) && (fabs(d+1)>0.0000001)){
					dis23 = sqrt((a-c)*(a-c) + (b-d)*(b-d));	//3->2距离
					r23[0] = a-c;								//3->2向量
					r23[1] = b-d;								//3->2向量

					curDP = (r23[0]*r34[0]+r23[1]*r34[1])/(dis23*dis34);	//cosθ
					curDPErr = 1-curDP;										//1-cosθ
					curDisErr = abs(dis23-dis34);							//距离偏差绝对值
				
					if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
					{
						minDisErr = curDisErr;	minDPErr = curDPErr;
						for(obj=curObjNum*5;obj<(curObjNum+1)*5;obj++){	//g_allObjInf((curObjNum-1)*5+1:curObjNum*5,3) = curObjNum;
							g_allObjInf[obj*3+2] = curObjNum+1;}
						g_allObjInf[(curObjNum*5+4)*3]		= objPointF[4][j*2];
						g_allObjInf[(curObjNum*5+4)*3+1]	= objPointF[4][j*2+1];
						g_allObjInf[(curObjNum*5+3)*3]		= objPointF[3][i*2];
						g_allObjInf[(curObjNum*5+3)*3+1]	= objPointF[3][i*2+1];
						g_allObjInf[(curObjNum*5+2)*3]		= objPointF[2][k*2];
						g_allObjInf[(curObjNum*5+2)*3+1]	= objPointF[2][k*2+1];
						objPointF[4][j*2]	= -1;
						objPointF[4][j*2+1]	= -1;
						objPointF[3][i*2]	= -1;
						objPointF[3][i*2+1]	= -1;
						objPointF[2][k*2]	= -1;
						objPointF[2][k*2+1]	= -1;
						curObjNum++;
						if(curObjNum>39)	return curObjNum;
						is_continue			= 0;
					}}
				}
				//-------------------------------------13-------------------------------------------------(没有2)
				if(is_continue != 0){	//不继续就跳过
				minDisErr = Vr;	minDPErr = Vr;
				for(k=0;k<g_objNum[1];k++)
				{
					c = objPointF[1][k*2];
					d = objPointF[1][k*2+1];
					if((fabs(c+1)>0.0000001) && (fabs(d+1)>0.0000001)){
					dis13 = sqrt((a-c)*(a-c) + (b-d)*(b-d));	//3->1距离
					r13[0] = a-c;								//3->1向量
					r13[1] = b-d;								//3->1向量

					curDP = (r13[0]*r34[0]+r13[1]*r34[1])/(dis13*dis34);	//cosθ
					curDPErr = 1-curDP;										//1-cosθ
					curDisErr = abs(dis13/2-dis34);							//距离偏差绝对值
				
					if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
					{
						minDisErr = curDisErr;	minDPErr = curDPErr;
						for(obj=curObjNum*5;obj<(curObjNum+1)*5;obj++){	//g_allObjInf((curObjNum-1)*5+1:curObjNum*5,3) = curObjNum;
							g_allObjInf[obj*3+2] = curObjNum+1;}
						g_allObjInf[(curObjNum*5+4)*3]		= objPointF[4][j*2];
						g_allObjInf[(curObjNum*5+4)*3+1]	= objPointF[4][j*2+1];
						g_allObjInf[(curObjNum*5+3)*3]		= objPointF[3][i*2];
						g_allObjInf[(curObjNum*5+3)*3+1]	= objPointF[3][i*2+1];
						g_allObjInf[(curObjNum*5+1)*3]		= objPointF[1][k*2];
						g_allObjInf[(curObjNum*5+1)*3+1]	= objPointF[1][k*2+1];
						objPointF[4][j*2]	= -1;
						objPointF[4][j*2+1]	= -1;
						objPointF[3][i*2]	= -1;
						objPointF[3][i*2+1]	= -1;
						objPointF[1][k*2]	= -1;
						objPointF[1][k*2+1]	= -1;
						curObjNum++;
						if(curObjNum>39)	return curObjNum;
						is_continue			= 0;
					}}
				}
				//-------------------------------------03-------------------------------------------------(没有1.2)
				if(is_continue != 0){	//不继续就跳过
				minDisErr = Vr;	minDPErr = Vr;
				for(k=0;k<g_objNum[0];k++)
				{
					c = objPointF[0][k*2];
					d = objPointF[0][k*2+1];
					if((fabs(c+1)>0.0000001) && (fabs(d+1)>0.0000001)){
					dis03 = sqrt((a-c)*(a-c) + (b-d)*(b-d));	//3->0距离
					r03[0] = a-c;								//3->0向量
					r03[1] = b-d;								//3->0向量

					curDP = (r03[0]*r34[0]+r03[1]*r34[1])/(dis03*dis34);	//cosθ
					curDPErr = 1-curDP;										//1-cosθ
					curDisErr = abs(dis03/3-dis34);							//距离偏差绝对值
				
					if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
					{
						minDisErr = curDisErr;	minDPErr = curDPErr;
						for(obj=curObjNum*5;obj<(curObjNum+1)*5;obj++){	//g_allObjInf((curObjNum-1)*5+1:curObjNum*5,3) = curObjNum;
							g_allObjInf[obj*3+2] = curObjNum+1;}
						g_allObjInf[(curObjNum*5+4)*3]		= objPointF[4][j*2];
						g_allObjInf[(curObjNum*5+4)*3+1]	= objPointF[4][j*2+1];
						g_allObjInf[(curObjNum*5+3)*3]		= objPointF[3][i*2];
						g_allObjInf[(curObjNum*5+3)*3+1]	= objPointF[3][i*2+1];
						g_allObjInf[(curObjNum*5+0)*3]		= objPointF[0][k*2];
						g_allObjInf[(curObjNum*5+0)*3+1]	= objPointF[0][k*2+1];
						objPointF[4][j*2]	= -1;
						objPointF[4][j*2+1]	= -1;
						objPointF[3][i*2]	= -1;
						objPointF[3][i*2+1]	= -1;
						objPointF[0][k*2]	= -1;
						objPointF[0][k*2+1]	= -1;
						curObjNum++;
						if(curObjNum>39)	return curObjNum;
						is_continue			= 0;
					}}
				}}}
			}}
		}}
	
	for(i=preObjNum;i<curObjNum;i++)
	{
		minDisErr = Vr;	minDPErr = Vr;
		a = g_allObjInf[(i*5+2)*3];				//2坐标
		b = g_allObjInf[(i*5+2)*3+1];
		if((fabs(a)>0.0000001) && (fabs(b)>0.0000001)){	//没有2
		//---------------------------------02-23-----------------------------------------
		e = g_allObjInf[(i*5+3)*3];				//3坐标
		f = g_allObjInf[(i*5+3)*3+1];
		for(j=0;j<g_objNum[0];j++)								//遍历第0帧的连通域坐标
		{
			c		= objPointF[0][j*2];						//0坐标
			d		= objPointF[0][j*2+1];
			if((fabs(c+1)>0.0000001) && (fabs(d+1)>0.0000001)){
			dis02	= sqrt((a-c)*(a-c) + (b-d)*(b-d));			//2->0距离
			dis23	= sqrt((e-a)*(e-a) + (f-b)*(f-b));			//3->2距离
			r02[0]	= a-c;										//2->0向量
			r02[1]	= b-d;										//2->0向量
			r23[0]	= e-a;										//3->2向量
			r23[1]	= f-b;										//3->2向量
			
			curDP = (r02[0]*r23[0]+r02[1]*r23[1])/(dis02*dis23);//cosθ
			curDPErr = 1-curDP;									//1-cosθ
			curDisErr = abs(dis02/2-dis23);						//距离偏差绝对值
			
			if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
			{
				minDPErr=curDPErr;
				minDisErr=curDisErr;
				g_allObjInf[(i*5+0)*3]		= objPointF[0][j*2];
				g_allObjInf[(i*5+0)*3+1]	= objPointF[0][j*2+1];
				objPointF[0][j*2]			= -1;
				objPointF[0][j*2+1]			= -1;
			}}
		}}
	}
	
	//第4种:一共3个目标,0.2有-----------------------------------------------------------------------------------
	for(i=0;i<g_objNum[2];i++){									//遍历第2帧的连通域坐标
		for(j=0;j<g_objNum[0];j++)								//遍历第0帧的连通域坐标
		{
			//-------------------------------------02-------------------------------------------------
			a = objPointF[2][i*2];
			b = objPointF[2][i*2+1];
			c = objPointF[0][j*2];
			d = objPointF[0][j*2+1];
			if((fabs(a+1)>0.0000001) && (fabs(b+1)>0.0000001) && (fabs(c+1)>0.0000001) && (fabs(d+1)>0.0000001)){
			dis02 = sqrt((a-c)*(a-c) + (b-d)*(b-d));			//2->0距离
			r02[0] = a-c;										//2->0向量
			r02[1] = b-d;										//2->0向量
			if(dis02/2<=Vr)
			{
				//-------------------------------------23-------------------------------------------------
				minDisErr = Vr;	minDPErr = Vr;	is_continue = 1;
				for(k=0;k<g_objNum[3];k++)
				{
					e = objPointF[3][k*2];
					f = objPointF[3][k*2+1];
					if((fabs(e+1)>0.0000001) && (fabs(f+1)>0.0000001)){
					dis23 = sqrt((e-a)*(e-a) + (f-b)*(f-b));	//3->2距离
					r23[0] = e-a;								//3->2向量
					r23[1] = f-b;								//3->2向量

					curDP = (r23[0]*r02[0]+r23[1]*r02[1])/(dis23*dis02);	//cosθ
					curDPErr = 1-curDP;										//1-cosθ
					curDisErr = abs(dis23-dis02/2);							//距离偏差绝对值
				
					if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
					{
						minDisErr = curDisErr;	minDPErr = curDPErr;
						for(obj=curObjNum*5;obj<(curObjNum+1)*5;obj++){	//g_allObjInf((curObjNum-1)*5+1:curObjNum*5,3) = curObjNum;
							g_allObjInf[obj*3+2] = curObjNum+1;}
						g_allObjInf[(curObjNum*5+0)*3]		= objPointF[0][j*2];
						g_allObjInf[(curObjNum*5+0)*3+1]	= objPointF[0][j*2+1];
						g_allObjInf[(curObjNum*5+2)*3]		= objPointF[2][i*2];
						g_allObjInf[(curObjNum*5+2)*3+1]	= objPointF[2][i*2+1];
						g_allObjInf[(curObjNum*5+3)*3]		= objPointF[3][k*2];
						g_allObjInf[(curObjNum*5+3)*3+1]	= objPointF[3][k*2+1];
						objPointF[0][j*2]	= -1;
						objPointF[0][j*2+1]	= -1;
						objPointF[2][i*2]	= -1;
						objPointF[2][i*2+1]	= -1;
						objPointF[3][k*2]	= -1;
						objPointF[3][k*2+1]	= -1;
						curObjNum++;
						if(curObjNum>39)	return curObjNum;
						is_continue			= 0;
					}}
				}
				//-------------------------------------24-------------------------------------------------(没有3)
				if(is_continue != 0){	//不继续就跳过
				minDisErr = Vr;	minDPErr = Vr;
				for(k=0;k<g_objNum[4];k++)
				{
					e = objPointF[4][k*2];
					f = objPointF[4][k*2+1];
					if((fabs(e+1)>0.0000001) && (fabs(f+1)>0.0000001)){
					dis24 = sqrt((e-a)*(e-a) + (f-b)*(f-b));	//4->2距离
					r24[0] = e-a;								//4->2向量
					r24[1] = f-b;								//4->2向量

					curDP = (r24[0]*r02[0]+r24[1]*r02[1])/(dis24*dis02);	//cosθ
					curDPErr = 1-curDP;										//1-cosθ
					curDisErr = abs(dis24/2-dis02/2);							//距离偏差绝对值
				
					if((curDisErr<minDisErr)&&(curDisErr<=err)&&(curDPErr<minDPErr)&&(curDPErr<=0.1))	//距离和角度都满足
					{
						minDisErr = curDisErr;	minDPErr = curDPErr;
						for(obj=curObjNum*5;obj<(curObjNum+1)*5;obj++){	//g_allObjInf((curObjNum-1)*5+1:curObjNum*5,3) = curObjNum;
							g_allObjInf[obj*3+2] = curObjNum+1;}
						g_allObjInf[(curObjNum*5+0)*3]		= objPointF[0][j*2];
						g_allObjInf[(curObjNum*5+0)*3+1]	= objPointF[0][j*2+1];
						g_allObjInf[(curObjNum*5+2)*3]		= objPointF[2][i*2];
						g_allObjInf[(curObjNum*5+2)*3+1]	= objPointF[2][i*2+1];
						g_allObjInf[(curObjNum*5+4)*3]		= objPointF[4][k*2];
						g_allObjInf[(curObjNum*5+4)*3+1]	= objPointF[4][k*2+1];
						objPointF[0][j*2]	= -1;
						objPointF[0][j*2+1]	= -1;
						objPointF[2][i*2]	= -1;
						objPointF[2][i*2+1]	= -1;
						objPointF[4][k*2]	= -1;
						objPointF[4][k*2+1]	= -1;
						curObjNum++;
						if(curObjNum>39)	return curObjNum;
						is_continue			= 0;
					}}
				}}
			}}
		}}
	return curObjNum;
}

/********************************修正偏移量******************************************/
void modify(unsigned short cnt)
{
	int i=0;
	if(cnt == 0)
		return;
	else
	{
		for(i=0;i<cnt;i++)
		{
			if(fabs(g_allObjInf[i*15+0]) > 0.0000001)
				g_allObjInf[i*15+0] += g_offsetR[0];
			if((g_allObjInf[i*15+0]<0) || (g_allObjInf[i*15+0]>1023))	g_allObjInf[i*15+0] = 0;
			if(fabs(g_allObjInf[i*15+1]) > 0.0000001)
				g_allObjInf[i*15+1] += g_offsetC[0];
			if((g_allObjInf[i*15+1]<0) || (g_allObjInf[i*15+1]>1023))	g_allObjInf[i*15+1] = 0;
			if(fabs(g_allObjInf[i*15+3]) > 0.0000001)
				g_allObjInf[i*15+3] += g_offsetR[1];
			if((g_allObjInf[i*15+3]<0) || (g_allObjInf[i*15+3]>1023))	g_allObjInf[i*15+3] = 0;
			if(fabs(g_allObjInf[i*15+4]) > 0.0000001)
				g_allObjInf[i*15+4] += g_offsetC[1];
			if((g_allObjInf[i*15+4]<0) || (g_allObjInf[i*15+4]>1023))	g_allObjInf[i*15+4] = 0;
			if(fabs(g_allObjInf[i*15+6]) > 0.0000001)
				g_allObjInf[i*15+6] += g_offsetR[2];
			if((g_allObjInf[i*15+6]<0) || (g_allObjInf[i*15+6]>1023))	g_allObjInf[i*15+6] = 0;
			if(fabs(g_allObjInf[i*15+7]) > 0.0000001)
				g_allObjInf[i*15+7] += g_offsetC[2];
			if((g_allObjInf[i*15+7]<0) || (g_allObjInf[i*15+7]>1023))	g_allObjInf[i*15+7] = 0;
			if(fabs(g_allObjInf[i*15+9]) > 0.0000001)
				g_allObjInf[i*15+9] += g_offsetR[3];
			if((g_allObjInf[i*15+9]<0) || (g_allObjInf[i*15+9]>1023))	g_allObjInf[i*15+9] = 0;
			if(fabs(g_allObjInf[i*15+10]) > 0.0000001)
				g_allObjInf[i*15+10] += g_offsetC[3];
			if((g_allObjInf[i*15+10]<0) || (g_allObjInf[i*15+10]>1023))	g_allObjInf[i*15+10] = 0;
			if(fabs(g_allObjInf[i*15+12]) > 0.0000001)
				g_allObjInf[i*15+12] += g_offsetR[4];
			if((g_allObjInf[i*15+12]<0) || (g_allObjInf[i*15+12]>1023))	g_allObjInf[i*15+12] = 0;
			if(fabs(g_allObjInf[i*15+13]) > 0.0000001)
				g_allObjInf[i*15+13] += g_offsetC[4];
			if((g_allObjInf[i*15+13]<0) || (g_allObjInf[i*15+13]>1023))	g_allObjInf[i*15+13] = 0;
		}
	}
}

extern void getCentroid(void);
extern void moveFilter(void);
extern void moveFill(void);

void move(void)	//2150ms
{
	unsigned short countRegion = 0;
	getMaxIndex();
	clear();
	countRegion = region();
	g_moveSum = movepath(countRegion);
	modify(g_moveSum);
	getCentroid();
	moveFilter();
	moveFill();
	return;
}


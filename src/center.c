#include <stdio.h>
#include <math.h>
#include <yj6701.h>

/*************************计算目标质心、取目标质心图*********************************
**********************************参数说明*******************************************
	R：		粗略行						——输入
	C：		粗略列						——输入
	frm：	当前帧号					——输入
	idx_in：目标索引					——输入
	cenR:	质心行						——输入输出
	cenC:	质心列						——输入输出
**********************************临时变量*******************************************
	row、col：							----循环使用行、列
	max_Row、max_Col：					----最大亮度像素的行、列
	max_Val：							----最大亮度像素的灰度值
	curIdx：							----当前目标的索引
	k：									----指针
	pointQueue：						----队列指针
	queue：								----种子填充使用的队列
	SUM_MSK：							----种子填充得到的15*15图(已归一化)
	SUM_RMB：							----用于求质心的最终15*15图
	var：								----筛选满足条件的阈值
	cur：								----当前值
	m、sumM、sumC、sumR:				----求质心使用
************************************************************************************/
void getCenIM(short R, short C, unsigned short frm, unsigned char *idx_in, float *cenR, float *cenC)
{
	short row=0,col=0,max_Row=0,max_Col=0,curIdx=0,k=0,pointQueue=0;
	unsigned short queue[225][2],max_Val=0;
	float SUM_RMB[225],SUM_MSK[225],var = 0.1,cur=0,m=0,sumM=0,sumC=0,sumR=0;
	
	if ((R - g_offsetR[frm] < 0) || (C - g_offsetC[frm] < 0) || (R - g_offsetR[frm] >= IMG_WID) || (C - g_offsetC[frm] >= IMG_WID) || (R >= IMG_HEI) || (C >= IMG_WID) || (R < 0) || (C < 0))
	{
		return;
	}
	
	memset(SUM_RMB,0,900);
	memset(SUM_MSK,0,900);
	memset(queue,0,900);
	
	//取目标中最大亮度的像素位置
	max_Row=R;	max_Col=C;	max_Val=0;
	curIdx = idx_in[(R-g_offsetR[frm])*IMG_WID+C-g_offsetC[frm]];
	if (curIdx == 0)
	{
		return;
	}
	for(row=R-2;row<=R+2;row++)
	{
		for(col=C-2;col<=C+2;col++)
		{
			if((row-g_offsetR[frm]>=0) && (col-g_offsetC[frm]>=0) && (row-g_offsetR[frm]<IMG_WID) && (col-g_offsetC[frm]<IMG_WID) && (row>=0) && (row<IMG_HEI) && (col>=0) && (col< IMG_WID))
			{
				if((idx_in[(row-g_offsetR[frm])*IMG_WID+col-g_offsetC[frm]] == curIdx) && (g_imgRmb[frm][row*IMG_WID+col]>max_Val))
				{
					max_Row = row;
					max_Col = col;
					max_Val = g_imgRmb[frm][row*IMG_WID+col];
				}
			}
		}
	}
	if (max_Val == 0)
	{
		return;
	}

	//取图并归一化
	k = 0;
	for(row=max_Row-7;row<=max_Row+7;row++)
	{
		for(col=max_Col-7;col<=max_Col+7;col++)
		{
			if ((row < 0) || (col < 0) || (row >= IMG_WID) || (col >= IMG_WID))
			{
				SUM_MSK[k++] = 0;
			}
			else
			{
				SUM_MSK[k++] = pow(((double)g_imgRmb[frm][row*IMG_WID + col]) / max_Val, 2.0);	//归一化并平方(将所有灰度值置为0到1中间的值)
			}
		}
	}
		
	//4种子填充
	pointQueue=0;
	queue[pointQueue][0]	= 7;
	queue[pointQueue++][1]	= 7;
	SUM_RMB[7*15+7] = SUM_MSK[7*15+7];		//取小数
	for(k=0;k<pointQueue;k++)				//遍历queue中的第queueCount个
	{
		row = queue[k][0];
		col = queue[k][1];
		cur = SUM_MSK[row*15+col];
		SUM_MSK[row*15+col] = 0;

		if((SUM_MSK[(row-1)*15+col]<=cur) && (SUM_MSK[(row-1)*15+col]>var) && (row>0))
		{
			queue[pointQueue][0]	= row-1;
			queue[pointQueue++][1]	= col;
			SUM_RMB[(row-1)*15+col]	= SUM_MSK[(row-1)*15+col];									//取小数
		}
		if((SUM_MSK[row*15+col-1]<=cur) && (SUM_MSK[row*15+col-1]>var) && (col>0))
		{
			queue[pointQueue][0]	= row;
			queue[pointQueue++][1]	= col-1;
			SUM_RMB[row*15+col-1]	= SUM_MSK[row*15+col-1];									//取小数
		}
		if((SUM_MSK[(row+1)*15+col]<=cur) && (SUM_MSK[(row+1)*15+col]>var) && (row<14))
		{
			queue[pointQueue][0]	= row+1;
			queue[pointQueue++][1]	= col;
			SUM_RMB[(row+1)*15+col]	= SUM_MSK[(row+1)*15+col];									//取小数
		}
		if((SUM_MSK[row*15+col+1]<=cur) && (SUM_MSK[row*15+col+1]>var) && (col<14))
		{
			queue[pointQueue][0]	= row;
			queue[pointQueue++][1]	= col+1;
			SUM_RMB[row*15+col+1]	= SUM_MSK[row*15+col+1];									//取小数
		}
	}

	////求质心
	for(row=0;row<15;row++)
	{
		for(col=0;col<15;col++)
		{
			m = SUM_RMB[row*15+col];
			sumM += m;
			sumC += col*m;
			sumR += row*m;
		}
	}
	*cenR = max_Row+sumR/sumM-7;
	*cenC = max_Col+sumC/sumM-7;
}

/**********************************求质心*******************************************/
void getCentroid(void)
{
	unsigned short frm = 0, i = 0;

	for(i=0;i<g_moveSum;i++)
	{
		for(frm=0;frm<5;frm++)
		{
			getCenIM(g_allObjInf[15 * i + frm * 3], g_allObjInf[15 * i + frm * 3 + 1], frm, g_corImg, &g_allObjInf[15 * i + frm * 3], &g_allObjInf[15 * i + frm * 3 + 1]);
		}
	}
}

/***********************************
	0.	○	○	○	○	○----
	1.	×	○	○	○	○----①
	2.	○	×	○	○	○----①
	3.	○	○	×	○	○----①
	4.	○	○	○	×	○----①
	5.	○	○	○	○	×----①
	6.	×	×	○	○	○----②
	7.	○	×	×	○	○----③
	8.	○	○	×	×	○----④
	9.	○	○	○	×	×----
	10.	×	○	×	○	○----
	11.	×	○	○	×	○----
	12.	×	○	○	○	×----
	13.	○	×	○	×	○----
	14.	○	×	○	○	×----
	15.	○	○	×	○	×----
***********************************/
void moveFilter(void)
{
	float a[5][2] = {{0,0},{0,0},{0,0},{0,0},{0,0}};			//存放10个行列值
	float r[4][2]={{0,0},{0,0},{0,0},{0,0}},dis[4]={0,0,0,0};	//向量
	float curDP = 0;											//判断角度
	unsigned short cnt = 0,i = 0,j = 0,error = 0,frmBuf[5] = {0,0,0,0,0};
	
	for(cnt=0;cnt<g_moveSum;cnt++)
	{
		memset(a[0],0,40);
		memset(r[0],0,32);
		memset(dis,0,16);
		memset(frmBuf,0,10);
		for(i=0,j=0;i<5;i++)
		{
			a[j][0] = g_allObjInf[cnt*15+i*3+0];
			a[j][1] = g_allObjInf[cnt*15+i*3+1];
			if((abs(a[j][0]) > 0.0000001) && (abs(a[j][1]) > 0.0000001))
			{
				a[j][0] -= g_offsetR[i];
				a[j][1] -= g_offsetC[i];
				frmBuf[j] = i;
				if(j>0)
				{
					r[j-1][0] = a[j][0] - a[j-1][0];
					r[j-1][1] = a[j][1] - a[j-1][1];
					dis[j-1] = sqrt(pow((double)(a[j][0] - a[j-1][0]),2.0) + pow((double)(a[j][1] - a[j-1][1]),2.0));
				}
				j++;
			}
		}
		
		if(j < 3)
		{
			memset(g_allObjInf+cnt*15,0,15*4);
			error++;
		}
		if(j == 3)
		{
			if((a[0][0] - (int)a[0][0] > 0) && (a[0][1] - (int)a[0][1] > 0) && (a[1][0] - (int)a[1][0] > 0) && (a[1][1] - (int)a[1][1] > 0) && (a[2][0] - (int)a[2][0] > 0) && (a[2][1] - (int)a[2][1] > 0))
			{
				for(i=1;i<j-1;i++)
				{
					curDP = 1 - (r[i][0]*r[i-1][0]+r[i][1]*r[i-1][1])/(dis[i]*dis[i-1]);
					if((curDP > 0.01) || (curDP < 0) || (dis[i]/(frmBuf[i+1] - frmBuf[i]) < 2) || (dis[i-1]/(frmBuf[i] - frmBuf[i-1]) < 2) || (abs(dis[i]/(frmBuf[i+1] - frmBuf[i]) - dis[i-1]/(frmBuf[i] - frmBuf[i-1])) > 0.8))
					{
						memset(g_allObjInf+cnt*15,0,15*4);
						error++;
						break;
					}
				}
			}
			else
			{
				memset(g_allObjInf+cnt*15,0,15*4);
				error++;
			}
		}
	}
	g_moveSum -= error;

	return;
}

void moveFill(void)
{
	int i=0,j=0,m=0;
	float row = 0,col = 0,a[2][3]={{0,0,0},{0,0,0}};

	for(i=0,j=0;i<g_moveSum;j++)
	{
		if(abs(g_allObjInf[15*j+2]) > 0.0000001)
		{
			if(i<j)
			{
				memcpy(g_allObjInf+15*i,g_allObjInf+15*j,60);
				memset(g_allObjInf+15*j,0,60);
			}
			i++;
		}
	}
	
	for(i=0;i<g_moveSum;i++)
	{
		m=0;
		for(j=0;(j<5)&&(m<2);j++)
		{
			if(abs(g_allObjInf[15*i+j*3]) > 0.0000001)
			{
				a[m][0] = g_allObjInf[15*i+j*3];	//行
				a[m][1] = g_allObjInf[15*i+j*3+1];	//列
				a[m][2] = j;						//帧号
				m++;
			}
		}
		row = (a[1][0] - a[0][0])/(a[1][2] - a[0][2]);
		col = (a[1][1] - a[0][1])/(a[1][2] - a[0][2]);
		for(j=0;j<5;j++)
		{
			if(abs(g_allObjInf[15*i+j*3]) < 0.0000001)
			{
				g_allObjInf[15*i+j*3] = a[0][0] + row*(j - a[0][2]);
				g_allObjInf[15*i+j*3+1] = a[0][1] + col*(j - a[0][2]);
				if((g_allObjInf[15*i+j*3]<0) || (g_allObjInf[15*i+j*3]>1023) || (g_allObjInf[15*i+j*3+1]<0) || (g_allObjInf[15*i+j*3+1]>1023))
				{
					g_allObjInf[15*i+j*3] = 0;
					g_allObjInf[15*i+j*3+1] = 0;
				}
			}
		}
	}

	return;
}

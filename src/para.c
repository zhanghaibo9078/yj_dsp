#include <STDIO.H> 
#include <MATH.H>
#include <yj6701.h>

/********************************曝光时间映射***************************************/
/*******************************参数说明*********************************************
b：曝光时间字节（8bit）			——输入
************************************************************************************/
int mapExpl(unsigned char b)
{
	switch (b)
	{
	case 0x00:
		return 150;
	case 0x0C:
		return 300;
	case 0x18:
		return 450;
	case 0x24:
		return 600;
	case 0x30:
		return 750;
	case 0x3C:
		return 900;
	case 0x48:
		return 1050;
	case 0x54:
		return 1200;
	case 0x60:
		return 1350;
	case 0x6C:
		return 1500;
	case 0x78:
		return 1650;
	case 0x84:
		return 1800;
	case 0x90:
		return 1950;
	case 0x9C:
		return 2100;
	case 0xA8:
		return 2250;
	case 0xB4:
		return 2400;
	case 0xC0:
		return 2550;
	case 0xCC:
		return 2700;
	case 0xD8:
		return 2850;
	case 0xE4:
		return 3000;
	default:
		return -1;
	}
}

/********************************解读控制参数***************************************/
/*******************************参数说明*********************************************
para：控制参数（32bit）			——输入
|31~20|	|	19~18|	*|17-16|	*|	15-8  |*	|	7-6  |	|5-4 |	|3-2 |	|		1-0  |
|空闲 | |工作模式|	*|帧频 |	*|曝光时间|*	|读出通道|	|增益|	|偏置|	|图像输出接口|
************************************************************************************/
void para2value(unsigned int *para)
{
	unsigned char *p;
	int expo = 0;
	/*顺序0-1-2-3*/
	p = (unsigned char *)para;
	/*曝光时间*/
	expo = mapExpl(p[2]);
	if (g_thresholdMW == 0)
	{
		if (expo < 900)
		{
			/*去噪索引*/
			g_thresholdMW = 3;
		}
		else if (expo < 1650)
		{
			/*去噪索引*/
			g_thresholdMW = 4;
		}
		else if (expo < 2850)
		{
			/*去噪索引*/
			g_thresholdMW = 5;
		}
		else if (expo == 3000)
		{
			/*去噪索引*/
			g_thresholdMW = 6;
		}
		else
		{
			/*去噪索引*/
			g_thresholdMW = 3;
		}
	}
	switch (p[3] & 0x30)
	{
	case 0x10:
		/*1倍增益*/
		if (g_thresholdBW == 70)
		{
			g_thresholdBW = 20;
		}
		if (g_thresholdBCK == 0)
		{
			if (expo < 1350)
			{
				/*背景预处理*/
				g_thresholdBCK = 2;
			}
			else if (expo < 2400)
			{
				/*背景预处理*/
				g_thresholdBCK = 3;
			}
			else if (expo < 3000)
			{
				/*背景预处理*/
				g_thresholdBCK = 4;
			}
			else if (expo == 3000)
			{
				/*背景预处理*/
				g_thresholdBCK = 5;
			}
			else
			{
				/*背景预处理*/
				g_thresholdBCK = 2;
			}
		}
		break;
	default:
		/*自动增益/4倍增益*/
		if (g_thresholdBW == 70)
		{
			g_thresholdBW = 100;
		}
		if (g_thresholdBCK == 0)
		{
			if (expo < 600)
			{	
				/*背景预处理*/
				g_thresholdBCK = 9;
			}
			else if (expo < 1050)
			{	
				/*背景预处理*/
				g_thresholdBCK = 11;
			}
			else if (expo < 1500)
			{	
				/*背景预处理*/
				g_thresholdBCK = 14;
			}
			else if (expo < 1950)
			{	
				/*背景预处理*/
				g_thresholdBCK = 20;
			}
			else if (expo < 2400)
			{	
				/*背景预处理*/
				g_thresholdBCK = 23;
			}
			else if (expo <= 3000)
			{	
				/*背景预处理*/
				g_thresholdBCK = 26;
			}
			else
			{	
				/*背景预处理*/
				g_thresholdBCK = 7;
			}
		}
		break;
	}
}

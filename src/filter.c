#include "../include/yj6701.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

extern void writeImg(void *p,char *str,unsigned int len);

//用镜像后图像滤波1次，in图像为镜像后大小，out图像为原图大小,wid和hei为镜像后的尺寸
void mirrorFileter(unsigned short *inImg,unsigned short *outImg,unsigned short window,unsigned short wid,unsigned short hei)
{
	unsigned short i=0,j=0,k=0,n=0;
	unsigned int sum=0,rowSum=0;

	unsigned short *IM = (unsigned short*)malloc(window*window*sizeof(unsigned short));		//求均值缓存

	for(i=window/2;i<=hei-window/2;i++)		//行
		for(j=window/2;j<=wid-window/2;j++)	//列
		{
			//抠图32*32
			n=0;
			for(k=i-16;k<i+16;k++)
				memcpy(IM+n++*window,inImg+k*wid+j-window/2,window*2);
			//求均值
			sum = 0;
			for(n=0;n<window;n++)
			{
				rowSum = 0;
				for(k=0;k<window;k++)
					rowSum += IM[n*window+k];
				sum += rowSum>>5;
			}
			sum = sum>>5;
			//赋值
			//if(inImg[i*wid+j]<sum+g_thresholdBCK)
			//	outImg[(i-window/2)*(wid-window+1)+(j-window/2)] = inImg[i*wid+j];
			//else
			//	outImg[(i-window/2)*(wid-window+1)+(j-window/2)] = sum + g_thresholdBCK;
			outImg[(i-window/2)*(wid-window+1)+(j-window/2)] = sum;
		}
}

//对图像进行镜像处理，out图像为镜像后大小，in图像为原图大小,wid和hei为原图的尺寸
void mirrorImg(unsigned short *inImg,unsigned short *outImg,unsigned short window,unsigned short wid,unsigned short hei)
{
	unsigned short outWid = wid+window-1;
	unsigned short outHei = hei+window-1;
	unsigned short i=0,j=0;
	for(i=0;i<hei;i++)
		memcpy(outImg+(i+window/2)*outWid+window/2,inImg+i*wid,wid*2);
	for(i=window/2;i<window/2+hei;i++)
	{
		for(j=0;j<window/2;j++)
			outImg[i*outWid+j] = outImg[i*outWid+window-1-j];
		for(j=window/2+wid;j<outWid;j++)
			outImg[i*outWid+j] = outImg[i*outWid+(wid+outWid-j)];
	}
	for(i=0;i<window/2;i++)
		memcpy(outImg+i*outWid,outImg+(window-1-i)*outWid,outWid*2);
	for(i=hei+window/2;i<outHei;i++)
		memcpy(outImg+i*outWid,outImg+(hei+outHei-i)*outWid,outWid*2);
}

void filter(unsigned short **inImg,unsigned short **outImg,unsigned short cnt,unsigned short window,unsigned short wid,unsigned short hei)
{
	if(cnt==0)	return;
	unsigned int a,f,i,j;
	unsigned short *inImgL = (unsigned short*)malloc(wid*hei/2*sizeof(unsigned short));							//左半幅图
	unsigned short *inImgR = (unsigned short*)malloc(wid*hei/2*sizeof(unsigned short));							//右半幅图
	unsigned short *imgBck = (unsigned short*)malloc(wid*hei/2*sizeof(unsigned short));							//背景图
	memset(imgBck,0,wid*hei);
	unsigned short *imgMir = (unsigned short*)malloc((wid/2+window-1)*(hei+window-1)*sizeof(unsigned short));	//镜像图
	memset(imgMir,0,(wid/2+window-1)*(hei+window-1)*2);

	//帧号
	for(f=0;f<5;f++)
	{
		printf("开始滤波第%d帧\n",f);
		for(i=0;i<hei;i++)
		{
			memcpy(inImgL+i*wid/2,inImg[f]+i*wid,wid);
			memcpy(inImgR+i*wid/2,inImg[f]+i*wid+wid/2,wid);
		}
		/*-------------左---------------*/
		memcpy(imgBck,inImgL,wid*hei);
		//滤波次数
		for(a=0;a<cnt;a++)
		{
			mirrorImg(imgBck,imgMir,window,wid/2,hei);	//镜像
			/*FILE *f = fopen("1.raw","wb+");
			fwrite(imgMir,1,(wid/2+window-1)*(hei+window-1)*2,f);
			fclose(f);*/
			mirrorFileter(imgMir,imgBck,window,wid/2+window-1,hei+window-1);	//滤波
		}
		for(i=0;i<hei;i++)
		{
			memcpy(outImg[f]+i*wid,imgBck+i*wid/2,wid);
		}
		/*-------------右---------------*/
		memcpy(imgBck,inImgR,wid*hei);
		//滤波次数
		for(a=0;a<cnt;a++)
		{
			mirrorImg(imgBck,imgMir,window,wid/2,hei);	//镜像
			mirrorFileter(imgMir,imgBck,window,wid/2+window-1,hei+window-1);	//滤波
		}
		for(i=0;i<hei;i++)
		{
			memcpy(outImg[f]+i*wid+wid/2,imgBck+i*wid/2,wid);
		}
		char s[260];
		// sprintf(s,"data\\filter_background_%d.raw",f);
		// writeImg(outImg[f],s,IMG_FRM*2);

		for(a=0;a<wid*hei;a++)
		{
			if(inImg[f][a]>(outImg[f][a]))//+g_thresholdBCK))
				outImg[f][a] = inImg[f][a] - outImg[f][a];// - g_thresholdBCK;
			else
				outImg[f][a] = 0;
				//outImg[f][a] = outImg[f][a] + g_thresholdBCK - inImg[f][a];
		}
		sprintf(s,"data\\before_filter_%d.raw",f);
		writeImg(inImg[f],s,IMG_FRM*2);
		sprintf(s,"data\\after_filter_%d.raw",f);
		writeImg(outImg[f],s,IMG_FRM*2);
	}
	return;
}
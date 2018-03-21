#include "../include/yj6701.h"

void init()
{
    int value = 0;
    printf("输入“二值化参数”：70\n");
    g_thresholdBW = 70;
    printf("输入“去噪索引参数”：0\n");
    // scanf("%d",&value);
    g_thresholdMW = value;
    printf("输入“背景预处理参数”：0\n");
    // scanf("%d",&value);
    g_thresholdBCK = value;
    for(int i=0;i<5;i++)
    {
        g_imgOrig[i] = (unsigned short *)malloc(IMG_WID*IMG_HEI*sizeof(unsigned short));	//原始图像存储指针
        memset(g_imgOrig[i],0,IMG_WID*IMG_HEI*sizeof(unsigned short));
        g_imgRmb[i] = (unsigned short *)malloc(IMG_WID*IMG_HEI*sizeof(unsigned short));	    //去除背景后图像存储指针
        memset(g_imgRmb[i],0,IMG_WID*IMG_HEI*sizeof(unsigned short));
        g_tris[i]	=(struct tri*)malloc(120*sizeof(struct tri));                           //配准用三角形
        memset(g_tris[i],0,120*sizeof(struct tri));
        g_stars[i]	=(struct star*)malloc(10*sizeof(struct star));                         //配准用星
        memset(g_stars[i],0,10*sizeof(struct star));
        objPointF[i]    = (float *)malloc(2048*sizeof(float));				                //第0,1,2,3,4帧图连通域的坐标信息
        memset(objPointF[i],0,2048*sizeof(float));
    }
    g_regLabelMap = (unsigned char *)malloc(IMG_WID*IMG_HEI*sizeof(unsigned char));     //配准连通域图(1024*1024B)
    memset(g_regLabelMap,0,IMG_WID*IMG_HEI*sizeof(unsigned char));
    g_maxIndex = (unsigned char *)malloc(IMG_WID*IMG_HEI*sizeof(unsigned char));	    //最大值索引帧
    memset(g_maxIndex,0,IMG_WID*IMG_HEI*sizeof(unsigned char));
    g_corImg = (unsigned char *)malloc(IMG_WID*IMG_HEI*sizeof(unsigned char));	        //去噪后
    memset(g_corImg,0,IMG_WID*IMG_HEI*sizeof(unsigned char));
    g_objPntLst		= (float *)malloc(4096*sizeof(float));				                //存储帧号、连通域、及粗略坐标
    g_allObjInf		= (float *)malloc(2048*sizeof(float));				                //运动目标细则(5行3列的倍数)
    g_moveSum		= 0;                    							                //运动目标总数
    fWriteHex = fopen("data\\result.hex", "wb+");
}

/******************读图像*********************
参数str：文件路径（必须以.raw结尾）
参数frmBeg：处理的文件起始帧号
参数framePara：图像帧计数（2B）
参数time：曝光起始时间（7B*5）
参数control_word：控制字（4B）
**********************************************/
int readFile(char *str,int frmBeg,unsigned int time[5][2], unsigned int *framePara,unsigned int *control_word)
{
    FILE *fp[5];
    char strName[260];
    int i = 0,j=0;

    //打开文件
    for(i=0;i<5;i++)
    {
        sprintf(strName,"%s%d.raw",str,frmBeg+i);
        printf("读--%s\n",strName);
        fp[i] = fopen(strName,"rb");
        if(fp[i] == NULL)
            return 0;
    }

    //判断是否为1072*1028
    fread(g_imgOrig[0],1,8,fp[0]);
    fseek(fp[0],0,0);
    if((g_imgOrig[0][0] == 0x5449 && g_imgOrig[0][1] == 0x1FCE)||(g_imgOrig[0][0] == 0x4954 && g_imgOrig[0][1] == 0xCE1F))
    {
        printf("源图像尺寸：1072_1028pix\n");
        //读取文件
        for(i=0;i<5;i++)
        {
            for(j=1;j<1025;j++)
            {
                fseek(fp[i],j*1072*2+48,0);
                fread(g_imgOrig[i]+1024*(j-1),1,1024*2,fp[i]);
            }
            fseek(fp[i], 8, 0);								//定位到第8个字节
            fread(time[i], 1, 7, fp[i]);					//读取7个字节时间码
            if (i == 0)
            {
                fseek(fp[i], 22, 0);						//定位到第22个字节
                fread((char *)framePara + 1, 1, 1, fp[i]);	//读取2个字节帧计数
                fread((char *)framePara, 1, 1, fp[i]);		//读取2个字节帧计数
            }
            fseek(fp[i], 24, 0);							//定位到第24个字节
            fread(&control_word[i], 1, 4, fp[i]);			//读取4个字节控制参数
        }
    }
    else
    {
        printf("1024_1024pix\n");
        //读取文件
        for(i=0;i<5;i++)
            fread(g_imgOrig[i],1,IMG_FRM*2,fp[i]);
    }
    //关闭文件
    for(i=0;i<5;i++)
        fclose(fp[i]);
    return 1;
}

//写滤波后的图
void write12345()
{
	FILE *fp;
	fp = fopen("..\\data\\滤波结果1.raw","wb+");
	fwrite(g_imgRmb[0],1,IMG_FRM*2,fp);
	fclose(fp);
	fp = fopen("..\\data\\滤波结果2.raw","wb+");
	fwrite(g_imgRmb[1],1,IMG_FRM*2,fp);
	fclose(fp);
	fp = fopen("..\\data\\滤波结果3.raw","wb+");
	fwrite(g_imgRmb[2],1,IMG_FRM*2,fp);
	fclose(fp);
	fp = fopen("..\\data\\滤波结果4.raw","wb+");
	fwrite(g_imgRmb[3],1,IMG_FRM*2,fp);
	fclose(fp);
	fp = fopen("..\\data\\滤波结果5.raw","wb+");
	fwrite(g_imgRmb[4],1,IMG_FRM*2,fp);
	fclose(fp);
	return;
}

//写图
void writeImg(void *p,char *str,unsigned int len)
{
	FILE *fp;
	fp = fopen(str,"wb+");
	fwrite(p,1,len,fp);
	fclose(fp);
	return;
}

//写目标位置
void writePosi(int frm)
{
	FILE *fp;
	char str[260];
	int i=0,j=0,k=0;

    if(frm==1)
    {
        fp = fopen("data\\starPosi.txt","wb+");
        sprintf(str,"帧号   目标号        行             列\n");
        fprintf(fp,str);
    }
    else
        fp = fopen("data\\starPosi.txt","a");
    for(i=0;i<5;i++)
    {
        for(j=0;j<g_starSum[i];j++)
        {
            sprintf(str,"%03d     %03d      %f     %f	%f\n",frm+i,j, g_starPosi[i][3*j], g_starPosi[i][3 * j + 1], g_starPosi[i][3 * j + 2]);	//帧号||目标号||行||列||亮度
            fprintf(fp,str);
        }
    }
    fclose(fp);
    fp=NULL;
    if(frm==1)
    {
        fp = fopen("data\\movePosi.txt","wb+");
        sprintf(str,"帧号   目标号        行             列\n");
        fprintf(fp,str);
    }
    else
        fp = fopen("data\\movePosi.txt","a");
    for(i=0,k=0;i<g_moveSum;i++,k++)
    {
        if(g_allObjInf[15*k+2] == 0)
        {
            i--;
            continue;
        }
        for(j=0;j<5;j++)
        {
            sprintf(str,"%03d     %03d      %.4f     %.4f\n",frm+j,i,g_allObjInf[15*k+3*j],g_allObjInf[15*k+3*j+1]);	//帧号||目标号||行||列
            fprintf(fp,str);
        }
    }
    fclose(fp);
}
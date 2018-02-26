#include <STDIO.H>    
#include <CSL_IRQ.H>
#include <math.h>
#include <yj6701.h>
/***************************************************************/
/*    EMIF接收图像                                             */
/***************************************************************/
void emifRecvImg(void)
{
	unsigned int i=0;
	int *recv_To_sd;

	if(g_frameNum_r%2)	//接收预处理后图像
	{
		i = 0;
		//memset(g_imgRmb[g_frameNum_r/2%5],0,IMG_WID*IMG_HEI);
		recv_To_sd = (int *)(g_imgRmb[g_frameNum_r/2%5]);
		g_emifImg = (unsigned int *)0x03040000;		//指向预处理后图像地址
		while(i<1024*512){
			//memcpy((float *)(g_imgRmb[g_frameNum_r/2%5])+i++,g_emifImg+(i%65536),4);
			*(recv_To_sd+i) = *g_emifImg;i++;}
	}
	else	//接收原始图像
	{
		i = 0;
		//memset(g_imgOrig[g_frameNum_r/2%5],0,IMG_WID*IMG_HEI);
		recv_To_sd = (int *)(g_imgOrig[g_frameNum_r/2%5]);
		g_emifImg = (unsigned int *)0x03000000;		//指向原始图像地址
		while(i<1024*512){
			//memcpy((float *)(g_imgOrig[g_frameNum_r/2%5])+i++,g_emifImg+(i%65536),4);
			*(recv_To_sd+i) = *g_emifImg;i++;}
	}

	return;
}
/***************************************************************/
/*    设置中断		                                           */
/***************************************************************/
void SetupInterrupts(void)
{
    IER=0x0032;		//设置中断
    CSR=CSR|0x1;	//打开中断
}
/***************************************************************/
/*    EMIF接收注入数据                                         */
/***************************************************************/
void emifRecvEE(void)
{
	unsigned short i=0;
	g_emifImg = (unsigned int *)0x03180000;		//指向接收注入数据的地址

	while(i<512){
		memcpy(g_AreaEE+g_injectNum_r*512+i,g_emifImg+(i%65536),4);i++;}	//一次接收2KB数据

	if(g_injectSum == 0)
		g_injectSum = *((unsigned char *)g_AreaEE+2);	//获取需要接收的总中断次数

	return;
}
/***************************************************************/
/*    将注入进度发送                                           */
/***************************************************************/
void sendStepInject(unsigned int step)
{
	int i=0,j=0;
	unsigned char stepBuf[256];
	g_stepInject = step;
	g_emifImg = (unsigned int *)0x033C0000;			//EMIF发送
	memset(stepBuf,0,256);
	stepBuf[0] = 0xEB;
	stepBuf[1] = 0x90;
	stepBuf[8] = step;			//进度标识位
	stepBuf[253] = 0x7B + step;	//校验和
	stepBuf[254] = 0x09;
	stepBuf[255] = 0xD7;
	for(j=0;j<64;j++){
		*g_emifImg = *(unsigned int *)(stepBuf+j*4);}	//发送给FPGA
	for (i=0;i<3000;i++){;}		//延时
}
/***************************************************************/
/*    将解析注入数据                                           */
/***************************************************************/
void analyzeData(void)
{
	unsigned int checkSum = 0,checkSum_r = 0,i = 0;
	g_lenArea00 = (((unsigned int)(*((unsigned char *)g_AreaEE+3)))<<24) | (((unsigned int)(*((unsigned char *)g_AreaEE+4)))<<16) | (((unsigned int)(*((unsigned char *)g_AreaEE+5)))<<8) | ((unsigned int)(*((unsigned char *)g_AreaEE+6)));	//00区数据长度
	g_lenArea80 = (((unsigned int)(*((unsigned char *)g_AreaEE+7)))<<24) | (((unsigned int)(*((unsigned char *)g_AreaEE+8)))<<16) | (((unsigned int)(*((unsigned char *)g_AreaEE+9)))<<8) | ((unsigned int)(*((unsigned char *)g_AreaEE+10)));	//80区数据长度
	
	checkSum = 0;	//校验和
	checkSum_r = 0;	//校验和位
	for(i = 0;i < 252;)	//求校验和
	{
		checkSum += (((unsigned int)(*((unsigned char *)g_AreaEE+i)))<<24) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+1)))<<16) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+2)))<<8) | ((unsigned int)(*((unsigned char *)g_AreaEE+i+3)));
		i += 4;
	}
	checkSum_r = (((unsigned int)(*((unsigned char *)g_AreaEE+i)))<<24) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+1)))<<16) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+2)))<<8) | ((unsigned int)(*((unsigned char *)g_AreaEE+i+3)));
	i += 4;
	if(checkSum != checkSum_r)
	{
		sendStepInject(2);	//包头出错
		return;
	}
	
	if(g_lenArea00 != 0)
	{
		checkSum = 0;		//变量初始化
		checkSum_r = 0;		//变量初始化
		for(;i < g_lenArea00 + 256 - 4;)	//求校验和
		{
			checkSum += (((unsigned int)(*((unsigned char *)g_AreaEE+i)))<<24) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+1)))<<16) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+2)))<<8) | ((unsigned int)(*((unsigned char *)g_AreaEE+i+3)));
			i += 4;
		}
		checkSum_r = (((unsigned int)(*((unsigned char *)g_AreaEE+i)))<<24) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+1)))<<16) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+2)))<<8) | ((unsigned int)(*((unsigned char *)g_AreaEE+i+3)));
		i += 4;
		if(checkSum != checkSum_r)	//如果校验出错
		{
			sendStepInject(3);	//00区出错
			return;
		}
	}
	
	if(g_lenArea80 != 0)
	{
		checkSum = 0;		//变量初始化
		checkSum_r = 0;		//变量初始化
		for(;i < g_lenArea00 + g_lenArea80 + 256 - 4;)	//求校验和
		{
			checkSum += (((unsigned int)(*((unsigned char *)g_AreaEE+i)))<<24) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+1)))<<16) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+2)))<<8) | ((unsigned int)(*((unsigned char *)g_AreaEE+i+3)));
			i += 4;
		}
		checkSum_r = (((unsigned int)(*((unsigned char *)g_AreaEE+i)))<<24) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+1)))<<16) | (((unsigned int)(*((unsigned char *)g_AreaEE+i+2)))<<8) | ((unsigned int)(*((unsigned char *)g_AreaEE+i+3)));
		i += 4;
		if(checkSum != checkSum_r)	//如果校验出错
		{
			sendStepInject(4);	//80区出错
			return;
		}
	}
	sendStepInject(5);	//数据接收正确
	return;
}
/***************************************************************/
/*    将注入数据写入EE                                         */
/***************************************************************/
void begWrEE(void)
{
	unsigned int i = 0,j = 0,m = 0;
	unsigned char *src,*des;
	*(unsigned int *)0x1800004 = 0xFFFF3F23;	//CE1

   	*(unsigned int *)(0x01500000+(0x05555<<2)) = 0xaa;    //first    
	*(unsigned int *)(0x01500000+(0x0AAAA<<2)) = 0x55;    //second
	*(unsigned int *)(0x01500000+(0x05555<<2)) = 0xA0;    //third

	src = (unsigned char *)g_AreaEE+256;
	des = (unsigned char *)0x01500000;
	m=0;
	while(m<g_lenArea00)	//写00区
	{
		for (i=0;i<50;i++)
			for (j=0;j<100;j++);
		if(*(des+m*4) == *(src+m))
		{
			m++;
			if(m%1024 == 0)
				sendStepInject(5);
		}
		else
		{
			*(des+m*4) = *(src+m);
		}
	}
	sendStepInject(6);	//00区写完

	*(unsigned int *)(0x01580000+(0x05555<<2)) = 0xaa;    //first
	*(unsigned int *)(0x01580000+(0x0AAAA<<2)) = 0x55;    //second
	*(unsigned int *)(0x01580000+(0x05555<<2)) = 0xA0;    //third

	m=0;
	src = (unsigned char *)g_AreaEE+256+g_lenArea00;
	des = (unsigned char *)0x01580000;
	while(m<g_lenArea80)	//写80区
	{
		if(m<1)
		{
			for (i=0;i<5000;i++)
				for (j=0;j<100;j++);
		}
		for (i=0;i<50;i++)
			for (j=0;j<100;j++);
		if(*(des+m*4) == *(src+m))
		{
			m++;
			if(m%1024 == 0)
				sendStepInject(6);
		}
		else
		{
			*(des+m*4) = *(src+m);
		}
	}
	sendStepInject(7);	//80区写完
}

void checkEE(void)
{
	unsigned int checkSum = 0,checkSum_r = 0,i = 0,j = 0;
	*(unsigned int *)0x1800004 = 0xFFFF3F03;	//CE1

	if(g_lenArea00 != 0)
	{
		checkSum = 0;		//变量初始化
		checkSum_r = 0;		//变量初始化
		for(i=0;i < g_lenArea00 - 4;)
		{
			checkSum += (((unsigned int)(*((unsigned char *)0x01440000+i)))<<24) | (((unsigned int)(*((unsigned char *)0x01440000+i+1)))<<16) | (((unsigned int)(*((unsigned char *)0x01440000+i+2)))<<8) | ((unsigned int)(*((unsigned char *)0x01440000+i+3)));
			i += 4;
			for (j=0;j<5000;j++){;}
		}
		checkSum_r = (((unsigned int)(*((unsigned char *)0x01440000+i)))<<24) | (((unsigned int)(*((unsigned char *)0x01440000+i+1)))<<16) | (((unsigned int)(*((unsigned char *)0x01440000+i+2)))<<8) | ((unsigned int)(*((unsigned char *)0x01440000+i+3)));
		i += 4;
		if(checkSum != checkSum_r)
		{
			sendStepInject(8);
			return;
		}
	}
	sendStepInject(9);
	
	if(g_lenArea80 != 0)
	{
		checkSum = 0;		//变量初始化
		checkSum_r = 0;		//变量初始化
		for(i=0;i < g_lenArea80 - 4;)
		{
			checkSum += (((unsigned int)(*((unsigned char *)0x01460000+i)))<<24) | (((unsigned int)(*((unsigned char *)0x01460000+i+1)))<<16) | (((unsigned int)(*((unsigned char *)0x01460000+i+2)))<<8) | ((unsigned int)(*((unsigned char *)0x01460000+i+3)));
			i += 4;
			for (j=0;j<5000;j++){;}
		}
		checkSum_r = (((unsigned int)(*((unsigned char *)0x01460000+i)))<<24) | (((unsigned int)(*((unsigned char *)0x01460000+i+1)))<<16) | (((unsigned int)(*((unsigned char *)0x01460000+i+2)))<<8) | ((unsigned int)(*((unsigned char *)0x01460000+i+3)));
		i += 4;
		if(checkSum != checkSum_r)
		{
			sendStepInject(10);
			return;
		}
	}
	sendStepInject(11);
}

void beginInject(void)
{
	unsigned int i = 0,j = 0;
	sendStepInject(1);	//指示接收完成
	analyzeData();		//开始解析数据
	if(g_stepInject != 5)
	{
		while(1)
		{
			for (i=0;i<50000;i++){
				for (j=0;j<5000;j++){;}}
			sendStepInject(g_stepInject);	//发送对应状态
		}
	}
	//第一次写
	begWrEE();			//开始写EEPROM
	checkEE();			//开始校验EEPROM
	//第二次写
	if(g_stepInject != 11)
	{
		begWrEE();			//开始写EEPROM
		checkEE();			//开始校验EEPROM
	}
	while(1)
	{
		for (i=0;i<10000;i++){
			for (j=0;j<10000;j++){;}}
		sendStepInject(g_stepInject);	//重复报告最终状态直到复位
	}
}



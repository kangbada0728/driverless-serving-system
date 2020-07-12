#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>  // for ioctl
#include <sys/mman.h>
#include <unistd.h>
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h> 
#include <pthread.h> 
#include <linux/input.h>
#include "bitmap.h"
#include <linux/fb.h>  
#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define RCVBUFSIZE 32
#define	 FBDEV_FILE "/dev/fb0"
#define BIT_VALUE_24BIT   24
#define  INPUT_DEVICE_LIST	"/proc/bus/input/devices"
#define  EVENT_STR	"/dev/input/event"
#define TRUE	1
#define FALSE	0
#define DRIVER_NAME		"/dev/oled"
#define CMD_TXT_WRITE		0
#define CMD_CURSOR_POS		1
#define CMD_CEAR_SCREEN		2
#define DRIVER_NAME2		"/dev/tlcd"
#define CUSOR_THICK		10

#define CLEAR_DISPLAY		0x0001
#define CURSOR_AT_HOME		0x0002

// Entry Mode set 
#define MODE_SET_DEF		0x0004
#define MODE_SET_DIR_RIGHT	0x0002
#define MODE_SET_SHIFT		0x0001

// Display on off
#define DIS_DEF				0x0008
#define DIS_LCD				0x0004
#define DIS_CURSOR			0x0002
#define DIS_CUR_BLINK		0x0001

// shift
#define CUR_DIS_DEF			0x0010
#define CUR_DIS_SHIFT		0x0008
#define CUR_DIS_DIR			0x0004

// set DDRAM  address 
#define SET_DDRAM_ADD_DEF	0x0080

// read bit
#define BUSY_BIT			0x0080
#define DDRAM_ADD_MASK		0x007F
#define DDRAM_ADDR_LINE_1	0x0000
#define DDRAM_ADDR_LINE_2	0x0040
#define SIG_BIT_E			0x0400
#define SIG_BIT_RW			0x0200
#define SIG_BIT_RS			0x0100
#define LINE_NUM			2
#define COLUMN_NUM			16		
#define RST_BIT_MASK	0xEFFF		
#define CS_BIT_MASK		0xF7FF
#define DC_BIT_MASK		0xFBFF
#define WD_BIT_MASK		0xFDFF
#define RD_BIT_MASK		0xFEFF
#define DEFAULT_MASK	0xFFFF

#define CMD_SET_COLUMN_ADDR		0x15
#define CMD_SET_ROW_ADDR		0x75
#define CMD_WRITE_RAM			0x5C
#define CMD_READ_RAM			0x5D
#define CMD_LOCK				0xFD
#define MODE_WRITE		0
#define MODE_READ		1
#define MODE_CMD		2
#define MODE_RESET		3
#define MODE_IMAGE		4
#define MODE_INIT		5
unsigned short keyma[8] = { 0xfb4f, 0xf76d, 0xef3f, 0xdf3f, 0xfb66, 0xf77f, 0xef3f, 0xdf3f };//7segment

typedef struct{
	int sundae;
	int ramen;
	int totPrice;
}Table;
Table table[9];

static unsigned short gamma[64] =
{
	0xB8,
	0x02, 0x03, 0x04, 0x05,
	0x06, 0x07, 0x08, 0x09,
	0x0A, 0x0B, 0x0C, 0x0D,
	0x0E, 0x0F, 0x10, 0x11,
	0x12, 0x13, 0x15, 0x17,
	0x19, 0x1B, 0x1D, 0x1F,
	0x21, 0x23, 0x25, 0x27,
	0x2A, 0x2D, 0x30, 0x33,
	0x36, 0x39, 0x3C, 0x3F,
	0x42, 0x45, 0x48, 0x4C,
	0x50, 0x54, 0x58, 0x5C,
	0x60, 0x64, 0x68, 0x6C,
	0x70, 0x74, 0x78, 0x7D,
	0x82, 0x87, 0x8C, 0x91,
	0x96, 0x9B, 0xA0, 0xA5,
	0xAA, 0xAF, 0xB4

};

int fda = 0;
int fdb = 0;
int fdc = 0;
int fdd = 0;
int fde = 0;
int fdf = 0;
int totprice = 0;
unsigned short dip_result[2][5] = {
	{ 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0 }
};
void delay(clock_t);
void move_num(int);
const unsigned short NumData[10][5] =
{
	{ 0xfe00, 0xfd7f, 0xfb41, 0xf77f, 0xef00 },//0
	{ 0xfe00, 0xfd42, 0xfb7f, 0xf740, 0xef00 },//1
	{ 0xfe00, 0xfd79, 0xfb49, 0xf74f, 0xef00 },//2
	{ 0xfe00, 0xfd49, 0xfb49, 0xf77f, 0xef00 },//3
	{ 0xfe00, 0xfd0f, 0xfb08, 0xf77f, 0xef00 },//4
	{ 0xfe00, 0xfd4f, 0xfb49, 0xf779, 0xef00 },//5
	{ 0xfe00, 0xfd7f, 0xfb49, 0xf779, 0xef00 },//6
	{ 0xfe00, 0xfd07, 0xfb01, 0xf77f, 0xef00 },//7
	{ 0xfe00, 0xfd7f, 0xfb49, 0xf77f, 0xef00 },//8
	{ 0xfe00, 0xfd4f, 0xfb49, 0xf77f, 0xef00 }//9
};
static int Mode;
int ArduinoStat = 0;	// 터치 상태
int mode = 1;			// 1:카트 보내기 2:주문확인 3:테이블 비우기
int lockScreen = 0;		// 화면 잠금
unsigned short result[6] = { 0x1234, 0, 0, 0, 0, 0 };
unsigned int set[6] = { 0, 0, 0, 0, 0, 0 };
unsigned short finish= 0;

#define  MAX_BUFF	200

int		screen_width;
int		screen_height;
int		bits_per_pixel;
int		line_length;
int		touchState = 0;

#define MAX_TOUCH_X	0x740
#define MAX_TOUCH_Y	0x540
void usage(void)
{
	printf("====================================================\n");
	printf("\nUsage: ./bitmap [FILE.bmp]\n");
	printf("====================================================\n");
}

// 비트맵 이미지를 LCD 패널 위에 띄우는 함수
void read_bmp(char *filename, char **pDib, char **data, int *cols, int *rows)
{
	BITMAPFILEHEADER    bmpHeader;
	BITMAPINFOHEADER    *bmpInfoHeader;
	unsigned int    size;
	unsigned char   magicNum[2];
	int     nread;
	FILE    *fp;

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("ERROR\n");
		return;
	}

	// identify bmp file
	magicNum[0] = fgetc(fp);
	magicNum[1] = fgetc(fp);
//	printf("magicNum : %c%c\n", magicNum[0], magicNum[1]);

	if (magicNum[0] != 'B' && magicNum[1] != 'M') {
		printf("It's not a bmp file!\n");
		fclose(fp);
		return;
	}

	nread = fread(&bmpHeader.bfSize, 1, sizeof(BITMAPFILEHEADER), fp);
	size = bmpHeader.bfSize - sizeof(BITMAPFILEHEADER);
	*pDib = (unsigned char *)malloc(size);      // DIB Header(Image Header)
	fread(*pDib, 1, size, fp);
	bmpInfoHeader = (BITMAPINFOHEADER *)*pDib;

//	printf("nread : %d\n", nread);
//	printf("size : %d\n", size);

	// check 24bit
	if (BIT_VALUE_24BIT != (bmpInfoHeader->biBitCount))     // bit value
	{
		printf("It supports only 24bit bmp!\n");
		fclose(fp);
		return;
	}

	*cols = bmpInfoHeader->biWidth;
	*rows = bmpInfoHeader->biHeight;
	*data = (char *)(*pDib + bmpHeader.bfOffBits - sizeof(bmpHeader)-2);
	fclose(fp);
}

void close_bmp(char **pDib)     // DIB(Device Independent Bitmap)
{
	free(*pDib);
}

// 터치한 점 좌표 반환
void readFirstCoordinate(int fd, int* cx, int* cy)
{
	struct input_event event;
	int readSize;

	while (1)
	{
		readSize = read(fd, &event, sizeof(event));

		if (readSize == sizeof(event))
		{
			//		printf("type :%04X \n",event.type);
			//		printf("code :%04X \n",event.code);
			//		printf("value:%08X \n",event.value);
			if (event.type == EV_ABS)
			{
				//printf("haha1\n");
				if (event.code == ABS_MT_POSITION_X)
				{
					//printf("haha2\n");
					*cx = event.value*screen_width / MAX_TOUCH_X;
				}
				else if (event.code == ABS_MT_POSITION_Y)
				{
					//	printf("haha3\n");
					*cy = event.value*screen_height / MAX_TOUCH_Y;
				}
			}
			else if ((event.type == EV_SYN) && (event.code == SYN_REPORT))
			{
				//	printf("hasha");
				break;
			}

		}
		//		printf("\n");
	}
}

void initScreen(unsigned char *fb_mem)
{
	int		coor_y;
	int		coor_x;
	unsigned long *ptr;

	for (coor_y = 0; coor_y < screen_height; coor_y++)
	{
		ptr = (unsigned long *)fb_mem + screen_width * coor_y;
		for (coor_x = 0; coor_x < screen_width; coor_x++)
		{
			*ptr++ = 0x000000;
		}
	}
}

void DieWithError(char *errorMessage)
{
	perror(errorMessage);
	exit(1);
}

int AcceptTCPConnection(int servSock)
{
	int clntSock;                    /* Socket descriptor for client */
	struct sockaddr_in echoClntAddr; /* Client address */
	unsigned int clntLen;            /* Length of client address data structure */

	/* Set the size of the in-out parameter */
	clntLen = sizeof(echoClntAddr);

	/* Wait for a client to connect */
	if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr,
		&clntLen)) < 0)
		DieWithError("accept() failed");

	/* clntSock is connected to a client! */

	printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

	return clntSock;
}

// 데이터 처리
int HandleTCPClient(int clntSocket)
{
	char echoBuffer[RCVBUFSIZE];        /* Buffer for echo string */
	//char test[10]="1234";
	char test2[2] = "x";
	int recvMsgSize;                    /* Size of received message */
	int i = 0;

	while (1)
	{
		recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0);//아두이노 클라이언트에서 send를 보내면 이친구가 read모드인데 읽어옴
		
	// 아두이노에 보낼 메시지 설정
		if (ArduinoStat == 1 && mode==1){
			strcpy(test2, "a");
		}
		else if (ArduinoStat == 2 && mode ==1){
			strcpy(test2, "b");
		}
		else if (ArduinoStat == 3 && mode == 1){
			strcpy(test2, "c");
		}
		else if (ArduinoStat == 4 && mode == 1){
			strcpy(test2, "d");
		}
		else if (ArduinoStat == 5 && mode == 1){
			strcpy(test2, "e");
		}
		else if (ArduinoStat == 6 && mode == 1){
			strcpy(test2, "f");
		}
		else if (ArduinoStat == 7 && mode == 1){
			strcpy(test2, "g");
		}
		else if (ArduinoStat == 8 && mode == 1){
			strcpy(test2, "h");
		}
		else if (ArduinoStat == 9 && mode == 1){
			strcpy(test2, "i");
		}
		else strcpy(test2, "x");

		//if echobuffer 만져서 test문에 할당
		send(clntSocket, test2, recvMsgSize, 0);
		sleep(3);
		// 나갈조건설정
		
		printf("%s\n", echoBuffer);
	}
	return 0;
}

int CreateTCPServerSocket(unsigned short port)
{
	int sock;                        /* socket to create */
	struct sockaddr_in echoServAddr; /* Local address */

	/* Create socket for incoming connections */
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed");

	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
	echoServAddr.sin_family = AF_INET;                /* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	echoServAddr.sin_port = htons(port);              /* Local port */

	/* Bind to the local address */
	if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("bind() failed");

	/* Mark the socket so it will listen for incoming connections */
	if (listen(sock, MAXPENDING) < 0)
		DieWithError("listen() failed");

	return sock;
}

int IsBusy(void)
{
	unsigned short wdata, rdata;

	wdata = SIG_BIT_RW;
	write(fde, &wdata, 2);

	wdata = SIG_BIT_RW | SIG_BIT_E;
	write(fde, &wdata, 2);

	read(fde, &rdata, 2);

	wdata = SIG_BIT_RW;
	write(fde, &wdata, 2);

	if (rdata &  BUSY_BIT)
		return TRUE;

	return FALSE;
}

int writecmd2(unsigned short cmd)
{
	unsigned short wdata;

	if (IsBusy())
		return FALSE;

	wdata = cmd;
	write(fde, &wdata, 2);

	wdata = cmd | SIG_BIT_E;
	write(fde, &wdata, 2);

	wdata = cmd;
	write(fde, &wdata, 2);

	return TRUE;
}

int setDDRAMAddr(int x, int y)
{
	unsigned short cmd = 0;
	//	printf("x :%d , y:%d \n",x,y);
	if (IsBusy())
	{
		perror("setDDRAMAddr busy error.\n");
		return FALSE;

	}

	if (y == 1)
	{
		cmd = DDRAM_ADDR_LINE_1 + x;
	}
	else if (y == 2)
	{
		cmd = DDRAM_ADDR_LINE_2 + x;
	}
	else
		return FALSE;

	if (cmd >= 0x80)
		return FALSE;


	//	printf("setDDRAMAddr w1 :0x%X\n",cmd);

	if (!writecmd2(cmd | SET_DDRAM_ADD_DEF))
	{
		perror("setDDRAMAddr error\n");
		return FALSE;
	}
	//	printf("setDDRAMAddr w :0x%X\n",cmd|SET_DDRAM_ADD_DEF);
	usleep(1000);
	return TRUE;
}

int displayMode(int bCursor, int bCursorblink, int blcd)
{
	unsigned short cmd = 0;

	if (bCursor)
	{
		cmd = DIS_CURSOR;
	}

	if (bCursorblink)
	{
		cmd |= DIS_CUR_BLINK;
	}

	if (blcd)
	{
		cmd |= DIS_LCD;
	}

	if (!writecmd2(cmd | DIS_DEF))
		return FALSE;

	return TRUE;
}

int writeCh(unsigned short ch)
{
	unsigned short wdata = 0;

	if (IsBusy())
		return FALSE;

	wdata = SIG_BIT_RS | ch;
	write(fde, &wdata, 2);

	wdata = SIG_BIT_RS | ch | SIG_BIT_E;
	write(fde, &wdata, 2);

	wdata = SIG_BIT_RS | ch;
	write(fde, &wdata, 2);
	usleep(1000);
	return TRUE;

}

int setCursorMode(int bMove, int bRightDir)
{
	unsigned short cmd = MODE_SET_DEF;

	if (bMove)
		cmd |= MODE_SET_SHIFT;

	if (bRightDir)
		cmd |= MODE_SET_DIR_RIGHT;

	if (!writecmd2(cmd))
		return FALSE;
	return TRUE;
}

int functionSet(void)
{
	unsigned short cmd = 0x0038; // 5*8 dot charater , 8bit interface , 2 line

	if (!writecmd2(cmd))
		return FALSE;
	return TRUE;
}

int writeStr(char* str)
{
	unsigned char wdata;
	int i;
	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] == '_')
			wdata = (unsigned char)' ';
		else
			wdata = str[i];
		writeCh(wdata);
	}
	return TRUE;

}
	
int clearScreen(int nline)
{
	int i;
	if (nline == 0)
	{
		if (IsBusy())
		{
			perror("clearScreen error\n");
			return FALSE;
		}
		if (!writecmd2(CLEAR_DISPLAY))
			return FALSE;
		return TRUE;
	}
	else if (nline == 1)
	{
		setDDRAMAddr(0, 1);
		for (i = 0; i <= COLUMN_NUM; i++)
		{
			writeCh((unsigned char)' ');
		}
		setDDRAMAddr(0, 1);

	}
	else if (nline == 2)
	{
		setDDRAMAddr(0, 2);
		for (i = 0; i <= COLUMN_NUM; i++)
		{
			writeCh((unsigned char)' ');
		}
		setDDRAMAddr(0, 2);
	}
	return TRUE;
}

void dohelp2(void)
{
	printf("Usage:\n");
	printf("tlcdtest w line string :=>display the string  at line  , charater  '_' =>' '\n");
	printf(" ex) tlcdtest w 0 cndi_text_test :=>display 'cndi text test' at line 1 \n");
	printf("tlcdtest c on|off blink line column : \n");
	printf(" => cursor set on|off =>1 or 0 , b => blink 1|0 , line column line position \n");
	printf("tlcdtset c  1 1 2 12  :=> display blink cursor at 2 line , 12 column.\n");
	printf("tlcdtest r [line] : => clear screen or clear line \n");
	printf("tlcdtest r  : => clear screen \n");
	printf("tlcdtest r 1: => clear line 1 \n");
}

void delay(clock_t n){
	clock_t start = clock();
	while(clock() - start<n);
}

void showmat(int dip){
move_num(dip);
write(fdb, &dip_result[0], 20);
usleep(100);
}

void move_num(int dipvalue)
{
	if(dipvalue==0){

				dip_result[0][0] = NumData[0][0];
				dip_result[0][1] = NumData[0][1];
				dip_result[0][2] = NumData[0][2];
				dip_result[0][3] = NumData[0][3];
				dip_result[0][4] = NumData[0][4];

				dip_result[1][0] = NumData[0][0];
				dip_result[1][1] = NumData[0][1];
				dip_result[1][2] = NumData[0][2];
				dip_result[1][3] = NumData[0][3];
				dip_result[1][4] = NumData[0][4];

	}else if(dipvalue==1){

				dip_result[0][0] = NumData[0][0];
                dip_result[0][1] = NumData[0][1];
                dip_result[0][2] = NumData[0][2];
                dip_result[0][3] = NumData[0][3];
                dip_result[0][4] = NumData[0][4];

                dip_result[1][0] = NumData[1][0];
                dip_result[1][1] = NumData[1][1];
                dip_result[1][2] = NumData[1][2];
                dip_result[1][3] = NumData[1][3];
                dip_result[1][4] = NumData[1][4];

	}else if(dipvalue==2){

				dip_result[0][0] = NumData[0][0];
                dip_result[0][1] = NumData[0][1];
                dip_result[0][2] = NumData[0][2];
                dip_result[0][3] = NumData[0][3];
                dip_result[0][4] = NumData[0][4];

                dip_result[1][0] = NumData[2][0];
                dip_result[1][1] = NumData[2][1];
                dip_result[1][2] = NumData[2][2];
                dip_result[1][3] = NumData[2][3];
                dip_result[1][4] = NumData[2][4];

	}else if(dipvalue==3){

				dip_result[0][0] = NumData[0][0];
                dip_result[0][1] = NumData[0][1];
                dip_result[0][2] = NumData[0][2];
                dip_result[0][3] = NumData[0][3];
                dip_result[0][4] = NumData[0][4];

                dip_result[1][0] = NumData[3][0];
                dip_result[1][1] = NumData[3][1];
                dip_result[1][2] = NumData[3][2];
                dip_result[1][3] = NumData[3][3];
                dip_result[1][4] = NumData[3][4];


	}else if(dipvalue==4){

				dip_result[0][0] = NumData[0][0];
                dip_result[0][1] = NumData[0][1];
                dip_result[0][2] = NumData[0][2];
                dip_result[0][3] = NumData[0][3];
                dip_result[0][4] = NumData[0][4];

                dip_result[1][0] = NumData[4][0];
                dip_result[1][1] = NumData[4][1];
                dip_result[1][2] = NumData[4][2];
                dip_result[1][3] = NumData[4][3];
                dip_result[1][4] = NumData[4][4];

	}else if(dipvalue==5){
				dip_result[0][0] = NumData[0][0];
                dip_result[0][1] = NumData[0][1];
                dip_result[0][2] = NumData[0][2];
                dip_result[0][3] = NumData[0][3];
                dip_result[0][4] = NumData[0][4];

                dip_result[1][0] = NumData[5][0];
                dip_result[1][1] = NumData[5][1];
                dip_result[1][2] = NumData[5][2];
                dip_result[1][3] = NumData[5][3];
                dip_result[1][4] = NumData[5][4];

	}else if(dipvalue==6){

				dip_result[0][0] = NumData[0][0];
                dip_result[0][1] = NumData[0][1];
                dip_result[0][2] = NumData[0][2];
                dip_result[0][3] = NumData[0][3];
                dip_result[0][4] = NumData[0][4];

                dip_result[1][0] = NumData[6][0];
                dip_result[1][1] = NumData[6][1];
                dip_result[1][2] = NumData[6][2];
                dip_result[1][3] = NumData[6][3];
                dip_result[1][4] = NumData[6][4];

        }else if(dipvalue==7){

				dip_result[0][0] = NumData[0][0];
                dip_result[0][1] = NumData[0][1];
                dip_result[0][2] = NumData[0][2];
                dip_result[0][3] = NumData[0][3];
                dip_result[0][4] = NumData[0][4];

                dip_result[1][0] = NumData[7][0];
                dip_result[1][1] = NumData[7][1];
                dip_result[1][2] = NumData[7][2];
                dip_result[1][3] = NumData[7][3];
                dip_result[1][4] = NumData[7][4];

        }else if(dipvalue==8){

				dip_result[0][0] = NumData[0][0];
                dip_result[0][1] = NumData[0][1];
                dip_result[0][2] = NumData[0][2];
                dip_result[0][3] = NumData[0][3];
                dip_result[0][4] = NumData[0][4];

                dip_result[1][0] = NumData[8][0];
                dip_result[1][1] = NumData[8][1];
                dip_result[1][2] = NumData[8][2];
                dip_result[1][3] = NumData[8][3];
                dip_result[1][4] = NumData[8][4];


        }else if(dipvalue==9){

				dip_result[0][0] = NumData[0][0];
                dip_result[0][1] = NumData[0][1];
                dip_result[0][2] = NumData[0][2];
                dip_result[0][3] = NumData[0][3];
                dip_result[0][4] = NumData[0][4];

                dip_result[1][0] = NumData[9][0];
                dip_result[1][1] = NumData[9][1];
                dip_result[1][2] = NumData[9][2];
                dip_result[1][3] = NumData[9][3];
                dip_result[1][4] = NumData[9][4];

        }else if(dipvalue==10){

                dip_result[0][0] = NumData[1][0];
                dip_result[0][1] = NumData[1][1];
                dip_result[0][2] = NumData[1][2];
                dip_result[0][3] = NumData[1][3];
                dip_result[0][4] = NumData[1][4];

                dip_result[1][0] = NumData[0][0];
                dip_result[1][1] = NumData[0][1];
                dip_result[1][2] = NumData[0][2];
                dip_result[1][3] = NumData[0][3];
                dip_result[1][4] = NumData[0][4];

        }else if(dipvalue==11){

                dip_result[0][0] = NumData[1][0];
                dip_result[0][1] = NumData[1][1];
                dip_result[0][2] = NumData[1][2];
                dip_result[0][3] = NumData[1][3];
                dip_result[0][4] = NumData[1][4];

                dip_result[1][0] = NumData[1][0];
                dip_result[1][1] = NumData[1][1];
                dip_result[1][2] = NumData[1][2];
                dip_result[1][3] = NumData[1][3];
                dip_result[1][4] = NumData[1][4];

        }else if(dipvalue==12){

                dip_result[0][0] = NumData[1][0];
                dip_result[0][1] = NumData[1][1];
                dip_result[0][2] = NumData[1][2];
                dip_result[0][3] = NumData[1][3];
                dip_result[0][4] = NumData[1][4];

                dip_result[1][0] = NumData[2][0];
                dip_result[1][1] = NumData[2][1];
                dip_result[1][2] = NumData[2][2];
                dip_result[1][3] = NumData[2][3];
                dip_result[1][4] = NumData[2][4];

        }else if(dipvalue==13){

                dip_result[0][0] = NumData[1][0];
                dip_result[0][1] = NumData[1][1];
                dip_result[0][2] = NumData[1][2];
                dip_result[0][3] = NumData[1][3];
                dip_result[0][4] = NumData[1][4];

                dip_result[1][0] = NumData[3][0];
                dip_result[1][1] = NumData[3][1];
                dip_result[1][2] = NumData[3][2];
                dip_result[1][3] = NumData[3][3];
                dip_result[1][4] = NumData[3][4];

        }else if(dipvalue==14){

                dip_result[0][0] = NumData[1][0];
                dip_result[0][1] = NumData[1][1];
                dip_result[0][2] = NumData[1][2];
                dip_result[0][3] = NumData[1][3];
                dip_result[0][4] = NumData[1][4];

                dip_result[1][0] = NumData[4][0];
                dip_result[1][1] = NumData[4][1];
                dip_result[1][2] = NumData[4][2];
                dip_result[1][3] = NumData[4][3];
                dip_result[1][4] = NumData[4][4];

        }else if(dipvalue==15){

                dip_result[0][0] = 0xfe03;
                dip_result[0][1] = 0xfd0d;
                dip_result[0][2] = 0xfb1a;
                dip_result[0][3] = 0xf722;
                dip_result[0][4] = 0xef44;

                dip_result[1][0] = 0xfe44;
                dip_result[1][1] = 0xfd22;
                dip_result[1][2] = 0xfb1a;
                dip_result[1][3] = 0xf70d;
                dip_result[1][4] = 0xef03;

        }


}
///////////////////////////////oled////////////////////////////
void doHelp(void)
{
	printf("Usage:\n");
	printf("oledtest w d1 [d2] .... :=> write data \n");
	printf("oledtest r readnum  :=> read data \n");
	printf("oledtest c cmd [sub1] [sub2] .... :cmd set\n");
	printf("oledtest t :=> reset \n");
	printf("oledtest i :=> init \n");
	printf("oledtest d file(.img):=> loading image file\n");
}

unsigned long simple_strtoul(char *cp, char **endp, unsigned int base)
{
	unsigned long result = 0, value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp - '0' : (islower(*cp)
		? toupper(*cp) : *cp) - 'A' + 10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

unsigned long read_hex(const char* str){
	char addr[128];
	strcpy(addr, str);
	return simple_strtoul(addr, NULL, 16);
}

// signal form 
//	12bit	11bit	10bit	9bit	8bit	7bit	6bit	5bit	4bit	3bit	2bit	1bit	0bit
//	RST#	CS#		D/C#	WD#		RD#		D7		D6		D5		D4		D3		D2		D1		D0
// trigger => WD or RD rising edge
/************************************************************************************************
************************************************************************************************/

int reset(void)
{
	unsigned short wdata;

	wdata = RST_BIT_MASK;
	write(fdc, &wdata, 2);
	usleep(2000);
	wdata = DEFAULT_MASK;
	write(fdc, &wdata, 2);
	return TRUE;
}

int writeCmd(int size, unsigned short* cmdArr)
{
	int i;
	unsigned short wdata;

	wdata = CS_BIT_MASK & DC_BIT_MASK;
	write(fdc, &wdata, 2);

	wdata = CS_BIT_MASK & DC_BIT_MASK & WD_BIT_MASK;
	write(fdc, &wdata, 2);

	wdata = CS_BIT_MASK & DC_BIT_MASK & WD_BIT_MASK & (cmdArr[0] | 0xFF00);
	write(fdc, &wdata, 2);

	wdata = CS_BIT_MASK & DC_BIT_MASK & (cmdArr[0] | 0xFF00);
	write(fdc, &wdata, 2);

	wdata = CS_BIT_MASK & (cmdArr[0] | 0xFF00);
	write(fdc, &wdata, 2);

	for (i = 1; i < size; i++)
	{
		wdata = CS_BIT_MASK & WD_BIT_MASK;
		write(fdc, &wdata, 2);

		wdata = CS_BIT_MASK & WD_BIT_MASK & (cmdArr[i] | 0xFF00);
		write(fdc, &wdata, 2);

		wdata = CS_BIT_MASK & (cmdArr[i] | 0xFF00);
		write(fdc, &wdata, 2);

		//	wdata = CS_BIT_MASK & (cmdArr[i] | 0xFF00);
		//	write(fd,&wdata,2);
		//	printf("[0x%02X]",cmdArr[i]);

	}
	wdata = DEFAULT_MASK;
	write(fdc, &wdata, 2);
	//printf("\n");
	return TRUE;
}

int writeData(int size, unsigned char* dataArr)
{
	int i;
	unsigned short wdata;

	//wdata = CS_BIT_MASK;
	//write(fd,&wdata,2);

	wdata = CS_BIT_MASK & DC_BIT_MASK;
	write(fdc, &wdata, 2);

	//wdata = CS_BIT_MASK & DC_BIT_MASK & WD_BIT_MASK ;
	//write(fd,&wdata,2);

	wdata = CS_BIT_MASK & DC_BIT_MASK & WD_BIT_MASK & (CMD_WRITE_RAM | 0xFF00);
	write(fdc, &wdata, 2);

	wdata = CS_BIT_MASK & DC_BIT_MASK & (CMD_WRITE_RAM | 0xFF00);
	write(fdc, &wdata, 2);

	wdata = CS_BIT_MASK &  (CMD_WRITE_RAM | 0xFF00);
	write(fdc, &wdata, 2);

	for (i = 0; i < size; i++)
	{
		wdata = CS_BIT_MASK & WD_BIT_MASK;
		write(fdc, &wdata, 2);

		wdata = CS_BIT_MASK & WD_BIT_MASK & ((unsigned char)dataArr[i] | 0xFF00);
		write(fdc, &wdata, 2);

		wdata = CS_BIT_MASK & ((unsigned char)dataArr[i] | 0xFF00);
		write(fdc, &wdata, 2);


	}
	wdata = DEFAULT_MASK;
	write(fdc, &wdata, 2);

	return TRUE;

}

int readData(int size, unsigned short* dataArr)
{

	int i;
	unsigned short wdata;

	wdata = CS_BIT_MASK & DC_BIT_MASK;
	write(fdc, &wdata, 2);

	wdata = CS_BIT_MASK & DC_BIT_MASK & (CMD_READ_RAM | 0xFF00);
	write(fdc, &wdata, 2);

	wdata = CS_BIT_MASK & DC_BIT_MASK & WD_BIT_MASK &(CMD_READ_RAM | 0xFF00);
	write(fdc, &wdata, 2);

	wdata = CS_BIT_MASK & DC_BIT_MASK & (CMD_READ_RAM | 0xFF00);
	write(fdc, &wdata, 2);

	wdata = CS_BIT_MASK &  (CMD_READ_RAM | 0xFF00);
	write(fdc, &wdata, 2);


	for (i = 0; i < size; i++)
	{
		//wdata = CS_BIT_MASK ;
		//write(fd,&wdata,2);

		wdata = CS_BIT_MASK;
		write(fdc, &wdata, 2);

		wdata = CS_BIT_MASK & RD_BIT_MASK;
		write(fdc, &wdata, 2);

		wdata = CS_BIT_MASK & RD_BIT_MASK;
		write(fdc, &wdata, 2);
		
		wdata = CS_BIT_MASK;
		write(fdc, &wdata, 2);

		read(fdc, &dataArr[i], 2);

		//wdata = CS_BIT_MASK ;
		//write(fd,&wdata,2);

	}
	wdata = DEFAULT_MASK;
	write(fdc, &wdata, 2);

	return TRUE;
}

int setAddressDefalut(void)
{
	unsigned short  cmd[3];
	cmd[0] = CMD_SET_COLUMN_ADDR;
	cmd[1] = 0;
	cmd[2] = 127;
	writeCmd(3, cmd);

	cmd[0] = CMD_SET_ROW_ADDR;
	cmd[1] = 0;
	cmd[2] = 127;
	writeCmd(3, cmd);

	return TRUE;
}

// to send cmd  , must unlock
int setCmdLock(int bLock)
{
	unsigned short  cmd[3];

	cmd[0] = CMD_LOCK;
	if (bLock)
	{
		cmd[1] = 0x16; // lock
		writeCmd(2, cmd);

	}
	else
	{
		cmd[1] = 0x12; // lock
		writeCmd(2, cmd);

		// A2,B1,B3,BB,BE accessible
		cmd[1] = 0xB1;
		writeCmd(2, cmd);
	}
	return TRUE;
}

int imageLoading(char* fileName)
{
	int imgfile;
	unsigned char* data = NULL;
	int  width, height;

	imgfile = open(fileName, O_RDONLY);
	if (imgfile < 0)
	{
		printf("imageloading(%s)  file is not exist . err.\n", fileName);
		return FALSE;
	}
	setCmdLock(FALSE);

	read(imgfile, &width, sizeof(unsigned char));
	read(imgfile, &height, sizeof(unsigned char));

	data = malloc(128 * 128 * 3);

	read(imgfile, data, 128 * 128 * 3);

	close(imgfile);

	writeData(128 * 128 * 3, data);

	setCmdLock(TRUE);
	return TRUE;
}

int Init(void)
{
	unsigned short wdata[10];
	unsigned char  wcdata[10];
	int i, j;
	wdata[0] = 0xFD;
	wdata[1] = 0x12;
	writeCmd(2, wdata);


	wdata[0] = 0xFD;
	wdata[1] = 0xB1;
	writeCmd(2, wdata);

	wdata[0] = 0xAE;
	writeCmd(1, wdata);

	wdata[0] = 0xB3;
	wdata[1] = 0xF1;
	writeCmd(2, wdata);

	wdata[0] = 0xCA;
	wdata[1] = 0x7F;
	writeCmd(2, wdata);

	wdata[0] = 0xA2;
	wdata[1] = 0x00;
	writeCmd(2, wdata);

	wdata[0] = 0xA1;
	wdata[1] = 0x00;
	writeCmd(2, wdata);

	wdata[0] = 0xA0;
	wdata[1] = 0xB4;
	writeCmd(2, wdata);

	wdata[0] = 0xAB;
	wdata[1] = 0x01;
	writeCmd(2, wdata);

	wdata[0] = 0xB4;
	wdata[1] = 0xA0;
	wdata[2] = 0xB5;
	wdata[3] = 0x55;
	writeCmd(4, wdata);

	wdata[0] = 0xC1;
	wdata[1] = 0xC8;
	wdata[2] = 0x80;
	wdata[3] = 0xC8;
	writeCmd(4, wdata);

	wdata[0] = 0xC7;
	wdata[1] = 0x0F;
	writeCmd(2, wdata);

	// gamma setting 
	writeCmd(64, gamma);


	wdata[0] = 0xB1;
	wdata[1] = 0x32;
	writeCmd(2, wdata);

	wdata[0] = 0xB2;
	wdata[1] = 0xA4;
	wdata[2] = 0x00;
	wdata[3] = 0x00;
	writeCmd(4, wdata);

	wdata[0] = 0xBB;
	wdata[1] = 0x17;
	writeCmd(2, wdata);

	wdata[0] = 0xB6;
	wdata[1] = 0x01;
	writeCmd(2, wdata);

	wdata[0] = 0xBE;
	wdata[1] = 0x05;
	writeCmd(2, wdata);

	wdata[0] = 0xA6;
	writeCmd(1, wdata);


	for (i = 0; i < 128; i++)
	{
		for (j = 0; j < 128; j++)
		{
			wcdata[0] = 0x3F;
			wcdata[1] = 0;
			wcdata[2] = 0;
			writeData(3, wcdata);
		}
	}
	wdata[0] = 0xAF;
	writeCmd(1, wdata);
	return TRUE;
}

void show_tottlcd(int tk){
	int sun = table[tk - 1].sundae;
	int ram = table[tk - 1].ramen;
	char s1[10];
	char s2[10];
	int nCmdMode;
	int bCursorOn, bBlink, nline, nColumn;
	char strWtext[COLUMN_NUM + 1];
	char strWtext3[COLUMN_NUM + 1];
	char strWtext1[12] = "sundae : ";
	char strWtext2[12] = "ramen : ";

	sprintf(s1, "%d", sun);
	sprintf(s2, "%d", ram);
	strcpy(strWtext, strWtext1);
	strcat(strWtext, s1);
	strcpy(strWtext3, strWtext2);
	strcat(strWtext3, s2);

	bCursorOn = 1;
	bBlink = 1;
	nline = 2;
	nColumn = 12;
	displayMode(bCursorOn, bBlink, TRUE);
	setDDRAMAddr(nline - 1, nColumn);
	displayMode(bCursorOn, bBlink, TRUE);
	setDDRAMAddr(nline - 1, nColumn);

	nline = 1;
	nColumn = 0;
	setDDRAMAddr(nColumn, nline);
	usleep(2000);
	writeStr(strWtext);
	setDDRAMAddr(nColumn, nline+1);
	usleep(2000);
	writeStr(strWtext3);

}

void show7seg(int key, int dip){
	int jj = 0;
	int kk = 0;
	if (key == 0x01){	//keypad ==0x01
	for (jj = 0; jj<3000; jj++){
	for (kk = 0; kk<4; kk++){
	write(fdd, &keyma[kk], 2);
	usleep(100);
	}
	}
	table[dip - 1].sundae++;
	table[dip - 1].totPrice += 3500;
	} //keypad ==0x01
	else if (key == 0x02){
	for (jj = 0; jj<3000; jj++){
	for (kk = 4; kk<8; kk++){
	write(fdd, &keyma[kk], 2);
	usleep(100);
	}
	}table[dip - 1].ramen++;
	table[dip - 1].totPrice += 4800;
	}

}
void show7seg2(int key){
	
		int jj = 0;
		int kk = 0;
		if (key == 0x01){	//keypad ==0x01
			for (jj = 0; jj < 3000; jj++){
				for (kk = 0; kk < 4; kk++){
					write(fdd, &keyma[kk], 2);
					usleep(100);
				}
			}
			//totprice += 3500;
		} //keypad ==0x01
		else if (key == 0x02){
			for (jj = 0; jj < 3000; jj++){
				for (kk = 4; kk < 8; kk++){
					write(fdd, &keyma[kk], 2);
					usleep(100);
				}
			}//totprice += 4800;
		}
		sleep(1);
	//	ArduinoStat = 0;
	//	lockScreen = 0;
}
void showoled(int key){
	Mode = MODE_IMAGE;
	if (key == 0x01)
		imageLoading("sundae.jpg.img");
	else if (key == 0x02)
		imageLoading("ramen.jpg.img");
}
/////////////////oled//////////////////////////
void showtlcd(int key){
	int nCmdMode;
	int bCursorOn, bBlink, nline, nColumn;
	char strWtext[COLUMN_NUM + 1];
	char *strWtext1 = "sundae_choiced";
	char *strWtext2 = "ramen_choiced";
	if (key == 0x01)
		strcpy(strWtext, strWtext1);
	else if (key == 0x02)
		strcpy(strWtext, strWtext2);
	bCursorOn = 1;
	bBlink = 1;
	nline = 2;
	nColumn = 12;
	displayMode(bCursorOn, bBlink, TRUE);
	setDDRAMAddr(nline - 1, nColumn); 
	displayMode(bCursorOn, bBlink, TRUE);
	setDDRAMAddr(nline - 1, nColumn);
	
	nline = 1;
	nColumn = 0;
	setDDRAMAddr(nColumn, nline);
	usleep(2000);
	writeStr(strWtext);
}
void showtlcd2(int key){
	int nCmdMode;
	int bCursorOn, bBlink, nline, nColumn;
	char strWtext[COLUMN_NUM + 1];
	char *strWtext1 = "table_1_clear";
	char *strWtext2 = "table_2_clear";
	char *strWtext3 = "table_3_clear";
	char *strWtext4 = "table_4_clear";
	char *strWtext5 = "table_5_clear";
	char *strWtext6 = "table_6_clear";

	char *strWtext7 = "table_7_clear";
	char *strWtext8 = "table_8_clear";
	char *strWtext9 = "table_9_clear";
	if (key == 1)
		strcpy(strWtext, strWtext1);
	else if (key == 2)
		strcpy(strWtext, strWtext2);
	else if (key == 3)
		strcpy(strWtext, strWtext3);
	else if (key == 4)
		strcpy(strWtext, strWtext4);
	else if (key == 5)
		strcpy(strWtext, strWtext5);
	else if (key == 6)
		strcpy(strWtext, strWtext6);
	else if (key == 7)
		strcpy(strWtext, strWtext7);
	else if (key == 8)
		strcpy(strWtext, strWtext8);
	else if (key == 9)
		strcpy(strWtext, strWtext9);
	bCursorOn = 1;
	bBlink = 1;
	nline = 2;
	nColumn = 12;
	displayMode(bCursorOn, bBlink, TRUE);
	setDDRAMAddr(nline - 1, nColumn);
	displayMode(bCursorOn, bBlink, TRUE);
	setDDRAMAddr(nline - 1, nColumn);

	nline = 1;
	nColumn = 0;
	setDDRAMAddr(nColumn, nline);
	usleep(2000);
	writeStr(strWtext);
	sleep(3);
	clearScreen(0);
}
void show_totprice(int dip){
	int count1 = table[dip-1].totPrice;
	set[0] = count1 / 100000;
	count1 %= 100000;
	if (set[0] == 0){
		result[0] = 0xfe3f;
	}
	else if (set[0] == 1){
		result[0] = 0xfe06;
	}
	else if (set[0] == 2){
		result[0] = 0xfe5b;
	}
	else if (set[0] == 3){
		result[0] = 0xfe4f;
	}
	else if (set[0] == 4){
		result[0] = 0xfe66;
	}
	else if (set[0] == 5){
		result[0] = 0xfe6d;
	}
	else if (set[0] == 6){
		result[0] = 0xfe7d;
	}
	else if (set[0] == 7){
		result[0] = 0xfe27;
	}
	else if (set[0] == 8){
		result[0] = 0xfe7f;
	}
	else if (set[0] == 9){
		result[0] = 0xfe67;
	}

	set[1] = count1 / 10000;
	count1 %= 10000;
	if (set[1] == 0){
		result[1] = 0xfd3f;
	}
	else if (set[1] == 1){
		result[1] = 0xfd06;
	}
	else if (set[1] == 2){
		result[1] = 0xfd5b;
	}
	else if (set[1] == 3){
		result[1] = 0xfd4f;
	}
	else if (set[1] == 4){
		result[1] = 0xfd66;
	}
	else if (set[1] == 5){
		result[1] = 0xfd6d;
	}
	else if (set[1] == 6){
		result[1] = 0xfd7d;
	}
	else if (set[1] == 7){
		result[1] = 0xfd27;
	}
	else if (set[1] == 8){
		result[1] = 0xfd7f;
	}
	else if (set[1] == 9){
		result[1] = 0xfd67;
	}


	set[2] = count1 / 1000;
	count1 %= 1000;
	if (set[2] == 0){
		result[2] = 0xfb3f;
	}
	else if (set[2] == 1){
		result[2] = 0xfb06;
	}
	else if (set[2] == 2){
		result[2] = 0xfb5b;
	}
	else if (set[2] == 3){
		result[2] = 0xfb4f;
	}
	else if (set[2] == 4){
		result[2] = 0xfb66;
	}
	else if (set[2] == 5){
		result[2] = 0xfb6d;
	}
	else if (set[2] == 6){
		result[2] = 0xfb7d;
	}
	else if (set[2] == 7){
		result[2] = 0xfb27;
	}
	else if (set[2] == 8){
		result[2] = 0xfb7f;
	}
	else if (set[2] == 9){
		result[2] = 0xfb67;
	}

	set[3] = count1 / 100;
	count1 %= 100;
	if (set[3] == 0){
		result[3] = 0xf73f;
	}
	else if (set[3] == 1){
		result[3] = 0xf706;
	}
	else if (set[3] == 2){
		result[3] = 0xf75b;
	}
	else if (set[3] == 3){
		result[3] = 0xf74f;
	}
	else if (set[3] == 4){
		result[3] = 0xf766;
	}
	else if (set[3] == 5){
		result[3] = 0xf76d;
	}
	else if (set[3] == 6){
		result[3] = 0xf77d;
	}
	else if (set[3] == 7){
		result[3] = 0xf727;
	}
	else if (set[3] == 8){
		result[3] = 0xf77f;
	}
	else if (set[3] == 9){
		result[3] = 0xf767;
	}


	set[4] = count1 / 10;
	count1 %= 10;
	if (set[4] == 0){
		result[4] = 0xef3f;
	}
	else if (set[4] == 1){
		result[4] = 0xef06;
	}
	else if (set[4] == 2){
		result[4] = 0xef5b;
	}
	else if (set[4] == 3){
		result[4] = 0xef4f;
	}
	else if (set[4] == 4){
		result[4] = 0xef66;
	}
	else if (set[4] == 5){
		result[4] = 0xef6d;
	}
	else if (set[4] == 6){
		result[4] = 0xef7d;
	}
	else if (set[4] == 7){
		result[4] = 0xef27;
	}
	else if (set[4] == 8){
		result[4] = 0xef7f;
	}
	else if (set[4] == 9){
		result[4] = 0xef67;
	}


	set[5] = count1 / 1;
	if (set[5] == 0){
		result[5] = 0xdf3f;
	}
	else if (set[5] == 1){
		result[5] = 0xdf06;
	}
	else if (set[5] == 2){
		result[5] = 0xdf5b;
	}
	else if (set[5] == 3){
		result[5] = 0xdf4f;
	}
	else if (set[5] == 4){
		result[5] = 0xdf66;
	}
	else if (set[5] == 5){
		result[5] = 0xdf6d;
	}
	else if (set[5] == 6){
		result[5] = 0xdf7d;
	}
	else if (set[5] == 7){
		result[5] = 0xdf27;
	}
	else if (set[5] == 8){
		result[5] = 0xdf7f;
	}
	else if (set[5] == 9){
		result[5] = 0xdf67;
	}
	int jjj;
	int iii;

	for (jjj = 0; jjj<1900; jjj++){
		for (iii = 0; iii<6; iii++){
			//write(fd, 0xfb66, 4);		
			write(fdd, &result[iii], 2);
			usleep(100);
		}
	}

	finish = 0xff00;

	write(fdd, &finish, 2);
	totprice = 0;

	printf("bye bye\n");
}

// LCD 패널 터치 영역에 따른 상태 변화
void *t_function(void *data)
{
	
	int thr_id2;
	pthread_t p_thread_2[9];
	char	eventFullPathName[100];
	int	eventnum;
	int	x, y, prex = 0, prey = 0;
	int	fb_fd, fp;

	struct  fb_var_screeninfo fbvar;
	struct  fb_fix_screeninfo fbfix;
	unsigned char   *fb_mapped;
	int		mem_size;
	eventnum = atoi("2");

	sprintf(eventFullPathName, "%s%d", EVENT_STR, eventnum);

	printf("touch input event name:%s\n", eventFullPathName);

	fp = open(eventFullPathName, O_RDONLY);
	if (-1 == fp)
	{
		printf("%s open fail\n", eventFullPathName);
		//return 1;
	}

	if (access(FBDEV_FILE, F_OK))
	{
		printf("%s: access error\n", FBDEV_FILE);
		close(fp);
		//return 1;
	}

	if ((fb_fd = open(FBDEV_FILE, O_RDWR)) < 0)
	{
		printf("%s: open error\n", FBDEV_FILE);
		close(fp);
		//return 1;
	}

	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &fbvar))
	{
		printf("%s: ioctl error - FBIOGET_VSCREENINFO \n", FBDEV_FILE);
		goto fb_err;
	}

	if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fbfix))
	{
		printf("%s: ioctl error - FBIOGET_FSCREENINFO \n", FBDEV_FILE);
		goto fb_err;
	}

	screen_width = fbvar.xres;
	screen_height = fbvar.yres;
	bits_per_pixel = fbvar.bits_per_pixel;
	line_length = fbfix.line_length;

	//printf("screen_width : %d\n", screen_width);
	//printf("screen_height : %d\n", screen_height);
	//printf("bits_per_pixel : %d\n", bits_per_pixel);
	//printf("line_length : %d\n", line_length);

	mem_size = screen_width * screen_height * 4;
	fb_mapped = (unsigned char *)mmap(0, mem_size,
		PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if (fb_mapped < 0)
	{
		printf("mmap error!\n");
		goto fb_err;
	}

	initScreen(fb_mapped);
	int count = 0;
back:
	while (count<10)
	{
		readFirstCoordinate(fp, &x, &y);

		//drawCoordinate(fb_mapped,x,y, prex, prey);
		//sleep(1);
		prex = x;
		prey = y;
		count++;
	}
	//printf("%d,%d\n", x, y);
	count = 0;
	if (mode == 1){
		if (x < 640 && x > 460){
			if (y < 240){
				lockScreen++;
				ArduinoStat = 1;
				sleep(3);		//send 와 receive 싱크 맞추기
				lockScreen--;
				ArduinoStat = 0;
			}
			else if (y < 500 && y > 300){
				lockScreen++;
				ArduinoStat = 2;
				sleep(3);
				lockScreen--;
				ArduinoStat = 0;
			}
			else if (y < 770 && y > 560){
				lockScreen++;
				ArduinoStat = 3;
				sleep(3);
				
				lockScreen--; ArduinoStat = 0;
			}
		}
		else if (x < 420 && x > 250){
			if (y < 240){
				lockScreen++;
				ArduinoStat = 4;
				sleep(3);
				
				lockScreen--; ArduinoStat = 0;
			}
			else if (y < 500 && y > 300){

				lockScreen++;
				ArduinoStat = 5;
				sleep(3);
				
				lockScreen--; 
				ArduinoStat = 0;
			}
			else if (y < 770 && y > 560){

				lockScreen++;
				ArduinoStat = 6;
				sleep(3);
				
				lockScreen--;
				ArduinoStat = 0;
			}
		}
		else if (x < 200 && x > 40){
			if (y < 240){
				lockScreen++;
				ArduinoStat = 7;
				sleep(3);
				
				lockScreen--;
				ArduinoStat = 0;
			}
			else if (y < 500 && y > 300){
				lockScreen++;
				ArduinoStat = 8;
				sleep(3);
				
				lockScreen--;
				ArduinoStat = 0;
			}
			else if (y < 770 && y > 560){
				lockScreen++;
				ArduinoStat = 9;
				sleep(3);
				lockScreen--;
				ArduinoStat = 0;
			}
		}

	}
else if (mode == 2){
	if (x < 640 && x > 460){
		if (y < 240){
			lockScreen++;
			ArduinoStat = 1;
			printf("sundae:%d\nramen:%d\ntotprice:%d\n",table[0].sundae,table[0].ramen,table[0].totPrice);
			sleep(1);
			show_totprice(1);
			show_tottlcd(1);
			sleep(2);
			clearScreen(0);
			clearScreen(1);
			lockScreen--;
			ArduinoStat = 0;
		
		}
		else if (y < 500 && y > 300){
			lockScreen++;
			ArduinoStat = 2;
			printf("sundae:%d\nramen:%d\ntotprice:%d\n", table[1].sundae, table[1].ramen, table[1].totPrice);
			sleep(1);
			show_totprice(2);
			show_tottlcd(2);
			sleep(2);
			clearScreen(0);
			clearScreen(1);
			lockScreen--;
			ArduinoStat = 0;
		}
		else if (y < 770 && y > 560){
			lockScreen++;
			ArduinoStat = 3;
			printf("sundae:%d\nramen:%d\ntotprice:%d\n", table[2].sundae, table[2].ramen, table[2].totPrice);
			sleep(1);
			show_totprice(3);
			show_tottlcd(3);
			sleep(2);
			clearScreen(0);
			clearScreen(1);
			lockScreen--;
			ArduinoStat = 0;
		}
	}
	else if (x < 420 && x > 250){
		if (y < 240){
			lockScreen++;
			ArduinoStat = 4;
			printf("sundae:%d\nramen:%d\ntotprice:%d\n", table[3].sundae, table[3].ramen, table[3].totPrice);
			sleep(1);
			show_totprice(4);
			show_tottlcd(4);
			sleep(2);
			clearScreen(0);
			clearScreen(1);
			lockScreen--;
			ArduinoStat = 0;
		}
		else if (y < 500 && y > 300){
			lockScreen++;
			ArduinoStat = 5;
			printf("sundae:%d\nramen:%d\ntotprice:%d\n", table[4].sundae, table[4].ramen, table[4].totPrice);
			sleep(1);
			show_totprice(5);
			show_tottlcd(5);
			sleep(2);
			clearScreen(0);
			clearScreen(1);
			lockScreen--;
			ArduinoStat = 0;
		}
		else if (y < 770 && y > 560){
			lockScreen++;
			ArduinoStat = 6;
			printf("sundae:%d\nramen:%d\ntotprice:%d\n", table[5].sundae, table[5].ramen, table[5].totPrice);
			sleep(1);
			show_totprice(6);
			show_tottlcd(6);
			sleep(2);
			clearScreen(0);
			clearScreen(1);
			lockScreen--;
			ArduinoStat = 0;
		}
	}
	else if (x < 200 && x > 40){
		if (y < 240){
			lockScreen++;
			ArduinoStat = 7;
			printf("sundae:%d\nramen:%d\ntotprice:%d\n", table[6].sundae, table[6].ramen, table[6].totPrice);
			sleep(1);
			show_totprice(7);

			show_tottlcd(7);
			sleep(2);
			clearScreen(0);
			clearScreen(1);
			lockScreen--;
			ArduinoStat = 0;
		}
		else if (y < 500 && y > 300){
			lockScreen++;
			ArduinoStat = 8;
			printf("sundae:%d\nramen:%d\ntotprice:%d\n", table[7].sundae, table[7].ramen, table[7].totPrice);
			sleep(1);
			show_totprice(8);
			show_tottlcd(8);
			sleep(2);
			clearScreen(0);
			clearScreen(1);
			lockScreen--;
			ArduinoStat = 0;
		}
		else if (y < 770 && y > 560){
			lockScreen++;
			ArduinoStat = 9;
			printf("sundae:%d\nramen:%d\ntotprice:%d\n", table[8].sundae, table[8].ramen, table[8].totPrice);
			sleep(1);
			show_totprice(9);
			show_tottlcd(9);
			sleep(2);
			clearScreen(0);
			clearScreen(1);
			lockScreen--;
			ArduinoStat = 0;
		}
	}
}
else if (mode == 3){

	if (x < 640 && x > 460){
		if (y < 240){
			lockScreen++;
			ArduinoStat = 1;
			table[0].sundae = 0;
			table[0].ramen = 0;
			table[0].totPrice = 0;
			showtlcd2(1);
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
			
		}
		else if (y < 500 && y > 300){
			lockScreen++;
			ArduinoStat = 2;
			table[1].sundae = 0;
			table[1].ramen = 0;
			table[1].totPrice = 0; 
			showtlcd2(2);
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
		}
		else if (y < 770 && y > 560){
			lockScreen++;
			ArduinoStat = 3;
			table[2].sundae = 0;
			table[2].ramen = 0;
			table[2].totPrice = 0;
			showtlcd2(3);
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
		}
	}
	else if (x < 420 && x > 250){
		if (y < 240){
			lockScreen++;
			ArduinoStat = 4;
			table[3].sundae = 0;
			table[3].ramen = 0;
			table[3].totPrice = 0;
			showtlcd2(4);
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
		}
		else if (y < 500 && y > 300){
			lockScreen++;
			ArduinoStat = 5;
			table[4].sundae = 0;
			table[4].ramen = 0;
			table[4].totPrice = 0;
			showtlcd2(5);
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
		}
		else if (y < 770 && y > 560){
			lockScreen++;
			ArduinoStat = 6;
			table[5].sundae = 0;
			table[5].ramen = 0;
			table[5].totPrice = 0;
			showtlcd2(6);
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
		}
	}
	else if (x < 200 && x > 40){
		if (y < 240){
			lockScreen++;
			ArduinoStat = 7;
			table[6].sundae = 0;
			table[6].ramen = 0;
			table[6].totPrice = 0;
			showtlcd2(7);
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
		}
		else if (y < 500 && y > 300){
			lockScreen++;
			ArduinoStat = 8;
			table[7].sundae = 0;
			table[7].ramen = 0;
			table[7].totPrice = 0;
			showtlcd2(8);
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
		}
		else if (y < 770 && y > 560){
			lockScreen++;
			ArduinoStat = 9;
			table[8].sundae = 0;
			table[8].ramen = 0;
			table[8].totPrice = 0;
			showtlcd2(9);
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
		}
	}
}

	if (x <1120 && x > 1015){
		if (y<260 && y>20){
			mode = 1;
			lockScreen++;
			ArduinoStat = 10;
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
			printf("mode1\n");
		}
		else if (y < 525 && y>280){
			mode = 2;
			lockScreen++;
			ArduinoStat = 11;
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
			printf("mode2\n");
		}
		else if (y<790 && y>545){
			mode = 3;
			lockScreen++;
			ArduinoStat = 12;
			sleep(1);
			lockScreen--;
			ArduinoStat = 0;
			printf("mode3\n");
		}
	}


	//	if(ArduinoStat==1)				//LED켜던지 해서 사용중 알리기  - 밑에 아두이노 행동 취하는 함수와 함께
	//show7seg2(1);       //아두이노 행동 취하기 show7seg2 함수는 나중에 지우기
		
		goto back;
	
	//	pthread_join(p_thread_2[0], (void **)&status);
fb_err:
	close(fb_fd);
	close(fp);
	
}

// ArduinoStat 값에 따라 화면 띄우기
void *t_function2(void *data)
{
	int i8, j8, k8, t8;
	int fbfd;
	int screen_width2;
	int screen_height2;
	int bits_per_pixel;
	int line_length;
	int coor_x2, coor_y2;
	int cols2 = 0, rows2 = 0;
	int mem_size2;
	int lcdLoad = 0;
	int first = 0;
	char    *pdata22, *data2;
	char    r2, g2, b2;
	unsigned long   bmpdata22[1280 * 800];
	unsigned long   pixel;
	unsigned char   *pfbmap;
	unsigned long   *ptr2;
	struct  fb_var_screeninfo fbvar;
	struct  fb_fix_screeninfo fbfix;

	printf("=================================\n");
	printf("Frame buffer Application - Bitmap\n");
	printf("=================================\n\n");

	read_bmp("default.bmp", &pdata22, &data2, &cols2, &rows2);
	
	while (1){

		
			if (first == 0){
				first++;
			}
			else{
				while (lockScreen == 0){
					;
				}
			}

			if (ArduinoStat == 1){
				read_bmp("1.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 2){
				read_bmp("2.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 3){
				read_bmp("3.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 4){
				read_bmp("4.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 5){
				read_bmp("5.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 8){
				read_bmp("8.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 6){
				read_bmp("6.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 7){
				read_bmp("7.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 9){
				read_bmp("9.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 10){
				read_bmp("sendCart.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 11){
				read_bmp("confirm.bmp", &pdata22, &data2, &cols2, &rows2);
			}
			else if (ArduinoStat == 12){
				read_bmp("empty.bmp", &pdata22, &data2, &cols2, &rows2);
			}
//			printf("Bitmap : cols2 = %d, rows2 = %d\n", cols2, rows2);
			//lockScreen++;

			for (j8 = 0; j8 < rows2 + 1; j8++)
			{
				k8 = j8 * cols2 * 3;
				t8 = (rows2 - 1 - j8) * cols2;

				for (i8 = 0; i8 < cols2; i8++)
				{
					b2 = *(data2 + (k8 + i8 * 3));
					g2 = *(data2 + (k8 + i8 * 3 + 1));
					r2 = *(data2 + (k8 + i8 * 3 + 2));

					pixel = ((r2 << 16) | (g2 << 8) | b2);
					bmpdata22[t8 + i8] = pixel;          // save bitmap data2 bottom up
				}
			}
			close_bmp(&pdata22);

			if ((fbfd = open(FBDEV_FILE, O_RDWR)) < 0)
			{
				printf("%s: open error\n", FBDEV_FILE);
				exit(1);
			}

			if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar))
			{
				printf("%s: ioctl error - FBIOGET_VSCREENINFO \n", FBDEV_FILE);
				exit(1);
			}

			if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fbfix))
			{
				printf("%s: ioctl error - FBIOGET_FSCREENINFO \n", FBDEV_FILE);
				exit(1);
			}

			if (fbvar.bits_per_pixel != 32)
			{
				fprintf(stderr, "bpp is not 32\n");
				exit(1);
			}

			screen_width2 = fbvar.xres;
			screen_height2 = fbvar.yres;
			bits_per_pixel = fbvar.bits_per_pixel;
			line_length = fbfix.line_length;
			mem_size2 = line_length * screen_height2;

//			printf("screen_width2 : %d\n", screen_width2);
//			printf("screen_height2 : %d\n", screen_height2);
//			printf("bits_per_pixel : %d\n", bits_per_pixel);
//			printf("line_length : %d\n", line_length);

			pfbmap = (unsigned char *)
				mmap(0, mem_size2, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

			if ((unsigned)pfbmap == (unsigned)-1)
			{
				perror("fbdev mmap\n");
				exit(1);
			}

			// fb clear - black
			for (coor_y2 = 0; coor_y2 < screen_height2; coor_y2++) {
				ptr2 = (unsigned long *)pfbmap + (screen_width2 * coor_y2);
				for (coor_x2 = 0; coor_x2 < screen_width2; coor_x2++)
				{
					*ptr2++ = 0x000000;
				}
			}

			// direction for image generating : (0,0)-> (1,0)-> (2,0)-> ...-> (row, column)
			for (coor_y2 = 0; coor_y2 < rows2; coor_y2++) {
				ptr2 = (unsigned long*)pfbmap + (screen_width2 * coor_y2);
				for (coor_x2 = 0; coor_x2 < cols2; coor_x2++) {
					*ptr2++ = bmpdata22[coor_x2 + coor_y2*cols2];
				}
			}
	

			// 얼마간 다른 화면을 보여준 후 다시 default 화면을 띄운다.
			sleep(3);
			read_bmp("default.bmp", &pdata22, &data2, &cols2, &rows2);

			for (j8 = 0; j8 < rows2 + 1; j8++)
			{
				k8 = j8 * cols2 * 3;
				t8 = (rows2 - 1 - j8) * cols2;

				for (i8 = 0; i8 < cols2; i8++)
				{
					b2 = *(data2 + (k8 + i8 * 3));
					g2 = *(data2 + (k8 + i8 * 3 + 1));
					r2 = *(data2 + (k8 + i8 * 3 + 2));

					pixel = ((r2 << 16) | (g2 << 8) | b2);
					bmpdata22[t8 + i8] = pixel;          // save bitmap data2 bottom up
				}
			}
			close_bmp(&pdata22);

			if ((fbfd = open(FBDEV_FILE, O_RDWR)) < 0)
			{
				printf("%s: open error\n", FBDEV_FILE);
				exit(1);
			}

			if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar))
			{
				printf("%s: ioctl error - FBIOGET_VSCREENINFO \n", FBDEV_FILE);
				exit(1);
			}

			if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fbfix))
			{
				printf("%s: ioctl error - FBIOGET_FSCREENINFO \n", FBDEV_FILE);
				exit(1);
			}

			if (fbvar.bits_per_pixel != 32)
			{
				fprintf(stderr, "bpp is not 32\n");
				exit(1);
			}

			screen_width2 = fbvar.xres;
			screen_height2 = fbvar.yres;
			bits_per_pixel = fbvar.bits_per_pixel;
			line_length = fbfix.line_length;
			mem_size2 = line_length * screen_height2;

//			printf("screen_width2 : %d\n", screen_width2);
//			printf("screen_height2 : %d\n", screen_height2);
//			printf("bits_per_pixel : %d\n", bits_per_pixel);
//			printf("line_length : %d\n", line_length);

			pfbmap = (unsigned char *)
				mmap(0, mem_size2, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

			if ((unsigned)pfbmap == (unsigned)-1)
			{
				perror("fbdev mmap\n");
				exit(1);
			}

			// fb clear - black
			for (coor_y2 = 0; coor_y2 < screen_height2; coor_y2++) {
				ptr2 = (unsigned long *)pfbmap + (screen_width2 * coor_y2);
				for (coor_x2 = 0; coor_x2 < screen_width2; coor_x2++)
				{
					*ptr2++ = 0x000000;
				}
			}

			// direction for image generating : (0,0)-> (1,0)-> (2,0)-> ...-> (row, column)
			for (coor_y2 = 0; coor_y2 < rows2; coor_y2++) {
				ptr2 = (unsigned long*)pfbmap + (screen_width2 * coor_y2);
				for (coor_x2 = 0; coor_x2 < cols2; coor_x2++) {
					*ptr2++ = bmpdata22[coor_x2 + coor_y2*cols2];
				}
			}
			while (lockScreen == 1){
				;
			}
}//while(1)
	
	munmap(pfbmap, mem_size2);
	close(fbfd);
}

// 와이~파이~
void *t_function3(void *data){
	int servSock;                    /* Socket descriptor for server */
	int clntSock;                    /* Socket descriptor for client */
	struct sockaddr_in echoServAddr; /* Local address */
	struct sockaddr_in echoClntAddr; /* Client address */
	unsigned short echoServPort;     /* Server port */
	unsigned int clntLen;            /* Length of client address data structure */

	echoServPort = atoi("3000");  /* First arg:  local port */

	/* Create socket for incoming connections */
	if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed");

	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
	echoServAddr.sin_family = AF_INET;                /* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	echoServAddr.sin_port = htons(echoServPort);      /* Local port */

	/* Bind to the local address */
	if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("bind() failed");

	/* Mark the socket so it will listen for incoming connections */
	if (listen(servSock, MAXPENDING) < 0)
		DieWithError("listen() failed");

	for (;;) /* Run forever */
	{
		/* Set the size of the in-out parameter */
		clntLen = sizeof(echoClntAddr);

		/* Wait for a client to connect */
		if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr,
			&clntLen)) < 0)
			DieWithError("accept() failed");

		/* clntSock is connected to a client! */

		printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

		if (HandleTCPClient(clntSock) == 1){
			if (listen(servSock, MAXPENDING) < 0)
				DieWithError("listen() failed");
		}
	}
	/* NOT REACHED */

}

int main(int argc, char **argv)
{  
	//oled
	int writeNum;
	unsigned char wdata[10];
	int readNum;
	unsigned short* rdata = NULL;
	unsigned short wCmd[10];
	
	//oled
	int qty = 0;
	for (qty = 0; qty < 9; qty++){
		table[qty].sundae = 0;
		table[qty].ramen = 0;
		table[qty].totPrice = 0;
	}
	int retvalue = 0;
	int dipvalue = 0;
	int keypad = 0x00;
	int totprice = 0;
	int dipvalue2 = 0;
	pthread_t p_thread[3];
	int thr_id;
	int status;
	char p1[] = "thread_1"; // 1번 스레드 이름
	char p2[] = "thread_2"; // 2번 스레드 이름
	char p3[] = "thread_3";	// 3번 스레드 이름

	thr_id = pthread_create(&p_thread[0], NULL, t_function, (void *)p1);    // 터치 -> 상태변화
	thr_id = pthread_create(&p_thread[1], NULL, t_function2, (void *)p2);	// LCD 화면 띄우기
	thr_id = pthread_create(&p_thread[2], NULL, t_function3, (void *)p3);	// 와이~파이~

	fda = open("/dev/dipsw", O_RDWR);
	fdb = open("/dev/mled", O_RDWR);
	fdc = open("/dev/oled", O_RDWR);
	fdd = open("/dev/fnd", O_RDWR);
	fde = open("/dev/tlcd", O_RDWR);
	fdf = open("/dev/key", O_RDWR);

	if (fda < 0)
	{
		perror("dipsw driver open error.\n");
		return 1;
	}
	if (fdb < 0)
	{
		perror("mled driver open error.\n");
		return 1;
	}
	if (fdc <0){
		perror("oled driver open error.\n");
		return 1;
	}
	if (fdd<0){
		perror("fnd driver open error.\n");
		return 1;
	}
	if (fde <0){
		perror("tcld driver open error.\n");
		return 1;
	}
	if (fdf<0){
		perror("key driver open error.\n");
		return 1;
	}
	clearScreen(0);
	Mode = MODE_INIT; Init();  //oled mode init
	int count = 0;
	
	while (1){
back:
		while (dipvalue == 0){ //read dipvalue
			read(fda, &retvalue, 4);
			retvalue &= 0xF;
			dipvalue = retvalue;
			dipvalue2 = retvalue;

		}
		while (count < 20){
			showmat(dipvalue);
			count++;
		}
		count = 0;
//		printf("dip:%x,ret:%x", dipvalue, retvalue);
		while (dipvalue != 0){
			//printf("dipvalue:%d\n", dipvalue);
			sleep(1);
			retvalue = 0;
			keypad = 0x00;
			//printf("retvalue:%x,keypad:%x\n", retvalue, keypad);
			while (keypad==0x00){
				
				//printf("keypad:%X\n", keypad);
				read(fdf, &retvalue, 4);
				//printf("retvalue:%x", retvalue);
				retvalue &= 0xFF;
				keypad = retvalue;
				//printf("keypad2:%X\n", keypad);
				read(fda, &retvalue, 4);
				retvalue &= 0xF;
				dipvalue = retvalue;
				//dipvalue2 = retvalue;
				retvalue = 0x00;

				read(fda, &retvalue, 4);
				retvalue &= 0xF;
				dipvalue = retvalue;
				//dipvalue2 = retvalue;
				//printf("dip:%d\n", dipvalue);
				if (dipvalue == 0){
					//총가격표시하기
					show_totprice(dipvalue2);

					sleep(2);
					goto back;
				}
				else
					retvalue = 0x00;
			}
			
				show7seg(keypad, dipvalue);
				//printf("7seg,keypad:%X\n",keypad);
				sleep(2);
				showoled(keypad);
				//printf("led,keypad:%X\n", keypad);
				//sleep(2);
				
				
				showtlcd(keypad);
				//printf("lcd,keypad:%X\n", keypad);
				sleep(3);
				clearScreen(0);
				//printf("lcdclear,keypad:%X\n", keypad);
				//sleep(2);
				reset();
				Init();
				keypad = 0x00;
			
				read(fda, &retvalue, 4);
				retvalue &= 0xF;
				dipvalue = retvalue;
		}
		
	}
	pthread_join(p_thread[0], (void **)&status);
	pthread_join(p_thread[1], (void **)&status);
	pthread_join(p_thread[2], (void **)&status);
	close(fda);
	close(fdb);
	close(fdc);
	close(fdd);
	close(fde);
	close(fdf);
	return retvalue;
}








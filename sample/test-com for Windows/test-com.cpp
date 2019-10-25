// 6AFS-Test.cpp
#define 	DEBUGSS	0

#include	<windows.h>
#include	<iostream.h>
#include	<stdio.h>
#include	<conio.h>
#include	<time.h>

int SetComAttr(HANDLE fdc);

int main()
	{
	FILE		*fd;
	HANDLE		fdc;
	char		fname[64];
	char		devname[64];
	char		str[256];
	unsigned short	data[6];
	int			comNo;
	int			tick;
	int			clk, clkb, clkb2, clk0;
	int			tw;
	int			num;
	DWORD		n;

	fd = NULL;
	fdc = NULL;

start :
	// COMポートをオープン
	printf("Enter COM port > ");
	scanf("%d", &comNo);
	printf("Open COM%d\n", comNo);

	sprintf(devname, "COM%d", comNo);
	fdc = CreateFile(devname, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0);
	if (fdc == INVALID_HANDLE_VALUE)
		goto over;

	// サンプリング周期を得る
	tw = 16;
	printf("Enter sampling time (ms) > ");
	scanf("%d", &tw);
	printf("Sampling time = %d ms\n", tw);

	printf("Enter File name > ");
	scanf("%s", fname);
	fd = fopen(fname, "w");
	if (!fd)
		goto over;

	// COMポートのボーレート等を設定
	SetComAttr(fdc);

	// データを読み出す
	printf("=== record data ===\n");
	clk0 = clock() / (CLOCKS_PER_SEC / 1000);
	clkb = 0;
	clkb2 = 0;
	num = 0;

	// 単データリクエスト（初回分）
	WriteFile(fdc, "R", 1, &n, 0);

	while (true)
		{
		// サンプリング周期だけ待つ
		while (true)
			{
			clk = clock() / (CLOCKS_PER_SEC / 1000) - clk0;

			if (clk >= clkb + tw)
				{
				clkb = clk / tw * tw;
				break;
				}
			}

		// 単データリクエスト（次回分）
		WriteFile(fdc, "R", 1, &n, 0);

		// 単データを得る
		n = 0;
		ReadFile(fdc, str, 27, &n, 0);
		
		if (n < 27)
			{
//			printf ("=== error ! n = %d ===\n", n);
			goto skip;
			}

		sscanf(str, "%1d%4hx%4hx%4hx%4hx%4hx%4hx",
			 &tick, &data[0], &data[1], &data[2], &data[3], &data[4], &data[5]);

		sprintf(str, "%05d,%d,%05d,%05d,%05d,%05d,%05d,%05d\n",
			clk / tw * tw, tick,
			data[0], data[1], data[2], data[3], data[4], data[5]);

		fprintf(fd, str);
		num++;

skip :
		if (clk >= 10000)
			break;

		// コンソールに間引き表示
		if (clk >= clkb2 + 1000)
			{
			printf(str);
			if (_kbhit())
				break;
			clkb2 = clk / 1000 * 1000;
			}
		}

over :
	if (fd)
		{
		fclose(fd);
		fd = NULL;
		}

	if (fdc)
		{
		EscapeCommFunction(fdc, CLRDTR);	// Clear the DTR line
		CloseHandle(fdc);
		fdc = NULL;
		}

	printf ("=== num = %d ===\n", num);

	printf("exit (y / n) ? > ");
	scanf("%s", str);
	if (str[0] == 'y')
		{
//		exit(0);
		}
	else
		{
		goto start;
		}
	}


int SetComAttr(HANDLE fdc)
	{
	int				status;
	BOOL			success;
	COMMTIMEOUTS	timeouts;
	DCB				dcb;

	status = 0;

	timeouts.ReadIntervalTimeout = 1; 
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.ReadTotalTimeoutConstant = 1;
	timeouts.WriteTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = 1;

	SetCommTimeouts(fdc, &timeouts);		//  Read Write のタイムアウトの設定
	EscapeCommFunction(fdc, SETDTR);		// Set the Data Terminal Ready line
	
	// Get the current settings of the COMM port 
	success = GetCommState(fdc, &dcb);
	if(!success)
		{
		status = -1;
		}

	// Modify the baud rate, etc.
	dcb.BaudRate = 921600;
	dcb.ByteSize = (BYTE)8;
	dcb.Parity = (BYTE)NOPARITY;
	dcb.StopBits = (BYTE)ONESTOPBIT;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	// Apply the new comm port settings
	success = SetCommState(fdc, &dcb);
	if(!success)
		{
		status = -2;
		}

	return (status);
	}


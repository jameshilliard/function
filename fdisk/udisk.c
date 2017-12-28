#define _FILE_OFFSET_BITS 64
#define __USE_LARGEFILE64

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdint.h>
#include <sys/statfs.h>


#define __M_SIZE				(1024 * 1024)
#define __BLK_SIZE			512

#define __SEEK_ONCE			0x1000

int main(int arg, char *argv[])
{
	int i, j, fd, ret;
	unsigned char buf[__BLK_SIZE];
	struct statfs diskInfo;
	uint64_t u32TotalSize, u32BlkCnt, u32PrintStart, u32PrintCnt = 1;
	long long tmp;

	if(arg != 4) {
		printf("udisk file blkstart blknumber\n");
		return 0;
	}

	u32PrintStart = atoll(argv[2]);
	u32PrintCnt   = atoll(argv[3]);
	printf("Print start:%ld, print count:%ld\n", u32PrintStart, u32PrintCnt);

	statfs("/dev/sdb", &diskInfo);
	printf("block size:0x%x, blocks: 0x%x\n", (unsigned int)diskInfo.f_bsize, (unsigned int)diskInfo.f_blocks);
	
	u32TotalSize = diskInfo.f_bsize * diskInfo.f_blocks;
	u32BlkCnt    = u32TotalSize / __BLK_SIZE;
	printf("Total size:%ldMB, block count:%ld\n", (u32TotalSize / __M_SIZE), u32BlkCnt);

	u32PrintCnt = u32PrintCnt > u32BlkCnt ? u32BlkCnt : u32PrintCnt;

	fd = open(argv[1], O_RDWR | __O_LARGEFILE);
	if(-1 == fd)	{
		printf("open file error!\n");
		return -1;
	}

	tmp = u32PrintStart;
	tmp *= 512;
	tmp = lseek(fd, tmp, SEEK_SET);
	if(tmp <0)	{
		printf("seek file error!\n");
		return -1;
	}
	
	for(j=0; j<u32PrintCnt; j++) {
		
		ret == read(fd, buf, 512);
		if(ret < 0)
		{
			printf("read file error!\n");
			return -1;
		}

		printf("\n-%ld-------------------------------------------------------------", (j+u32PrintStart));
		for(i=0; i<512; i++)
		{
			if((i%0x10) == 0)
				printf("\n%08x:\t", (i + j * __BLK_SIZE));

			printf("%02x ", buf[i]);
		}

		//getchar();
	}

	close(fd);
	printf("\ndump over !\n");
	return 0;
}

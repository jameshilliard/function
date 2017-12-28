#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/statfs.h>

#define __BLOCK_SIZE      512
#define __RAW_AREA_SIZE   (4 * 1024 * 1024)
#define __BLOCK_END       0xFFFFFFFFu
#define __PART_SIZE       (1024 * 1024 * 1024)


#define __calc_unit(offset)      ((offset) / __BLOCK_SIZE)

typedef struct
{
	uint32_t   u32CyStart;
	uint32_t   u32CyEnd;

	uint32_t   u32HdStart;
	uint32_t   u32HdEnd;
  
	uint32_t   u32StStart;
	uint32_t   u32StEnd;

	uint32_t   u32AvailBlk;
	uint32_t   u32Unit;
	uint32_t   u32TotalBlk;

	uint32_t   u32Mode;
} dev_info_t;

typedef struct
{
	uint8_t    u8Bootable;
	uint8_t    u8PartId;
  
	uint32_t   u32CyStart;
	uint32_t   u32CyEnd;

	uint32_t   u32HdStart;
	uint32_t   u32HdEnd;
  
	uint32_t   u32StStart;
	uint32_t   u32StEnd;

	uint32_t   u32BlkStart;
	uint32_t   u32BlkCnt;
	uint32_t   u32BlkEnd;
} partition_info_t;

void _encode_chs(int C, int H, int S, unsigned char *result) {
	
	*result++ = (unsigned char) H;
	*result++ = (unsigned char) ( S + ((C & 0x00000300) >> 2) );
	*result   = (unsigned char) (C & 0x000000FF); 
}

void _encode_partitionInfo(partition_info_t partInfo, unsigned char *result) {
  
	*result++ = partInfo.u8Bootable;

	_encode_chs(partInfo.u32CyStart, partInfo.u32HdStart, partInfo.u32StStart, result);
	result +=3;
	*result++ = partInfo.u8PartId;

	_encode_chs(partInfo.u32CyEnd, partInfo.u32HdStart, partInfo.u32StStart, result);
	result += 3;

	memcpy(result, (unsigned char *)&(partInfo.u32BlkStart), 4);
	result += 4;	
	
	memcpy(result, (unsigned char *)&(partInfo.u32BlkCnt), 4);
}

void _set_deviceInfo(dev_info_t *pstDevInfo, uint32_t u32TotalBlk) {
  
	pstDevInfo->u32CyStart    = 0;
	pstDevInfo->u32CyEnd      = 1023;

	pstDevInfo->u32HdStart    = 1;
	pstDevInfo->u32HdEnd      = 254;

	pstDevInfo->u32StStart    = 1;
	pstDevInfo->u32StEnd      = 63;

	pstDevInfo->u32TotalBlk   = u32TotalBlk;
	pstDevInfo->u32AvailBlk   = pstDevInfo->u32CyEnd *
	                            pstDevInfo->u32HdEnd *
	                            pstDevInfo->u32StEnd;
	pstDevInfo->u32Unit       = pstDevInfo->u32HdEnd * pstDevInfo->u32StEnd;
}

void _set_partitionInfo(uint32_t u32LbaStart, uint32_t u32Cnt,
						dev_info_t *pstDevInfo, partition_info_t *pstPartInfo) {

	uint32_t u32Tmp;
  
	pstPartInfo->u32BlkStart = u32LbaStart;
	pstPartInfo->u32CyStart  = u32LbaStart / (pstDevInfo->u32HdEnd * pstDevInfo->u32StEnd);
	u32Tmp = u32LbaStart % (pstDevInfo->u32HdEnd * pstDevInfo->u32StEnd);
	pstPartInfo->u32HdStart  = u32Tmp / pstDevInfo->u32StEnd;
	pstPartInfo->u32StStart  = u32Tmp % pstDevInfo->u32StEnd + 1;

	if(u32Cnt == __BLOCK_END) {
		pstPartInfo->u32BlkEnd   = pstDevInfo->u32HdEnd *
		                           pstDevInfo->u32StEnd *
		                           pstDevInfo->u32CyEnd;
		pstPartInfo->u32BlkCnt   = pstPartInfo->u32BlkEnd - pstPartInfo->u32BlkStart + 1;

		pstPartInfo->u32CyEnd    = pstPartInfo->u32BlkEnd / pstDevInfo->u32Unit;
		pstPartInfo->u32HdEnd    = pstDevInfo->u32HdEnd - 1;
		pstPartInfo->u32StEnd    = pstDevInfo->u32StEnd;
  }
  else {
		pstPartInfo->u32BlkCnt   = u32Cnt;
		pstPartInfo->u32BlkEnd   = u32LbaStart + u32Cnt -1;

		pstPartInfo->u32CyEnd    = pstPartInfo->u32BlkEnd / pstDevInfo->u32Unit;
		u32Tmp                   = pstPartInfo->u32BlkEnd % pstDevInfo->u32Unit;
		pstPartInfo->u32HdEnd    = u32Tmp / pstDevInfo->u32StEnd;
		pstPartInfo->u32StEnd    = u32Tmp % pstDevInfo->u32StEnd + 1;
  }
}

void do_partition(uint32_t u32TotalBlk, uint8_t *pu32Buf) {

	uint32_t u32BlkStart, u32BlkOffset;
	dev_info_t stDevInfo;
	partition_info_t stPartition[4];

	memset(&stDevInfo, 0, sizeof(dev_info_t));
	memset(&stPartition[0], 0, (sizeof(partition_info_t) * 4));
	_set_deviceInfo(&stDevInfo, u32TotalBlk);

	u32BlkStart  = __calc_unit(__RAW_AREA_SIZE);
	u32BlkOffset = __calc_unit(__PART_SIZE);
	stPartition[0].u8PartId = 0x83;
	_set_partitionInfo(u32BlkStart, u32BlkOffset, &stDevInfo, &stPartition[0]);

	u32BlkStart += u32BlkOffset;
	u32BlkOffset = __calc_unit(__PART_SIZE);
	stPartition[1].u8PartId = 0x83;
	_set_partitionInfo(u32BlkStart, u32BlkOffset, &stDevInfo, &stPartition[1]);

	u32BlkStart += u32BlkOffset;
	u32BlkOffset = __calc_unit(__PART_SIZE);
	stPartition[2].u8PartId = 0x83;
	_set_partitionInfo(u32BlkStart, u32BlkOffset, &stDevInfo, &stPartition[2]);

	u32BlkStart += u32BlkOffset;
	u32BlkOffset = __BLOCK_END;
	stPartition[3].u8PartId = 0x83;
	_set_partitionInfo(u32BlkStart, u32BlkOffset, &stDevInfo, &stPartition[3]);

	pu32Buf[510] = 0x55;
	pu32Buf[511] = 0xaa;
	_encode_partitionInfo(stPartition[0], &pu32Buf[0x1ce]);
	_encode_partitionInfo(stPartition[1], &pu32Buf[0x1de]);
	_encode_partitionInfo(stPartition[2], &pu32Buf[0x1ee]);
	_encode_partitionInfo(stPartition[3], &pu32Buf[0x1be]);
}

int main(int arg, char *argv[]) {
  
	int i, fd, ret;
	uint8_t mbr[512] = {0};
	struct statfs diskInfo;
	uint32_t u32TotalBlock; 

	statfs("/dev/sdb", &diskInfo);
	printf("block size:0x%x, blocks: 0x%x\n",
		   (unsigned int)diskInfo.f_bsize,
		   (unsigned int)diskInfo.f_blocks);

	u32TotalBlock = diskInfo.f_bsize * diskInfo.f_blocks / __BLOCK_SIZE;

	do_partition(u32TotalBlock, mbr);

	printf("mbr:");
	for(i=0; i<512; i++)
	{
		if((i%0x10) == 0)
			printf("\n%08x:\t", i);

		printf("%02x ", mbr[i]);
	}

	fd = open("/dev/sdb", O_RDWR);
	if(-1 == fd)
	{
		printf("open file error!");
		return -1;
	}	

	ret == write(fd, mbr, 512);
	if(fd < 0) {
	  printf("write partition information error!\n");
	  return -1;
	}
	close(fd);

	printf("\nmbr partition done!\n");
	return 0;
}

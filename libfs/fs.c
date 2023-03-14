#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF
#define BLOCK_SIZE 4096

struct superblock
{
	char signature[8];		  // must be equal to “ECS150FS”
	uint16_t total_blocks;	  // total amount of blocks of virtual disk
	uint16_t root_dir;		  // root directory block index
	uint16_t data_block;	  // data block start index
	uint16_t num_data_blocks; // amount of data blocks
	uint8_t num_FAT_blocks;	  // number of blocks for FAT
	char padding[4079];		  // unused/padding
};

struct FAT
{
	uint16_t *entries;
	uint16_t num_entries; // equal to the number of data blocks in disk
};

struct file_entry
{
	char file_name[16];
	uint32_t file_size;
	uint16_t first_data_block;
	char padding[10];
};

struct rootdir
{
	struct file_entry entries[128];
};

struct superblock sb;
struct FAT fat;
struct rootdir root;
struct file_entry fEntry;
int mounted = 0;

int fs_mount(const char *diskname)
{
	/* open the virtual disk, using the block API */
	if (block_disk_open(diskname) == -1)
	{
		return -1;
	}

	/* load the meta-information */
	if (block_read(0, &sb) == -1)
	{
		return -1;
	}

	/* validate signature of the superblock is ECS150FS */
	if (strcmp(sb.signature, "ECS150FS") != 0)
	{
		return -1;
	}

	/* initialize FAT */
	if (block_disk_count() != sb.num_data_blocks)
	{
		return -1;
	}

	fat.num_entries = sb.num_data_blocks;
	fat.entries = malloc(fat.num_entries * sizeof(uint16_t));

	/* load FAT blocks */
	uint16_t *block = malloc(BLOCK_SIZE); // block index of first FAT block

	for (int i = 1; i < sb.num_FAT_blocks; i++)
	{
		// index 0 of fat is EOC
		if (block_read(i, block) == -1)
		{
			return -1;
		}
		// copy block into fat entries array
		memcpy(fat.entries + (i * BLOCK_SIZE / 2), block, BLOCK_SIZE);
	}
	mounted = 1;
	return 0;
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
	free(fat.entries);

	if (block_disk_close() == -1)
	{
		return -1;
	}

	return 0;
}

int fs_info(void)
{
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
	if (!mounted)
	{
		return -1;
	}

	// Loop through directory and check if file exists or empty entry exists
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(root.entries[i].file_name, filename) == 0)
		{
			return -1;
		}
		else if (root.entries[i].file_name == "")
		{
			// Empty entry exists create a new blank file in entry
			memcpy(root.entries[i].file_name, filename, sizeof(fEntry));
			root.entries[i].file_size = 0;
			root.entries[i].first_data_block = FAT_EOC;
			return 0;
		}
	}

	return -1;
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
	if (!mounted)
	{
		return -1;
	}

	// Loop through root directory and look for filename

	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(root.entries[i].file_name, filename) == 0)
		{
			memcpy(root.entries[i].file_name, "", sizeof(fEntry));
			int curBlock = root.entries[i].first_data_block;
			for (int i = 0; i < fat.num_entries; i++)
			{
				if (curBlock == FAT_EOC)
				{
					break;
				}
				fat.entries[curBlock] = 0;
				curBlock = fat.entries[curBlock + 1];
			}
			return 0;
		}
	}

	return -1;
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
	if (!mounted)
	{
		return -1;
	}

	int curFile = 0;
	while (!strcmp(root.entries[curFile].file_name, "") == 0)
	{
		printf("file: %s, ", root.entries[curFile].file_name);
		printf("size: %d, ", root.entries[curFile].file_size);
		printf("data block: %d", root.entries[curFile].first_data_block);
		curFile++;
	}

	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

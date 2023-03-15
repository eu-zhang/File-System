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

struct file_descriptor
{
	uint32_t offset;
	uint16_t data_block_index;
	uint16_t dir_index;
	int open;
};

struct file_descriptor fd_table[128];

struct superblock sb;
struct FAT fat;
struct rootdir root;
struct file_entry fEntry;
int mounted = 0;

/* returns the index of the data block corresponding to the file’s offset */
int block_index(int fd)
{
	/* get current block offset */
	uint16_t block_index = root.entries[fd].first_data_block;
	uint16_t block_offset = fd_table[fd].offset % BLOCK_SIZE;

	/* follow FAT until block that corresponds to the offset */
	for (int i = 0; i < block_offset; i++)
	{
		// add error checking if reached EOC prematurely
		block_index = fat.entries[block_index];
	}
	return block_index;
}

/* allocates a new data block and link it at the end of the file’s data block chain */
void create_new_block()
{
}

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

	/* load root directory */
	if (block_read(sb.root_dir, &root) == -1)
	{
		return -1;
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
		else if (strcmp(root.entries[i].file_name, "\0") == 0)
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
			memcpy(root.entries[i].file_name, "\0", sizeof(fEntry));
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
	while (strcmp(root.entries[curFile].file_name, "\0") != 0)
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
	if (!mounted)
	{
		return -1;
	}

	printf("FS Info:\n");
	printf("total_blk_count=%d\n", sb.total_blocks);
	printf("fat_blk_count=%d\n", sb.num_FAT_blocks);
	printf("rdir_blk=%d\n", sb.root_dir);
	printf("data_blk=%d\n", sb.data_block);
	printf("data_blk_count=%d\n", sb.num_data_blocks);
	//   printf("fat_free_ratio=%d/%d\n", fat_free,sb.num_data_blocks);
	//   printf("rdir_free_ratio=%d/%d\n", rdir_free,FS_FILE_MAX_COUNT);
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
	if (!mounted)
	{
		return -1;
	}

	uint16_t block = block_index(fd);

	/* read @count bytes of data from the file referenced by @fd into @buf*/
	char bounce_buffer[BLOCK_SIZE];

	uint32_t bytes_read = 0; // bytes read so far

	while (bytes_read < count)
	{
		if (block_read(sb.data_block + block, &bounce_buffer) == -1)
		{
			return -1;
		}
		/* copy only right amount of bytes from bounce buffer into buf */
		uint32_t num_copied = count - bytes_read;
		if (num_copied > BLOCK_SIZE)
		{
			num_copied = BLOCK_SIZE;
		}
		memcpy(buf + bytes_read, bounce_buffer, num_copied); // copies copy num of bytes
		bytes_read += num_copied;

		/* update offset */
		fd_table[fd].offset += bytes_read;

		/* move to next block if still haven't read "count" bytes */
		if (bytes_read < count)
		{
			block = fat.entries[block];
			if (block == FAT_EOC)
			{
				break; // less than @count bytes until the end of the file
			}
		}
	}
	return bytes_read;
}

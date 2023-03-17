#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

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
	int open;
	struct file_entry *file;
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
	uint16_t block_offset = fd_table[fd].offset / BLOCK_SIZE;

	/* follow FAT until block that corresponds to the offset */
	for (int i = 0; i < block_offset; i++)
	{
		// add error checking if reached EOC prematurely

		block_index = fat.entries[block_index];
	}
	return block_index;
}

/* allocates a new data block and link it at the end of the file’s data block chain */
uint16_t create_new_block(uint16_t last_block, bool first_block, int fd) 
{
	uint16_t new_block = sb.data_block;
	/* first fit strategy */
	if (first_block) 
	{
		for (int i = 0; i < sb.num_data_blocks; i++)
		{
			if (fat.entries[new_block] == 0)
			{
				/* set new free block to be first data block */
				root.entries[fd].first_data_block = new_block;

				// mark as end of newly allocated block
				fat.entries[new_block] = FAT_EOC;
				last_block = new_block;
				break;	
			}
			new_block++;
		}
	} else {
		for (int i = 0; i < sb.num_data_blocks; i++)
		{
			if (fat.entries[new_block] == 0)
			{

				// mark as end of newly allocated block
				fat.entries[new_block] = FAT_EOC;
				fat.entries[last_block] = new_block;
				break;	
			}
			new_block++;
		}
	}
		
	/* link new block to end of data block chain */
	return new_block;
}

int verify_file_name(const char *filename)
{
	if (filename == NULL || strlen(filename) > FS_FILENAME_LEN)
	{
		return -1;
	}
	return 0;
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
	if (strncmp(sb.signature, "ECS150FS", 8) != 0)
	{
		return -1;
	}

	/* initialize FAT */
	if (block_disk_count() != sb.total_blocks)
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
		memcpy(fat.entries + (i * BLOCK_SIZE / sizeof(uint16_t)), block, BLOCK_SIZE);
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
	if (!mounted)
	{
		return -1;
	}

	uint16_t fat_free = 0, rdir_free = 0;

	for (int i = 0; i < sb.num_data_blocks; i++)
	{
		if (fat.entries[i] == 0)
		{
			fat_free++;
		}
	}
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (root.entries[i].file_name[0] == '\0')
		{
			rdir_free++;
		}
	}

	printf("FS Info:\n");
	printf("total_blk_count=%u\n", sb.total_blocks);
	printf("fat_blk_count=%u\n", sb.num_FAT_blocks);
	printf("rdir_blk=%u\n", sb.root_dir);
	printf("data_blk=%u\n", sb.data_block);
	printf("data_blk_count=%u\n", sb.num_data_blocks);
	printf("fat_free_ratio=%u/%u\n", fat_free, sb.num_data_blocks);
	printf("rdir_free_ratio=%u/%u\n", rdir_free, FS_FILE_MAX_COUNT);

	return 0;
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
	if (!mounted || verify_file_name(filename) == -1)
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
		else if (root.entries[i].file_name[0] == '\0')
		{
			// Empty entry exists create a new blank file in entry
			memcpy(root.entries[i].file_name, filename, FS_FILENAME_LEN);
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
	if (!mounted || verify_file_name(filename) == -1)
	{
		return -1;
	}

	// Loop through root directory and look for filename

	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(root.entries[i].file_name, filename) == 0)
		{
			root.entries[i].file_name[0] = '\0';
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
	printf("FS Ls:\n");
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (root.entries[i].file_name[0] != '\0')
		{
			/* Format info */
			printf("file: %s, ", root.entries[i].file_name);
     			printf("size: %d, ",  root.entries[i].file_size);
      			printf("data_blk: %d\n", root.entries[i].first_data_blk);
		}
	}

	return 0;
}

int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
	if (!mounted || verify_file_name(filename) == -1)
	{
		return -1;
	}

	// Loop through directory and check if the file exists
	for (int i = 0; i < FS_FILE_MAX_COUNT; i++)
	{
		if (strcmp(root.entries[i].file_name, filename) == 0)
		{

			// Find empty fd
			for (int j = 0; j < FS_OPEN_MAX_COUNT; j++)
			{
				if (fd_table[j].open == 0)
				{
					fd_table[j].offset = 0;
					fd_table[j].open = 1;
					fd_table[j].file = &root.entries[i];
					return 0;
				}
			}
		}
		return -1; // File not found
	}

	return -1; // empty fd not found
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
	if (!mounted || fd < 0 || fd > 32 || fd_table[fd].open == 0)
	{
		return -1;
	}

	fd_table[fd].open = 0;
	fd_table[fd].offset = 0;
	fd_table[fd].file = NULL;

	return 0;
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
	if (!mounted || fd < 0 || fd > 32 || fd_table[fd].open == 0)
	{
		return -1;
	}
	return fd_table[fd].file->file_size;
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */

	// Not mounted or invalid fd
	if (!mounted || fd < 0 || fd >= FS_OPEN_MAX_COUNT || fd_table[fd].open == 0)
	{
		return -1;
	}

	// Offset larger than file size
	if (offset > root.entries[fd].file_size)
	{
		return -1;
	}

	fd_table[fd].offset = offset;

	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/* Check if file system is mounted */
	if (!mounted)
	{
		return -1;
	}

	/* TODO: validate file descriptor and buffer */
	bool is_first_entry = false;
	
	uint16_t block = block_index(fd);

	uint16_t cur_index = block;
	/* write @count bytes of data from @buf into the file @fd */
	char bounce_buffer[BLOCK_SIZE];

	uint32_t bytes_written = 0;	// bytes written so far

	while (bytes_written < count)
	{
		if (block == FAT_EOC)
		{
			if (root.entries[fd].first_data_block == FAT_EOC) {
				is_first_entry = true;
			} else {
				is_first_entry = false;
			}
			block = create_new_block(cur_index, is_first_entry, fd);
			// block = fat.entries[block];

			// which one am i writing to???
		}
		
		/* Read block */
		if (block_read(block, &bounce_buffer) == -1)
		{
			return -1;
		}


		/* calculate offset for write (current offset % block size gives offset in block)*/
		/* block offset is offset within bounce buffer */
		uint32_t block_offset = fd_table[fd].offset % BLOCK_SIZE;

		/* get remaining bytes to be written total */
		uint32_t bytes_left = count - bytes_written;
		
		/* get remaining bytes to be written in current block */
		uint32_t block_bytes_left = BLOCK_SIZE - block_offset;

		if (bytes_left > block_bytes_left)
		{
			bytes_left = block_bytes_left;
		}

		/* copy input data from buf to bounce buffer, bytes left in current block */
		memcpy(bounce_buffer + block_offset, buf + bytes_written, bytes_left);

		/* update file offset */
		fd_table[fd].offset += bytes_left;
		bytes_written += bytes_left;

		/* Write block */
		
		block_write(sb.data_block + block, bounce_buffer);


		/* go to next block */
		cur_index = block;
	
		block = fat.entries[block];

	}
	if (fd_table[fd].offset > root.entries[fd].file_size)
	{
		root.entries[fd].file_size = fd_table[fd].offset;
	}

	return bytes_written;
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
	uint16_t bounce_buffer_offset = fd_table[fd].offset % BLOCK_SIZE;

	while (bytes_read < count)
	{
		if (block_read(sb.data_block + block, &bounce_buffer) == -1)
		{
			return -1;
		}
		
		/* copy only right amount of bytes from bounce buffer into buf */
		uint32_t num_to_copy = count - bytes_read;
		if (num_to_copy > BLOCK_SIZE)
		{
			num_to_copy = BLOCK_SIZE;
		}
		

		memcpy(buf + bytes_read, bounce_buffer + bounce_buffer_offset, num_to_copy); // copies copy num of bytes
		
		bytes_read += num_to_copy;
		
		/* move to next block if still haven't read "count" bytes */
		if (bytes_read < count)
		{
			block = fat.entries[block];
			
			if (block == FAT_EOC)
			{
				break; // less than @count bytes until the end of the file
			}
		}
		bounce_buffer_offset = 0;
	}
	/* update offset */
	fd_table[fd].offset += bytes_read;
	

	return bytes_read;
}

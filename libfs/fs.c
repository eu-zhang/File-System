#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xFFFF

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

/* TODO: Phase 1 */

int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
	/* open the virtual disk, using the block API, and load the meta-information that is necessary to handle the file system operations */
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
}

int fs_info(void)
{
	/* TODO: Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
}

int fs_ls(void)
{
	/* TODO: Phase 2 */
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

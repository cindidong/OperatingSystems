/*
 * NAME: Arnold Pfahnl, Cindi Dong
 * EMAIL: ajpfahnl@gmail.com, cindidong@gmail.com
 * ID: 305176399, 405126747
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include "ext2_fs.h"

#define EXT2_SUPERBLOCK_OFFSET 1024

static int fd;
static int block_size;
static int logical_block;
struct ext2_super_block  superblock;
struct ext2_group_desc   group_descriptor;
struct ext2_inode        inode_table;

void pread_wrap (int fildes, void *buf, size_t nbyte, off_t offset)
{
    if (pread(fildes, buf, nbyte, offset) == -1)
    {
        fprintf(stderr, "Error with read(): %s\n", strerror(errno));
        exit(2);
    }
}

char * create_time(time_t rawtime)
{
    struct tm * tm_time = gmtime(&rawtime);
    if (tm_time == NULL)
    {
        fprintf(stderr, "Error with gmtime(): %s\n", strerror(errno));
        exit(2);
    }
    char * timestr = malloc(100);
    sprintf(timestr, "%.2d/%.2d/%.2d %.2d:%.2d:%.2d",
            tm_time->tm_mon+1, tm_time->tm_mday, (tm_time->tm_year+1900)%100,
            tm_time->tm_hour,  tm_time->tm_min,   tm_time->tm_sec);
    return timestr;
}

void dir_block (__u32 block, int inode_num)
{
    //deal with each directory entry separately to avoid messing up indexing in the for loop
    struct ext2_dir_entry *dir = malloc(sizeof(struct ext2_dir_entry));
    int log_offset = 0;
    
    //if block is allocated
    if (block != 0)
    {
        //until reach the end of the block
        while(log_offset < block_size)
        {
            int curr_offset = (block * block_size) + log_offset;
            //putting the current directory entry in *directory
            pread_wrap(fd, dir, sizeof(struct ext2_dir_entry), curr_offset);
            //if it is allocated
            if(dir->inode != 0)
            {
                char name[EXT2_NAME_LEN+1];
                memcpy(&name, dir->name, dir->name_len);
                name[dir->name_len] = '\0';
                printf("DIRENT,%u,%d,%u,%u,%u,'%s'\n",
                       inode_num,
                       log_offset,
                       dir->inode,
                       dir->rec_len,
                       dir->name_len,
                       name);
            }
            //move to the next file in the directory (+ the rec_len of the previous file name)
            log_offset += dir->rec_len;
        }
    }
    free(dir);
}


void indirect_block(int inode_num, int level_orig, int level, __u32 block, char type)
{
    if (level == 0)
    {
        if (type == 'd')
        {
            dir_block(block, inode_num);
        }
        logical_block++;
        return;
    }
    __u32 block_ref;
    int offset;
    for (offset=0; offset<block_size; offset+=sizeof(block))
    {
        pread_wrap(fd, &block_ref, sizeof(block_ref), block*block_size + offset);
        if (block_ref != 0)
        {
            printf("INDIRECT,%u,%d,%u,%u,%u\n",
                   inode_num,
                   level,
                   logical_block,
                   block,
                   block_ref);
            indirect_block(inode_num, level_orig, level-1, block_ref, type);
        }
        else // increment logical block by number of blocks skipped
        {
            if (level == 1)
            {
                logical_block++;
            }
            else if (level == 2)
            {
                logical_block += block_size/sizeof(block);
            }
            else if (level == 3)
            {
                logical_block += (block_size/sizeof(block)) * (block_size/sizeof(block));
            }
        }
    }
}
 
int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: ./lab3a <file system image>\n");
        exit(1);
    }
    
    if ((fd = open(argv[1], O_RDONLY)) == -1)
    {
        fprintf(stderr, "Error opening file %s\n", argv[1]);
        exit(1);
    }
    
    // superblock summary
    /* 1. SUPERBLOCK
     * 2. total # of blocks (decimal)
     * 3. total number of i-nodes (decimal)
     * 4. block size (bytes, decimal)
     * 5. i-node size (bytes, decimal)
     * 6. blocks per group (decimal)
     * 7. i-nodes per group (decimal)
     * 8. first non-reserved i-node (decimal)
     */
    int offset = EXT2_SUPERBLOCK_OFFSET;
    pread_wrap(fd, &superblock, sizeof(superblock), offset);
    
    int total_blocks      = superblock.s_blocks_count;
    int total_inodes      = superblock.s_inodes_count;
    block_size            = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
    int inode_size        = superblock.s_inode_size;
    int blocks_per_group  = superblock.s_blocks_per_group;
    int inodes_per_group  = superblock.s_inodes_per_group;
    int first_nores_inode = superblock.s_first_ino;
    
    printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
           total_blocks,
           total_inodes,
           block_size,
           inode_size,
           blocks_per_group,
           inodes_per_group,
           first_nores_inode);
    
    // group summary
    /* 1. GROUP
     * 2. group number (decimal, starting from zero)
     * 3. total number of blocks in this group (decimal)
     * 4. total number of i-nodes in this group (decimal)
     * 5. number of free blocks (decimal)
     * 6. number of free i-nodes (decimal)
     * 7. block number of free block bitmap for this group (decimal)
     * 8. block number of free i-node bitmap for this group (decimal)
     * 9. block number of first block of i-nodes in this group (decimal)
     */
    // Note: only 1 group
    int last_group_blocks = total_blocks % blocks_per_group;
    int last_group_inodes = total_inodes % inodes_per_group;
   
    // group block offset
    int first_block = 0;
    if (block_size == EXT2_MIN_BLOCK_SIZE)
    {
        first_block = 1;
    }
    // NOTE: 0 for block size larger than 1KB, 1 otherwise
    offset = first_block*block_size + block_size;
    // NOTE: block group descriptor table starts on the first block following the superblock
    
    pread_wrap(fd, &group_descriptor, sizeof(group_descriptor), offset);
    
    int group_number     = 0;
    int blocks_total     = blocks_per_group;
    if (last_group_blocks > 0)
        blocks_total     = last_group_blocks;
    int inodes_total     = inodes_per_group;
    if (last_group_inodes > 0)
        inodes_total     = last_group_inodes;
    int free_blocks      = group_descriptor.bg_free_blocks_count;
    int free_inodes      = group_descriptor.bg_free_inodes_count;
    int block_bitmap_loc = group_descriptor.bg_block_bitmap;
    int inode_bitmap_loc = group_descriptor.bg_inode_bitmap;
    int inode_table_loc  = group_descriptor.bg_inode_table;
    
    printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
           group_number,
           blocks_total,
           inodes_total,
           free_blocks,
           free_inodes,
           block_bitmap_loc,
           inode_bitmap_loc,
           inode_table_loc);
    
    // free block entries
    /* 1. BFREE
     * 2. number of the free block (decimal)
     */
    offset = block_bitmap_loc*block_size;
    char byte;
    int i,j;
    for (i=0; i<blocks_total/8; i++)
    {
        pread_wrap(fd, &byte, 1, offset+i);
        
        for (j=0; j<8; j++)
        {
            if(!(0x1 & (byte>>j)))
            {
                printf("BFREE,%d\n",i*8+j+1);
                // NOTE: first block at bit 0
            }
        }
    }
    
    // free inode entries
    /* 1. IFREE
     * 2. number of the free I-node (decimal)
     */
    offset = inode_bitmap_loc*block_size;
    for (i=0; i<inodes_total/8; i++)
    {
        pread_wrap(fd, &byte, 1, offset+i);
        for (j=0; j<8; j++)
        {
            if(!(0x1 & (byte>>j)))
            {
                printf("IFREE,%d\n",i*8+j+1);
                // NOTE: first inode at bit 0
            }
        }
    }
    
    // inode summary
    /* 1. INODE
     * 2. inode number (decimal)
     * 3. file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
     * 4. mode (low order 12-bits, octal ... suggested format "%o")
     * 5. owner (decimal)
     * 6. group (decimal)
     * 7. link count (decimal)
     * 8. time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
     * 9. modification time (mm/dd/yy hh:mm:ss, GMT)
     * 10. time of last access (mm/dd/yy hh:mm:ss, GMT)
     * 11. file size (decimal)
     * 12. number of (512 byte) blocks of disk space (decimal) taken up by this file
     */
    
    offset = inode_table_loc*block_size;
    for (i=0; i<inodes_total; i++)
    {
        pread_wrap(fd, &inode_table, sizeof(inode_table), offset+(i*inode_size));
        if ((inode_table.i_mode == 0) || (inode_table.i_links_count == 0))
        {
            continue;
        }
        int inode_num = i + 1;
        char file_type;
        if ((inode_table.i_mode & 0xA000) == 0xA000)
            file_type = 's'; // symbolic link
        else if ((inode_table.i_mode & 0x8000) == 0x8000)
            file_type = 'f'; // regular file
        else if ((inode_table.i_mode & 0x4000) == 0x4000)
            file_type = 'd'; // directory
        else
            file_type = '?'; // other file
        __u16 mode            = inode_table.i_mode & 0xFFF;
        __u16 owner           = inode_table.i_uid;
        __u16 group           = inode_table.i_gid;
        __u16 link_count      = inode_table.i_links_count;
        char * tchange        = create_time((time_t)inode_table.i_ctime);
        char * tmod           = create_time((time_t)inode_table.i_mtime);
        char * taccess        = create_time((time_t)inode_table.i_atime);
        __u32 file_size       = inode_table.i_size;
        __u32 blocks_count    = inode_table.i_blocks;
        
        printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
               inode_num,
               file_type,
               mode,
               owner,
               group,
               link_count,
               tchange,
               tmod,
               taccess,
               file_size,
               blocks_count);
        
        free(tchange); free(tmod); free(taccess);
        
        if ( (file_type == 'f') ||
            (file_type == 'd') ||
            ((file_type == 's') && (file_size > 60)) )
        {
            int k;
            for (k=0; k<EXT2_N_BLOCKS; k++)
            {
                printf(",%d", inode_table.i_block[k]);
            }
        }
        printf("\n");
        
        // directory entries
        /* 1. DIRENT
         * 2. parent inode number (decimal) ... the I-node number of the directory that contains this entry
         * 3. logical byte offset (decimal) of this entry within the directory
         * 4. inode number of the referenced file (decimal)
         * 5. entry length (decimal)
         * 6. name length (decimal)
         * 7. name (string, surrounded by single-quotes)
         */
        if (file_type == 'd')
        {
            unsigned int t;
            for (t = 0; t < EXT2_NDIR_BLOCKS; t++)
            {
                if (inode_table.i_block[t] != 0)
                {
                    dir_block(inode_table.i_block[t], inode_num);
                }
            }
        }
        // indirect block references
        /* 1. INDIRECT
         * 2. I-node number of the owning file (decimal)
         * 3. (decimal) level of indirection for the block being scanned ...
         *    1 for single indirect, 2 for double indirect, 3 for triple
         * 4. logical block offset (decimal) represented by the referenced block.
         *    If the referenced block is a data block, this is the logical block
         *    offset of that block within the file. If the referenced block is a
         *    single- or double-indirect block, this is the same as the logical
         *    offset of the first data block to which it refers.
         * 5. block number of the (1, 2, 3) indirect block being scanned (decimal)
         *    . . . not the highest level block (in the recursive scan), but the
         *    lower level block that contains the block reference reported by this entry.
         * 6. block number of the referenced block (decimal)
         */
        //first 12 items in the array are for direct pointers
        //13 is the indirect pointer  EXT2_IND_BLOCK
        logical_block = 12;
        if (inode_table.i_block[EXT2_IND_BLOCK] != 0)
        {
            indirect_block(inode_num, 1, 1, inode_table.i_block[EXT2_IND_BLOCK], file_type);
        }
        else
        {
            logical_block += block_size/sizeof(__u32);
        }
        //14 is the double indirect pointer  EXT2_DIND_BLOCK
        if (inode_table.i_block[EXT2_DIND_BLOCK] != 0)
        {
            indirect_block(inode_num, 2, 2, inode_table.i_block[EXT2_DIND_BLOCK], file_type);
        }
        else
        {
            logical_block += (block_size/sizeof(__u32)) * (block_size/sizeof(__u32));
        }
        //15 is the triple indirect pointer  EXT2_TIND_BLOCK
        if (inode_table.i_block[EXT2_TIND_BLOCK] != 0)
        {
            indirect_block(inode_num, 3, 3, inode_table.i_block[EXT2_TIND_BLOCK], file_type);
        }
    }
    exit(0);
}


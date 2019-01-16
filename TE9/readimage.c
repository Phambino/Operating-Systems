#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;

void printitable(struct ext2_inode* itable, int id) {
    struct ext2_inode *cinodetable = &itable[id - 1];
    char type;

    if(cinodetable->i_mode & EXT2_S_IFREG){
	type = 'f';
    }
    else if(cinodetable->i_mode & EXT2_S_IFDIR){
	type = 'd';
    }
    else if(cinodetable->i_mode & EXT2_S_IFLNK){
	type = 'l';
    }
  
    printf("[%d] type: %c size: %d links: %d blocks %d\n",id, type, cinodetable->i_size, cinodetable->i_links_count, cinodetable->i_blocks);
    printf("[%d] Blocks: ", id);
    for(int i = 0; i < cinodetable->i_blocks/2; i++){
	printf(" %d", cinodetable->i_block[i]);
    } 
    printf("\n");

}

void printidir(struct ext2_inode* itable, int id) {
    struct ext2_inode *cinodetable = &itable[id - 1];
    char type;
    
    for(int i = 0; i < cinodetable->i_blocks/2;i++) {
	int j = 0;
	printf("	DIR BLOCK NUM: %d (for inode %d)\n", cinodetable->i_block[i],id);
	while(j < EXT2_BLOCK_SIZE) {
	    struct ext2_dir_entry *ent = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * cinodetable->i_block[i] + j);
	    if(ent->file_type & EXT2_FT_REG_FILE) {
		type = 'f';
	    }
	    else if(ent->file_type & EXT2_FT_DIR) {
		type = 'd';
	    }
	    else if(ent->file_type & EXT2_FT_SYMLINK) {
		type = 's';
	    }
	    printf("Inode: %d rec_len: %d name_len: %d type= %c name=%.*s\n", ent->inode, ent->rec_len, ent->name_len, type, ent->name_len, ent->name);
	    j+= ent->rec_len;
        printf("%d\n",j);

	}
    }
}

int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);

    struct ext2_group_desc *gdesc = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE * 2));
    printf("Block group:\n");
    printf("	block bitmap: %d\n", gdesc[0].bg_block_bitmap);
    printf("	inode bitmap: %d\n", gdesc[0].bg_inode_bitmap);
    printf("	block table: %d\n", gdesc[0].bg_inode_table);
    printf("	free blocks: %d\n", gdesc[0].bg_free_blocks_count);
    printf("	free inodes: %d\n", gdesc[0].bg_free_inodes_count);
    printf("	used_dirs: %d\n", gdesc[0].bg_used_dirs_count);

    printf("Block bitmap: ");
    char *blockbit = (char*)(disk + (EXT2_BLOCK_SIZE * gdesc->bg_block_bitmap));
    for(int byte = 0; byte < sb->s_blocks_count/8; byte++) {
	for(int bit = 0; bit < 8;bit++) {
	    int in_use = blockbit[byte] & (1 << bit);
	    printf(in_use ? "1" : "0");
	}
	printf(" ");
    }
    printf("\n");

    printf("Inode bitmap: ");
    char *inodebit = (char*)(disk + (EXT2_BLOCK_SIZE * gdesc->bg_inode_bitmap));
    for(int byte = 0; byte < sb->s_inodes_count/8; byte++) {
	for(int bit = 0; bit < 8;bit++) {
	    int in_use = inodebit[byte] & (1 << bit);
	    printf(in_use ? "1" : "0");
	}
	printf(" ");
    }
    printf("\n\n");

    struct ext2_inode *inodetable = (struct ext2_inode *)(disk + (EXT2_BLOCK_SIZE * gdesc->bg_inode_table));
  
    printf("Inodes:\n");
    printitable(inodetable, EXT2_ROOT_INO);

    for(int i = sb->s_first_ino; i < sb->s_inodes_count; i++) {
	int ibyte = i/8;
	int ibytei = i - (8 * ibyte);
 	if(inodebit[ibyte] & (1 << ibytei)) {
	    printitable(inodetable, i+1);
	}
    }

    printf("\nDirectory Blocks:\n");
    printidir(inodetable,EXT2_ROOT_INO);
    for(int i = sb->s_first_ino; i < sb->s_inodes_count; i++) {
	int ibyte2 = i/8;
	int ibytei2 = 1 - (8 * ibyte2);
	if(inodebit[ibyte2] & (1 << ibytei2)) {
	    struct ext2_inode *k = &inodetable[i];
	    char ktype;
    	    if(k->i_mode & EXT2_S_IFREG){
		ktype = 'f';
    	    }
    	    else if(k->i_mode & EXT2_S_IFDIR){
		ktype = 'd';
    	    }
    	    else if(k->i_mode & EXT2_S_IFLNK){
		ktype = 'l';
    	    }
	    if(ktype == 'd') {
		printidir(inodetable, i + 1);
	    }
	}
    }
    return 0;
}

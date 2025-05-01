/*
    implementation of API
*/

#include "def.h"
#include <pthread.h>

pthread_mutex_t mutex_for_fs_stat; // mutex used by RSFS_stat()

// initialize file system - should be called as the first thing before accessing
// this file system
int RSFS_init() {
  char *debugTitle = "RSFS_init";

  // initialize data blocks
  for (int i = 0; i < NUM_DBLOCKS; i++) {
    void *block = malloc(BLOCK_SIZE); // a data block is allocated from memory
    if (block == NULL) {
      printf("[%s] fails to init data_blocks\n", debugTitle);
      return -1;
    }
    data_blocks[i] = block;
  }

  // initialize bitmaps
  for (int i = 0; i < NUM_DBLOCKS; i++)
    data_bitmap[i] = 0;
  pthread_mutex_init(&data_bitmap_mutex, NULL);
  for (int i = 0; i < NUM_INODES; i++)
    inode_bitmap[i] = 0;
  pthread_mutex_init(&inode_bitmap_mutex, NULL);

  // initialize inodes
  for (int i = 0; i < NUM_INODES; i++) {
    inodes[i].length = 0;
  }
  pthread_mutex_init(&inodes_mutex, NULL);

  // initialize open file table
  for (int i = 0; i < NUM_OPEN_FILE; i++) {
    struct open_file_entry entry = open_file_table[i];
    entry.used = 0; // each entry is not used initially
    pthread_mutex_init(&entry.entry_mutex, NULL);
    entry.position = 0;
    entry.access_flag = -1;
    // entry.ref=0;
    entry.inode_number = -1;
  }
  pthread_mutex_init(&open_file_table_mutex, NULL);

  // initialize root inode
  root_inode_number = allocate_inode();
  if (root_inode_number < 0) {
    printf("[%s] fails to allocate root inode\n", debugTitle);
    return -1;
  }
  pthread_mutex_init(&root_dir_mutex, NULL);

  // initialize mutex_for_fs_stat
  pthread_mutex_init(&mutex_for_fs_stat, NULL);

  // return 0 means success
  return 0;
}

// create file
// if file does not exist, create the file and return 0;
// if file_name already exists, return -1;
// otherwise (other errors), return -2.
int RSFS_create(char file_name) {

  // search root_dir for dir_entry matching provided file_name
  struct dir_entry *dir_entry = search_dir(file_name);

  if (dir_entry) { // already exists
    printf("[create] file (%c) already exists.\n", file_name);
    return -1;
  } else {

    if (DEBUG)
      printf("[create] file (%c) does not exist.\n", file_name);

    // get a free inode
    char inode_number = allocate_inode();
    if (inode_number < 0) {
      printf("[create] fail to allocate an inode.\n");
      return -2;
    }
    if (DEBUG)
      printf("[create] allocate inode with number:%d.\n", inode_number);

    // insert (file_name, inode_number) to root directory entry
    dir_entry = insert_dir(file_name, inode_number);
    if (DEBUG)
      printf("[create] insert a dir_entry with file_name:%c.\n",
             dir_entry->name);

    return 0;
  }
}

// delete file
int RSFS_delete(char file_name) {

  char debug_title[32] = "[RSFS_delete]";

  // to do: find the corresponding dir_entry
  struct dir_entry *dir_entry = search_dir(file_name);
  if (dir_entry == NULL) {
    printf("%s director entry does not exist for file (%c)\n", debug_title,
           file_name);
    return -1;
  }

  // to do: find the corresponding inode
  int inode_number = dir_entry->inode_number;
  if (inode_number < 0 || inode_number >= NUM_INODES) {
    printf("%s inode number (%d) is invalid.\n", debug_title, inode_number);
    return -2;
  }
  struct inode *inode = &inodes[inode_number];

  // to do: find the data blocks, free them in data-bitmap
  pthread_mutex_lock(&data_bitmap_mutex);
  for (int i = 0; i <= inode->length / BLOCK_SIZE; i++) {
    int block_number = inode->block[i];
    if (block_number >= 0)
      data_bitmap[block_number] = 0;
  }
  pthread_mutex_unlock(&data_bitmap_mutex);

  // to do: free the inode in inode-bitmap
  pthread_mutex_lock(&inode_bitmap_mutex);
  inode_bitmap[inode_number] = 0;
  pthread_mutex_unlock(&inode_bitmap_mutex);

  // to do: free the dir_entry
  int ret = delete_dir(file_name);

  return 0;
}

// print status of the file system
void RSFS_stat() {

  pthread_mutex_lock(&mutex_for_fs_stat);

  printf("\nCurrent status of the file system:\n\n %16s%10s%10s\n", "File Name",
         "Length", "iNode #");

  // list files
  for (int i = 0; i < BLOCK_SIZE / sizeof(struct dir_entry); i++) {
    struct dir_entry *dir_entry = (struct dir_entry *)root_data_block + i;
    if (dir_entry->name == 0)
      continue;

    int inode_number = dir_entry->inode_number;
    struct inode *inode = &inodes[inode_number];

    printf("%16c%10d%10d\n", dir_entry->name, inode->length, inode_number);
  }

  // data blocks
  int db_used = 0;
  for (int i = 0; i < NUM_DBLOCKS; i++)
    db_used += data_bitmap[i];
  printf("\nTotal Data Blocks: %4d,  Used: %d,  Unused: %d\n", NUM_DBLOCKS,
         db_used, NUM_DBLOCKS - db_used);

  // inodes
  int inodes_used = 0;
  for (int i = 0; i < NUM_INODES; i++)
    inodes_used += inode_bitmap[i];
  printf("Total iNode Blocks: %3d,  Used: %d,  Unused: %d\n", NUM_INODES,
         inodes_used, NUM_INODES - inodes_used);

  // open files
  int of_num = 0;
  for (int i = 0; i < NUM_OPEN_FILE; i++)
    of_num += open_file_table[i].used;
  printf("Total Opened Files: %3d\n\n", of_num);

  pthread_mutex_unlock(&mutex_for_fs_stat);
}

//------ implementation of the following functions is incomplete
//---------------------------------------------------------

// open a file with RSFS_RDONLY or RSFS_RDWR flags
// return a file descriptor if succeed;
// otherwise return a negative integer value
int RSFS_open(char file_name, int access_flag) {
  char debug_title[32] = "[RSFS_open]";

  // to do: check to make sure access_flag is either RSFS_RDONLY or RSFS_RDWR
  if (access_flag != RSFS_RDONLY && access_flag != RSFS_RDWR) {
    printf("%s access flag not set properly\n", debug_title);
    return -1;
  }

  // to do: find dir_entry matching file_name
  struct dir_entry *dir_entry = search_dir(file_name);
  if (dir_entry == NULL) {
    printf("%s director entry does not exist for file (%c)\n", debug_title,
           file_name);
    return -1;
  }
  // to do: find the corresponding inode
  int inode_number = dir_entry->inode_number;
  if (inode_number < 0 || inode_number >= NUM_INODES) {
    printf("%s inode number (%d) is invalid.\n", debug_title, inode_number);
    return -2;
  }
  // to do: find an unused open-file-entry in open-file-table and fill the
  // fields of the entry properly
  pthread_mutex_lock(&open_file_table_mutex);
  int index = -1;
  for (int i = 0; i < NUM_OPEN_FILE; i++) {
    struct open_file_entry *entry = &open_file_table[i];
    if (entry->used == 0) {
      index = i;
      pthread_mutex_lock(&entry->entry_mutex);
      // printf("Updated entry %d to be used for file (%c)\n", i, file_name);
      entry->used = 1;
      entry->position = 0;
      entry->access_flag = access_flag;
      entry->inode_number = inode_number;
      pthread_mutex_unlock(&entry->entry_mutex);
      break;
    }
  }

  pthread_mutex_unlock(&open_file_table_mutex);
  if (index < 0) {
    printf("%s no unused file entry found\n", debug_title);
    return -3;
  }
  // to do: return the index of the open-file-entry in open-file-table as file
  // descriptor
  return index;
}

// append the content in buf to the end of the file of descriptor fd
// return the number of bytes actually appended to the file
int RSFS_append(int fd, void *buf, int size) {
  char *debug_title = "[RSFS_append]";

  // to do: check the sanity of the arguments:
  //  fd should be in [0,NUM_OPEN_FILE] and size>0.
  if (fd < 0 || fd > NUM_OPEN_FILE || size < 0) {
    printf("%s fd: %d out of limits, or size: %d too small", debug_title, fd,
           size);
    return 0;
  }

  // to do: get the open file entry corresponding to fd
  pthread_mutex_lock(&open_file_table_mutex);
  struct open_file_entry *entry = &open_file_table[fd];
  pthread_mutex_unlock(&open_file_table_mutex);
  // to do: check if the file is opened with RSFS_RDWR mode;
  //  otherwise return 0
  if (entry->access_flag != RSFS_RDWR) {
    printf("%s file not open with RSFS_RDWR", debug_title);
    return 0;
  }

  // to do: get the current position
  int pos = entry->position;

  // to do: get the inode
  int inode_number = entry->inode_number;
  struct inode *cur_inode = &inodes[inode_number];

  // figure out which "block" we are on
  int block_num = pos / BLOCK_SIZE;
  // get that block number
  int offset = pos % BLOCK_SIZE;
  // keep track of how much we have to add
  int to_add = size;
  char *cur_buf = buf;
  // keep track of how much space is left
  int length_left = NUM_POINTERS * BLOCK_SIZE - cur_inode->length;

  // to do: append the content in buf to the data blocks of the file
  //  from the end of the file; allocate new block(s) when needed
  //  - (refer to lecture L22 on how)
  while (to_add > 0 && block_num < NUM_POINTERS && length_left > 0) {
    char i_block = cur_inode->block[block_num];
    char *cur_block;
    int min_addition = BLOCK_SIZE - offset;
    if (i_block < 0) {
      int i;
      for (i = 0; i < NUM_DBLOCKS; i++) {
        if (data_bitmap[i] == 0) {
          pthread_mutex_lock(&data_bitmap_mutex);
          data_bitmap[i] = 1;
          pthread_mutex_unlock(&data_bitmap_mutex);
          break;
        }
      }
      cur_block = data_blocks[i];
      cur_inode->block[block_num] = i;
      offset = 0;
    } else {
      cur_block = data_blocks[i_block];
    }
    if (!cur_block) {
      break;
    }
    if (to_add < min_addition) {
      min_addition = to_add;
    }
    if (length_left < min_addition) {
      min_addition = length_left;
    }
    strncat(cur_block, cur_buf, min_addition);
    cur_buf += min_addition;
    to_add -= (min_addition);
    length_left -= (min_addition);
    cur_inode->length += min_addition;
    if (min_addition == (BLOCK_SIZE - offset)) {
      block_num++;
    }
  }

  // to do: update the current position in open file entry

  pthread_mutex_lock(&entry->entry_mutex);
  entry->position = (pos + (size - to_add));
  pthread_mutex_unlock(&entry->entry_mutex);
  // to do: return the number of bytes appended to the file
  return size - to_add;
}

// update current position of the file (which is in the open_file_entry) to
// offset return -1 if fd is invalid; otherwise return the current position
// after the update
int RSFS_fseek(int fd, int offset) {

  // to do: sanity test of fd; if fd is not valid, return -1
  if (fd < 0 || fd > NUM_OPEN_FILE) {
    return -1;
  }

  // to do: get the correspondng open file entry
  pthread_mutex_lock(&open_file_table_mutex);
  struct open_file_entry *entry = &open_file_table[fd];
  pthread_mutex_unlock(&open_file_table_mutex);

  // to do: get the current position
  int pos = entry->position;

  // to do: get the inode and file length
  int inode_num = entry->inode_number;
  int length = inodes[inode_num].length;

  // to do: check if argument offset is not within 0...length,
  //  do not proceed and return current position
  if (offset < 0 || offset > length) {
    return pos;
  }

  // to do: update the current position to offset, and
  //  return the new current position
  pthread_mutex_lock(&entry->entry_mutex);
  entry->position = offset;
  pthread_mutex_unlock(&entry->entry_mutex);
  return entry->position;
}

// read up to size bytes to buf from file's current position towards the end
// return -1 if fd is invalid; otherwise return the number of bytes actually
// read
int RSFS_read(int fd, void *buf, int size) {

  // to do: sanity test of fd and size (the size should not be negative)
  if (fd < 0 || fd > NUM_OPEN_FILE || size < 0) {
    return -1;
  }

  // to do: get the corresponding open file entry
  pthread_mutex_lock(&open_file_table_mutex);
  struct open_file_entry *entry = &open_file_table[fd];
  pthread_mutex_unlock(&open_file_table_mutex);

  // to do: get the current position
  int pos = entry->position;

  // to do: get the corresponding inode
  int inode_number = entry->inode_number;
  struct inode cur_inode = inodes[inode_number];
  // to do: read from the file
  int block_num = pos / BLOCK_SIZE;
  if (cur_inode.block[block_num] < 0) {
    return 0;
  }
  char *block = data_blocks[cur_inode.block[block_num]];
  int offset = pos % BLOCK_SIZE;

  if (block < 0) {
    return 0;
  }
  int read = 0;
  int total = pos;
  char *info = malloc(size + 1);
  while (read < size && block >= 0 && total < cur_inode.length) {
    for (; offset < BLOCK_SIZE; offset++) {
      info[read] = block[offset];
      read++;
      pos++;
    }
    block_num++;
    if (cur_inode.block[block_num] < 0) {
      break;
    }
    block = data_blocks[cur_inode.block[block_num]];
    offset = 0;
  }
  info[read] = '\0';
  strcat(buf, info);
  free(info);

  // to do: update the current position in open file entry
  pthread_mutex_lock(&entry->entry_mutex);
  entry->position = total;
  pthread_mutex_unlock(&entry->entry_mutex);

  // to do: return the actual number of bytes read
  return read;
}

// close file: return 0 if succeed; otherwise return -1
int RSFS_close(int fd) {
  char debug_title[32] = "[RSFS_close]";

  // to do: sanity test of fd
  if (fd < 0 || fd > NUM_OPEN_FILE) {
    printf("%s fd: %d not in range", debug_title, fd);
    return -1;
  }

  // to do: get the corresponding open file entry
  pthread_mutex_lock(&open_file_table_mutex);
  struct open_file_entry *entry = &open_file_table[fd];
  pthread_mutex_unlock(&open_file_table_mutex);
  // to do: get the corresponding inode
  // int inode_number = entry->inode_number;

  // to do: release this open file entry in the open file table
  pthread_mutex_lock(&entry->entry_mutex);
  entry->used = 0;
  pthread_mutex_unlock(&entry->entry_mutex);
  return 0;
}

// write the content of size (bytes) in buf to the file (of descripter fd)
int RSFS_write(int fd, void *buf, int size) {
  char *debug_title = "[RSFS_write]";

  // to do: check the sanity of the arguments:
  //  fd should be in [0,NUM_OPEN_FILE] and size>0.
  if (fd < 0 || fd > NUM_OPEN_FILE || size < 0) {
    printf("%s fd: %d out of limits, or size: %d too small", debug_title, fd,
           size);
    return 0;
  }

  // to do: get the open file entry corresponding to fd
  pthread_mutex_lock(&open_file_table_mutex);
  struct open_file_entry *entry = &open_file_table[fd];
  pthread_mutex_unlock(&open_file_table_mutex);
  // to do: check if the file is opened with RSFS_RDWR mode;
  //  otherwise return 0
  if (entry->access_flag != RSFS_RDWR) {
    printf("%s file not open with RSFS_RDWR", debug_title);
    return 0;
  }

  // to do: get the inode
  int inode_number = entry->inode_number;
  struct inode *cur_inode = &inodes[inode_number];
  int pos = entry->position;
  int block_num = pos / BLOCK_SIZE;
  int offset = pos % BLOCK_SIZE;

  int written = pos;

  int to_add = size;
  char *cur_buf = buf;
  // keep track of how much space is left
  int length_left = NUM_POINTERS * BLOCK_SIZE - cur_inode->length;

  // to do: append the content in buf to the data blocks of the file
  //  from the end of the file; allocate new block(s) when needed
  //  - (refer to lecture L22 on how)
  while (to_add > 0 && block_num < NUM_POINTERS && length_left > 0) {
    char i_block = cur_inode->block[block_num];
    char *cur_block;
    int min_addition = BLOCK_SIZE - offset;
    if (i_block < 0) {
      int i;
      for (i = 0; i < NUM_DBLOCKS; i++) {
        if (data_bitmap[i] == 0) {
          pthread_mutex_lock(&data_bitmap_mutex);
          data_bitmap[i] = 1;
          pthread_mutex_unlock(&data_bitmap_mutex);
          break;
        }
      }
      cur_block = data_blocks[i];
      cur_inode->block[block_num] = i;
      offset = 0;
    } else {
      cur_block = data_blocks[i_block];
    }
    if (!cur_block) {
      break;
    }
    if (to_add < min_addition) {
      min_addition = to_add;
    }
    if (length_left < min_addition) {
      min_addition = length_left;
    }
    strncpy(cur_block + offset, cur_buf, min_addition);
    cur_buf += min_addition;
    to_add -= (min_addition);
    length_left -= (min_addition);
    written += min_addition;
    if (min_addition == (BLOCK_SIZE - offset)) {
      block_num++;
      offset = 0;
    }
  }
  cur_inode->length = written;
  return size - to_add;
}

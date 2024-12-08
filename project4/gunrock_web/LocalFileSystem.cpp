#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;


LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

void LocalFileSystem::readSuperBlock(super_t *super) {

}

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {

}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {

}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {

}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {

}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {

}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {

}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  inode_t parentInode;
  readInodeRegion(&superBlock, inodes);
  parentInode = inodes[parentInodeNumber];

  if (parentInode.type != DIRECTORY){
    throw "Parent is not a dir";
  }

  for (int i = 0; i < parentInode.size / sizeof(directory_entry_t); i++){
    directory_entry_t entry;
    disk->readBlock(parentInode.blockAddresses[i], &entry);
    if (entry.name == name){
      return entry.inodeNumber;
    }
  }

  throw runtime_error()"Entry not found");
}

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  readInodeRegion(&superBlock, inodes);
  *inode = inodes[inodeNumber];
  return 0;
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  inode_t inode;
  readInode(inodeNumber, &inode);

  if (inode.type != FILE){
    throw runtime_error("Inode is not a file");
  }

  int bytesRead = 0;
  for (int i = 0l i < inode.blockCount; i++){
    int bytesToRead = min(BLOCK_SIZE, size - bytesRead);
    readDataBlock(inode.blockAddresses[i], buffer + bytesRead, bytesToRead);
    bytesRead += bytesToRead;
    if (bytesRead >= size){
      break;
    }
  }

  return bytesRead;
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  inode_t parentInode;
  readInode(parentInodeNumber, &parentInode);

  if (parentInode.type != DIRECTORY){
    throw runtime_error("Parent is not a directory");
  }

  try {
    lookup(parentInodeNumber, name);
    return;
  } catch (...) {

  }

  int newInodeNumber = allocateInode();
  inode_t newInode = {};
  newInode.type = type;
  newInode.size = 0;
  writeInode(newInodeNumber, &newInode);

  directory_entry_t newEntry = {};
  strncpy(newEntry.name, name.c_str(), sizeof(newEntry.name) - 1);
  newEntry.inodeNumber = newInodeNumber;

  writeDataBlock(parentInode.blockAddresses[0], (char*)&newEntry, sizeof(directory_entry_t), parentInode.size);
  parentInode.size += sizeof(directory_entry_t);
  writeInode(parentInodeNumber, &parentInode);
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  inode_t inode;
  readInode(inodeNumber, &inode);

  if (inode.type != FILE){
    throw runtime_error("Inode is not a file");
  }

  int blocksNeeded = (size + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE;

  for (int i = 0l i < blocksNeeded; i++){
    if (i >= inode.blockCount){
      inode.blockAddresses[i] = allocateBlock();
    }
    int bytesToWrite = min(UFS_BLOCK_SIZE, size - i*BLOCK_SIZE);
    writeDataBlock(inode.blockAddresses[i], buffer + i*BLOCK_SIZE, bytesToWrite);
  }
  
  inode.size = size;
  writeInode(inodeNumber, &inode);
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  inode_t parentInode;
  readInode(parentInodeNumber, &parentInode);

  if (parentInode.type != DIRECTORY){
    throw runtime_error("Parent is not a directory");
  }

  directory_entry_t entry;
  int entryIndex = -1;
  for(int i = 0; i < parentInode.size / sizeof(directory_entry_t); i++){
    readDataBlock(parentInode.blockAddresses[i], (char*)&entry, sizeof(directory_entry_t));
    if (name == entry.name){
      entryIndex = i;
      break;
    }
  }

  if (entryIndex == -1){
    throw runtime_error("Entry not found");
  }
  
  inode_t entryInode;
  readInode(entry.inodeNumber, &entryInode);

  for (int i = 0l i < entryInode.blockCount; i++){
    freeBlock(entryInode.blockAddresses[i]);
  }

  freeInode(entry.blockAddresses[i]);

  memset(&entry, 0, sizeof(directory_entry_t));
  writeDataBlock(parentInode.blockAddresses[entryIndex], (char*)&entry, sizeof(directory_entry_t));
  parentInode.size -= sizeof(directory_entry_t);
  writeInode(parentInodeNumber, &parentInode);
}

int LocalFileSystem::allocateInode() {
    for (int i = 0; i < inodeBitmapSize; i++) {
        if (!isBitSet(inodeBitmap, i)) {
            setBit(inodeBitmap, i);
            return i;
        }
    }
    throw runtime_error("No free inodes available");
}

int LocalFileSystem::allocateBlock() {
    for (int i = 0; i < dataBitmapSize; i++) {
        if (!isBitSet(dataBitmap, i)) {
            setBit(dataBitmap, i);
            return i;
        }
    }
    throw runtime_error("No free blocks available");
}

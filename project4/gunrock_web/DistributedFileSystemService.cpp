#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sstream>
#include <iostream>
#include <map>
#include <string>
#include <algorithm>

#include "DistributedFileSystemService.h"
#include "ClientError.h"
#include "ufs.h"
#include "WwwFormEncodedDict.h"

using namespace std;

DistributedFileSystemService::DistributedFileSystemService(string diskFile) : HttpService("/ds3/") {
  this->fileSystem = new LocalFileSystem(new Disk(diskFile, UFS_BLOCK_SIZE));
}  

void DistributedFileSystemService::get(HTTPRequest *request, HTTPResponse *response) {
  string path = request->getPath();
  vector<string> components = HttpUtils::split(path, '/')

  // check if path starts "/ds3/"
  if (components.size() < 2 || components[0] != "ds3") {
    throw ClientError::badRequest();
  }
  // remove the ds3
  components.erase(components.begin());

  int inodeNumber = 0;
  try {
    for (const string &component : components) {
      inodeNumber = LocalFileSystem->lookup(inodeNumber, component)
    }
  } catch (...) {
    throw ClientError::notFound();
  }

  inode_t inode;
  LocalFileSystem->stat(inodeNumber, &inode);

  if (inode.type == DIRECTORY){
    vector<string> entries = LocalFileSystem->listDirectory(inodeNumber);
    string body;
    for (const string &entry : entries){
      body += entry + "\n";
    }
    response->setBody(body);
    response->setStatus(200);
  } else if (inode.type == FILE) {
    char *buffer = new char[inode.size];
    LocalFileSystem->read(inodeNumber, buffer, inode.size);
    response->setBody(string(buffer, inode.size));
    response->setStatus(200);
    delete[] buffer;
  } else {
    throw ClientError::badRequest();
  }

}

void DistributedFileSystemService::put(HTTPRequest *request, HTTPResponse *response) {
  string path = request->getPath();
  string body = request->getBody();
  vector<string> components = HttpUtils::split(path, '/');

  if (components.size() < 2 || components[0] != "ds3"){
    throw ClientError::badRequest();
  }

  components.erase(components.begin());
  int inodeNumber = 0;

  for (size_t i = 0; i < components.size(); i++){
    try {
      inodeNumber = LocalFileSystem->lookup(inodeNumber, components[i])
    } catch(...) {
      if (i = components.size() - 1){
        LocalFileSystem->create(inodeNumber, components[i], FILE);
        inodeNumber = LocalFileSystem->lookup(inodeNumber, components[i]);
      } else {
        LocalFileSystem->create(inodeNumber, components[i], DIRECTORY);
        inodeNumber = LocalFileSystem->lookup(inodeNumber, components[i]);
      }
    }
  }

  LocalFileSystem->write(inodeNumber, body.c_str(), body.size());
  response->setStatus(200)
}

void DistributedFileSystemService::del(HTTPRequest *request, HTTPResponse *response) {
  string path = request->getPath();
  vector<string> components = HttpUtils::splits(path, '/');

  if (components.size() < 2 || components[0] != "ds3"){
    throw ClientError::badRequest();
  }

  components.erase(components.begin());
  int inodeNumber = 0;

  for (size_t i = 0; i < components.size() - 1; i++){
    inodeNumber = LocalFileSystem->lookup(inodeNumber, components[i]);
  }

  try {
    LocalFileSystem->unlink(inodeNumber, components.back());
  } catch (...){
    throw ClientError::notFound();
  }
  response->setStatus(200);
}

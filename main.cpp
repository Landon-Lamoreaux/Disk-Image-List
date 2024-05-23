#include <iostream>
#include <cstring>
#include "sfs_dir.h"
#include "sfs_inode.h"
#include "sfs_superblock.h"
#include <string>
#include <time.h>
#include <iomanip>

using namespace std;

extern "C"{
#include "driver.h"
}

void GetFileBlock(sfs_inode_t * node, size_t blockNumber, void* data);

void printLongList(sfs_dirent file, int superInode);

void findTime(time_t inTime, char* outTime);

int main(int argv, char** argc) {

    bool longList = false;
    int i, j = 0;

    if (argv == 3)
    {
        if ((string)argc[2] == "-l")
            longList = true;
    }
    if (argv > 3 || argv < 2) {
        cout << "Incorrect number of arguments" << endl;
        return -1;
    }

    /* declare a buffer that is the same size as a filesystem block */
    char raw_superblock[128];

    sfs_superblock *super = (sfs_superblock *) raw_superblock;
    sfs_inode inode[128/sizeof(sfs_inode)];

    string diskImage = argc[1];
    char* disk = const_cast<char *>(diskImage.c_str());

    /* open the disk image and get it ready to read/write blocks */
    driver_attach_disk_image(disk, 128);

    // Finding the super block.
    while(true) {

        // Read a block from the disk image.
        driver_read(super,i);

        // Is it the filesystem superblock?
        if(super->fsmagic == VMLARIX_SFS_MAGIC && !strcmp(super->fstypestr,VMLARIX_SFS_TYPESTR))
        {
            break;
        }
        i++;
    }

    sfs_dirent data[128/sizeof(sfs_dirent)];

    driver_read(inode, super->inodes);
    int extra = inode[0].size % 128 == 0 ? 0 : 1;

    for (i = 0; i < inode[0].size / 128 + extra; i++)
    {
        GetFileBlock(&inode[0], i, data);
        j = 0;
        while (j < 128/sizeof(sfs_dirent) && i * 4 + j < super->num_inodes - super->inodes_free + 1 && !((string)data[j].name).empty())
        {
            if (longList)
                printLongList(data[j], (int)super->inodes);
            cout << data[j].name << endl;
            j++;
        }
    }

    /* close the disk image */
    driver_detach_disk_image();

    return 0;
}

void printLongList(sfs_dirent file, int superInode)
{
    string permissions = "rwxrwxrwx";
    int mask = 0b100000000;
    int i = 0;
    sfs_inode node[128/sizeof(sfs_inode)];
    char time[40];
    int fileNum;
    string size;

    driver_read(node, file.inode / 2 + superInode);

    fileNum = file.inode % 2 == 0 ? 0 : 1;

    switch(node[fileNum].type)
    {
        case 0:
            cout << "-";
            break;
        case 1:
            cout << "d";
            break;
        case 2:
            cout << "c";
            break;
        case 3:
            cout << "b";
            break;
        case 4:
            cout << "p";
            break;
        case 5:
            cout << "s";
            break;
        case 6:
            cout << "l";
            break;
    }

    for (i = 0; i < 9; i++)
    {
        if (mask & node[fileNum].perm)
            cout << permissions[i];
        else
            cout << "-";
        mask = mask >> 1;
    }

    if (node[fileNum].type == 2 || node[fileNum].type == 3)
        size = to_string(node[fileNum].direct[0]) + ", " + to_string(node[fileNum].direct[1]);
    else
        size = to_string(node[fileNum].size);

    cout << " " << (int)node[fileNum].refcount << " " << setw(2) << right;
    cout << (int)node[fileNum].owner << " " << setw(2) << (int)node[fileNum].group << " " << setw(5);
    cout << size << " " << setw(20);
    findTime(node[fileNum].mtime, time);
    cout << time << " " << setw(10) << left;

}

void findTime(time_t inTime, char* outTime)
{
struct tm * timeTime;
timeTime = localtime(&inTime);
strftime(outTime, 40, "%b %d %H:%M %Y", timeTime);
}


void GetFileBlock(sfs_inode_t * node, size_t blockNumber, void* data)
{
    uint32_t ptrs[128];

    // direct
    if (blockNumber < 5)
    {
        //printf("pass\n");
        driver_read(data, node->direct[blockNumber]);
    }
        // indirect
    else if (blockNumber < (5 + 32))
    {
        driver_read(ptrs, node->indirect);
        driver_read(data, ptrs[blockNumber - 5]);
    }
        // double indirect
    else if (blockNumber < (5 + 32 + (32 * 32)))
    {
        driver_read(ptrs, node->dindirect);
        int tmp = (blockNumber - 5 - 32) / 32;
        driver_read(ptrs, ptrs[tmp]);
        tmp = (blockNumber - 5 - 32) % 32;
        driver_read(data, ptrs[tmp]);
    }
        // triple indirect
    else if (blockNumber < (5 + 32 + (32 * 32) + (32 * 32 * 32)))
    {
        driver_read(ptrs, node->tindirect);
        int tmp = (blockNumber - 5 - 32 - (32 * 32)) / (32 * 32);
        driver_read(ptrs, ptrs[tmp]);
        tmp = ((blockNumber - 5 - 32 - (32 * 32)) / 32) % 32;
        driver_read(ptrs, ptrs[tmp]);
        tmp = (blockNumber - 5 - 32 - (32 * 32)) % 32;
        driver_read(data, ptrs[tmp]);
    }
    else
    {
        printf("Error in block fetch, out of range\n");
        exit(1);
    }
}
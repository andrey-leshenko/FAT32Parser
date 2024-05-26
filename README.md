# FAT32 Disk Parser

On Linux we can open the disk device itself (like `/dev/sda`) and read all the raw bytes.
This utility does just that, parsing the FAT32 filesystem and being able to list directories and print files.

## Example usage

Be sure to run as root to be able to read the raw device.

```
$ make
gcc main.c -g -Wall -o main

$ ./main
usage: ./main DEV [PART [PATH]]

$ ./main /dev/sda
---------- MBR ----------
bootstrap_code: ...
uid: 003c0c10
reserved: 0000
partition 0:
    boot_indicator: 00
    sysid: 0c (FAT32)
    rel_sector: 00000800
    sectors: 001dc800
partition 1:
    (empty)
partition 2:
    (empty)
partition 3:
    (empty)
signature: aa55

$ ./main /dev/sda 0
dirents in cluster 128
--V--          0          0 USBSTIC.K   
HS-D-          3          0 SYSTEM~1     System Volume Information
---D-        185          0 DISK_P~1     disk_parser
---D-         53          0 DIR1         dir1
----A          6      28318 NAH.ZIP      Nah.zip
---D-        181          0 TRASH-~1     .Trash-1000
----A         54     508894 THIS_I~1     this_is_a_simple_test_file
----A         13       1476 BOOTEX.LOG  
----A          0          0 12345678.TXT 12345678.txt
----A          0          0 123456~1     12345678abc

$ ./main /dev/sda 0 dir1
dirents in cluster 128
---D-         53          0 .           
---D-          0          0 ..          
---D-        179          0 DIR2         dir2
----A          0          0 A.B          a.b

$ ./main /dev/sda 0 dir1/dir2
dirents in cluster 128
---D-        179          0 .           
---D-         53          0 ..          
----A        180         30 FILE         file

$ ./main /dev/sda 0 dir1/dir2/file
I am a simple text file!
$ 
```

## Useful manual commands

```sh
sudo xxd /dev/sda | less
sudo dd if=/dev/sda of=/dev/stdout bs=512 skip=$((0x800)) count=1 | xxd
```

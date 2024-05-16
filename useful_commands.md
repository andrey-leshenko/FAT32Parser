```sh
sudo xxd /dev/sda | less
sudo dd if=/dev/sda of=/dev/stdout bs=512 skip=$((0x800)) count=1 | xxd
```
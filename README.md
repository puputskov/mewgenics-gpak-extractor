# Mewgenics Data Extractor


# Usage
```batch
mew.exe "%MEWGENICS%\resources.gpak"
```

![Usage](./docs/usage.gif)

# How to build
```batch
cl extract.c /Femew.exe
```

## .gpak File Format
Mewgenics packs all the game inside of `resources.gpak` file.

File format itself is very trivial to extract, file starts with 4-bytes that
tells you how many files are inside of the .gpak file after that comes what I
call the file info section.

File info sections consists of N file info structs where the N is the number of
files in the .gpak file. File info struct has first 2-bytes for the length of
the filename, then comes the filename and last 4-bytes tell you the size of
the actual file.


After the file info section comes the file data, file data is in exactly same
order as the file info structs in file info section and there are N files one
for each file info struct.

### Header
|field name|size in bytes|desc|
|----------|-------------|----|
| num_files| 4           | How many files in the .gpak file |

### File Info
|field name|size in bytes|desc|
|----------|-------------|----|
| length | 2           | filename length |
| filename | 1 x length | filename including the path |
| file size | 4 | size of the actual file |


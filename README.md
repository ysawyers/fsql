# FSQL

FSQL (File System Query Language) allows you to do more with less by describing large file system operations with a simple to read and expressive language.

![example](./public/simple_query.gif)

## Build Instructions

```
sh build.sh && cp build/src/fsql /usr/local/bin

fsql <source_file>
```

## Query Structure

```
select <select_specifier> <...(path | nested_query)> where <rule> <disk operation> ;
```

### Select specifiers
- `all`: returns a file given a path to a file or the contents (files and directories) within a directory given a path to a directory
- `files`: returns a file given a path to a file or the files within a directory given a path to a directory
- `directories`: returns the directories within a directory given a path to a directory
- `recursive`: returns all the files with a directory and its subdirectories given a path to a directory

### Rules
**NOTE:** Rules can be chained together using and/or keywords
- `extension = "<extension>"`
- `size (< | >) N (B | KB | MB | GB)`

### Disk operations
**NOTE:** Nested queries cannot contain disk operations
- `display`: print the returned contents
- `delete`: delete the returned contents
- `copy <destination_path>`: copy the returned contents to the destination path
- `move <destination_path>`: move the returned contents to the destination path

## Examples

**Cleaning src/ folder by moving header files into a seperate directory**
```
select files 
    "./src" 
where 
    extension = ".hpp" or extension = ".h" 
move "./include";
```

**Nested query example**
```
select all
    (select files "./dump/images" where extension = ".png"),
    (select recursive "~/Desktop/photos")
move "~/Documents/media";
```
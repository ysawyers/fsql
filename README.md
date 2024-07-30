## FSQL

FSQL (File System Query Language) allows you to do more with less by describing large file system operations with a simple to read and expressive language.

## Example

```
SELECT "example_src_dir", "example_file.c" MOVE "~/project";
```

Similar to how SQL uses tables to organize tables, the runtime manages disk clusters which are bundles of files and folders
the user would want to perform operations on like MOVE, COPY, or DELETE.

In this example the `SELECT` statement creates a disk cluster composed of all the files and directories within `example_src_dir/` as well as the `example_file.c` file. Operations like `MOVE` in this case can be performed on this disk cluster, moving all the elements it contains to `~/project`.

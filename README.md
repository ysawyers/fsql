# FSQL

FSQL (File System Query Language) allows you to do more with less by describing large file system operations with a simple to read and expressive language.

## Example

```
SELECT "example_src_dir", "example_file.c" MOVE "~/project";
```

Similar to how SQL uses tables to organize tables, the runtime manages disk clusters which are bundles of files and folders
the user would want to perform operations on like MOVE, COPY, or DELETE.

In this example the `SELECT` statement creates a disk cluster composed of all the files and directories within `example_src_dir/` as well as the `example_file.c` file. Operations like `MOVE` in this case can be performed on this disk cluster, moving all the elements it contains to `~/project`.

## SELECT clause

SELECT is the basis of each query and constructs a disk cluster of files and folders on disk. Paths to files or folders
should be comma delimited after the `SELECT` clause.

```
SELECT "dir1", "dir2", ...;
```

Modifiers can also be added after `SELECT` to specify the kind of elements you want as a part of that disk cluster.

### Only grab files
```
SELECT FILES "dir1", "dir2";
```

### Only grab directories
```
SELECT DIRECTORIES "dir1", "dir";
```

## Nested-query example

Nested queries are also supported but they cannot include modifying clauses (read further ahead).

```
SELECT "dir1", (SELECT FILES "dir2");
```

## FILTERING clauses

Filtering clauses use regex patterns to grab specific files from the cluster.

### INCLUDE example
```
SELECT FILES "project" INCLUDE ".*\.cpp$";
```

### EXCLUDE example
```
SELECT "project" EXCLUDE ".*\.cpp$";
```

## MODIFYING clauses

Modifying clauses will perform filesystem operations and only one can be performed per statement.

### MOVE example
```
SELECT "dir1", "dir2" MOVE "dir3";
```

### COPY example
```
SELECT "dir1", "dir2" COPY "dir3";
```

### DELETE example
```
SELECT ... DELETE;
```

# TODO

- interactive shell
- recursive select modifier

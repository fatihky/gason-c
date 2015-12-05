gason-c
=======

# c implementation of gason
I converted c++ code to c and added some additional features(dynamically
object generation, encoding etc.). Thanks to gason's authors.

## Notes
- This library can encode json object to `char *`. please look at the `gason-c.h`.
- Please take a look at example.c before using this.
- Include gason-c.h and gason-c.c files to your project and compile with `-lm --std=c99`.

<br>

Run `make` to compile `example.c` and `pretty-print.c` files. And run ``make clean`
 to delete example programs.

Example usage of `pretty-print`:<br>
`cat test.json | ./pretty-print -`
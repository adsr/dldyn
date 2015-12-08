dldyn
=====

dldyn uses dyncall[1] to invoke a function in a dynamically opened library.
This is the worst possible way to invoke a function, but it may be helpful in
some cases.

Synopsis
========

    $ # Clone and build
    $ git clone https://github.com/adsr/dldyn.git
    $ cd dldyn
    $ git submodule update --init
    $ make
    $ # Some examples...
    $ ./dldyn -m /usr/local/lib/libpng15.so -f png_get_header_version
      libpng version 1.5.10 - March 29, 2012
    $ ./dldyn -m /lib/x86_64-linux-gnu/libm.so.6 -f cos -r double -d 3.1415926
      -1.000000
    $ echo '["hifromjson"]' > test.json && ./dldyn -m /lib/x86_64-linux-gnu/libjson-c.so.2.0.0 -f json_object_from_file -r pointer -S 512 -p test.json | hexdump -C
      <snip>
      000001d0  68 69 66 72 6f 6d 6a 73  6f 6e 00 00 00 00 00 00  |hifromjson......|
      <snip>

[1] http://www.dyncall.org/

# -*- coding: utf-8 -*-

from cffi import FFI

try:
    system_types = open('inc/system_types.h', 'r').read()
    fuse_types = open('inc/fuse_types.h', 'r').read()
except Exception:
    print('missing types headers!')
else:
    ffibuilder = FFI()
    ffibuilder.set_source("_fuse",
                          r"""
                            #include <fuse3/fuse.h>
                          """, libraries=['fuse3'],
                          define_macros=[("FUSE_USE_VERSION", "32"), ],
                          extra_compile_args=[],
                          extra_link_args=[])
    ffibuilder.cdef(system_types + fuse_types)
    if __name__ == '__main__':
        ffibuilder.compile(verbose=True)

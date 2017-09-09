#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#  Copyright 2017 Brandon Shaheed
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

"""
        hello example. same as libfuse example.
        https://github.com/libfuse/libfuse/blob/master/example/hello.c
"""

import os
import errno
import stat
from fuseme import ffi, lib, fuseme

# app data

HELLO_FILENAME = b'hello'
HELLO_CONTENTS = 'Hello example.\n'.encode('utf8')

# fuse operations


@ffi.callback("void* (struct fuse_conn_info*, struct fuse_config*)")
def hello_init(conn, cfg):
    cfg.kernel_cache = 1
    return ffi.NULL


@ffi.callback("int (char*, struct stat*, struct fuse_file_info*)")
def hello_getattr(path, st, fi):
    path = ffi.string(path)
    rc = 0

    if path == b"/":
        st.st_mode = (stat.S_IFDIR | 0o755)
        st.st_nlink = 2
    elif path[1:] == HELLO_FILENAME:
        st.st_mode = (stat.S_IFREG | 0o444)
        st.st_nlink = 1
        st.st_size = len(HELLO_CONTENTS)
    else:
        rc = -errno.ENOENT

    return rc


@ffi.callback("""int (char*, void*, fuse_fill_dir_t, off_t,
            struct fuse_file_info*, enum fuse_readdir_flags)""")
def hello_readdir(path, buf, filler, offset, info, flags):
    path = ffi.string(path)

    if path != b'/':
        return -errno.ENOENT

    filler(buf, b'.', ffi.NULL, 0, 0)
    filler(buf, b'..', ffi.NULL, 0, 0)
    filler(buf, HELLO_FILENAME, ffi.NULL, 0, 0)

    return 0


@ffi.callback("int (char*, struct fuse_file_info*)")
def hello_open(path, info):
    path = ffi.string(path)

    if path[1:] != HELLO_FILENAME:
        return -errno.ENOENT

    if (info.flags & os.O_ACCMODE) != os.O_RDONLY:
        return -errno.EACCES

    return 0


@ffi.callback("int (char*, char*, size_t, off_t, struct fuse_file_info*)")
def hello_read(path, buf, size, offset, info):
    path = ffi.string(path)

    if path[1:] != HELLO_FILENAME:
        return -errno.ENOENT

    length = len(HELLO_CONTENTS)
    if offset < length:
        if offset + size > length:
            size = length - offset
        lib.memcpy(buf, HELLO_CONTENTS[offset:offset + size], size)
    else:
        size = 0

    return size

HELLO_OPS = {
    'init': hello_init,
    'getattr': hello_getattr,
    'readdir': hello_readdir,
    'open': hello_open,
    'read': hello_read,
}

# entry


def main():
    """ entry """
    # program name, mount target, operations, optionally pass -d to fuse
    # use fusermount3 -u <target> to unmount
    fuseme('hello', 'vfs', HELLO_OPS, debug=False)

if __name__ == '__main__':
    main()

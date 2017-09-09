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
    fuse3 cffi bindings
"""

try:
    from _fuse import ffi, lib
except ImportError as e:
    raise ImportError("{}\nMake sure you ran 'python3 fuse_build.py'."
                      .format(e, __name__))

import sys


def fuseme(name, target, operations, debug=False):
    """ Pass along operations dict to fuse module """
    args = [ffi.new("char[]", name.encode('utf8')),
            ffi.new("char[]", target.encode('utf8')), ]

    if debug:
        args.extend([ffi.new("char[]", b'-d')])

    argv = ffi.new("char *[]", args)
    ops = ffi.new("struct fuse_operations*")
    try:
        for op in operations:
            setattr(ops, op, operations.get(op, ffi.NULL))
    except AttributeError as e:
        sys.stderr.write('invalid fuse operation:', op, '. ({})'.format(e))
    else:
        lib.fuse_main_real(len(args), argv, ops, ffi.sizeof(
            "struct fuse_operations"), ffi.NULL)

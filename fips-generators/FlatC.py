"""
Compiles the fbs files into *.h files
"""
Version = None

import os
import shutil
import platform
import genutil
import subprocess
from mod import log

#-------------------------------------------------------------------------------
def get_flatc_path() :
    path = os.path.dirname(os.path.abspath(__file__))
    if platform.system() == 'Windows' :
        path += '/../tools/win32/'
    elif platform.system() == 'Darwin' :
        path += '/../tools/osx/'
    elif platform.system() == 'Linux' :
        path +=  '/../tools/linux/'
    else :
        error("Unknown host system {}".format(platform.system()))
    return path + 'flatc'

#-------------------------------------------------------------------------------
def run_flatc(input_file, out_hdr) :
    cmd = [
        get_flatc_path(),
        '-c',
        '--gen-includes',
        '-o', os.path.dirname(input_file),
        input_file
    ]
    subprocess.call(cmd)
    shutil.move(os.path.splitext(input_file)[0] + "_generated.h", out_hdr)

#-------------------------------------------------------------------------------
def generate(input_file, out_src, out_hdr) :
    """
    :param input_file:  flatbuffers fbs file
    :param out_src:     must be None
    :param out_hdr:     path for output header files
    """
    if genutil.isDirty(Version, [input_file], [out_hdr]) :
        flatc_path = get_flatc_path()
        run_flatc(input_file, out_hdr)

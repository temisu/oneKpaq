#!/usr/bin/env python3

import os, re, shutil, stat, subprocess, sys, tempfile

if sys.argv[1] in ("-h","--help"):
    subprocess.run([os.getcwd()+"/onekpaq", "--help"])
    sys.exit(0)

finalout = sys.argv[-1]

with tempfile.NamedTemporaryFile(prefix="onekpaq") as tf:
    #print([os.getcwd()+"/onekpaq", *sys.argv[1:-1], tf.name])
    okpout = subprocess.run([os.getcwd()+"/onekpaq", *sys.argv[1:-1], tf.name],
        stdout=subprocess.PIPE,stderr=subprocess.PIPE, check=False)
    sys.stderr.buffer.write(okpout.stderr)
    sys.stderr.buffer.flush()
    sys.stdout.buffer.write(okpout.stdout)
    sys.stdout.buffer.flush()
    okpout.check_returncode()

    mode = int(sys.argv[1])

    out = okpout.stdout.decode('utf-8')
    match = re.search('P offset=([0-9]+) shift=([0-9]+)', out)
    assert match is not None
    offset, shift = (int(match.group(1)), int(match.group(2)))

    #print(offset, shift)

    #print(tf.name)
    subprocess.run([shutil.which('nasm'), "-fbin", "-o", finalout,
                    "onekpaq_stub_lnx32.asm", "-DPAYLOAD_OFF=%d"%offset,
                    "-DONEKPAQ_DECOMPRESSOR_MODE=%d"%mode,
                    "-DONEKPAQ_DECOMPRESSOR_SHIFT=%d"%shift,
                    "-DPAYLOAD_BIN=\"%s\""%tf.name], check=True)
    os.chmod(finalout, os.stat(finalout).st_mode|stat.S_IEXEC)


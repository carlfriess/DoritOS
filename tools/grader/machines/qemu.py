##########################################################################
# Copyright (c) 2009-2016 ETH Zurich.
# All rights reserved.
#
# This file is distributed under the terms in the attached LICENSE file.
# If you do not find this file, copies can be found by writing to:
# ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
##########################################################################

import os, signal, tempfile, subprocess, shutil
import debug, machines
from machines import ARMMachineBase

QEMU_SCRIPT_PATH = 'tools/qemu-wrapper.sh' # relative to source tree
GRUB_IMAGE_PATH = 'tools/grub-qemu.img' # relative to source tree
QEMU_CMD_ARM = 'qemu-system-arm'
QEMU_ARGS_GENERIC = '-nographic -no-reboot'.split()

class QEMUARMMachineBase(ARMMachineBase):
    def __init__(self, options):
        super(QEMUARMMachineBase, self).__init__(options)
        self.child = None
        self.tftp_dir = None
        self.options = options

    def get_coreids(self):
        return range(0, self.get_ncores())

    def get_tickrate(self):
        return None

    def get_boot_timeout(self):
        # shorter time limit for running a qemu test
        # FIXME: ideally this should somehow be expressed in CPU time / cycles
        return 60

    def get_machine_name(self):
        return self.name

    def force_write(self, consolectrl):
        pass

    def get_tftp_dir(self):
        if self.tftp_dir is None:
            debug.verbose('creating temporary directory for QEMU TFTP files')
            self.tftp_dir = tempfile.mkdtemp(prefix='harness_qemu_')
            debug.verbose('QEMU TFTP directory is %s' % self.tftp_dir)
        return self.tftp_dir

    def lock(self):
        pass

    def unlock(self):
        pass

    def setup(self):
        pass

    def _get_cmdline(self):
        raise NotImplementedError

    def _kill_child(self):
        # terminate child if running
        if self.child:
            os.kill(self.child.pid, signal.SIGTERM)
            self.child.wait()
            self.child = None
        self.masterfd = None

    def reboot(self):
        self._kill_child()
        cmd = self._get_cmdline()
        debug.verbose('starting "%s"' % ' '.join(cmd))
        import pty
        (self.masterfd, slavefd) = pty.openpty()
        self.child = subprocess.Popen(cmd, close_fds=True,
                                      stdout=slavefd,
                                      stdin=slavefd)
        os.close(slavefd)
        # open in binary mode w/o buffering
        self.qemu_out = os.fdopen(self.masterfd, 'rb', 0)

    def shutdown(self):
        self._kill_child()
        # try to cleanup tftp tree if needed
        if self.tftp_dir and os.path.isdir(self.tftp_dir):
            shutil.rmtree(self.tftp_dir, ignore_errors=True)
        self.tftp_dir = None

    def get_output(self):
        return self.qemu_out

@machines.add_machine
class QEMUMachineARMv7Uniproc(QEMUARMMachineBase):
    '''Uniprocessor ARMv7 QEMU'''
    name = 'qemu_armv7'

    imagename = "armv7_a15ve_image"

    def get_ncores(self):
        return 1

    def get_bootarch(self):
        return "armv7"

    def get_platform(self):
        return 'a15ve'

    def set_bootmodules(self, modules):
        # store path to kernel for _get_cmdline to use
        self.kernel_img = os.path.join(self.options.builddir,
                                       self.imagename)
        # write menu.lst
        debug.verbose("Writing menu.lst in build directory.")
        menulst_fullpath = os.path.join(self.options.builddir,
                "platforms", "arm", "menu.lst.armv7_a15ve")
        self._write_menu_lst(modules.get_menu_data('/'), menulst_fullpath)

        # produce ROM image
        debug.verbose("Building QEMU image.")
        debug.checkcmd(["make", self.imagename], cwd=self.options.builddir)

    def _get_cmdline(self):
        qemu_wrapper = os.path.join(self.options.sourcedir, QEMU_SCRIPT_PATH)

        return ([qemu_wrapper, '--arch', 'a15ve', '--image', self.kernel_img])

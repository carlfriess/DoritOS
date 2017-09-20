##########################################################################
# Copyright (c) 2014-2016 ETH Zurich.
# All rights reserved.
#
# This file is distributed under the terms in the attached LICENSE file.
# If you do not find this file, copies can be found by writing to:
# ETH Zurich D-INFK, Universitaetstr 6, CH-8092 Zurich. Attn: Systems Group.
##########################################################################

import debug, machines
import subprocess, os, tempfile, pty
from machines import ARMMachineBase
from subprocess_timeout import wait_or_kill

IMAGE_NAME="armv7_omap44xx_image.bin"

@machines.add_machine
class PandaboardMachine(ARMMachineBase):
    '''Machine to run tests on locally attached pandaboard. Assumes your
    pandaboard's serial port is attached to /dev/ttyUSB0'''
    name = 'pandaboard'

    def __init__(self, options):
        super(PandaboardMachine, self).__init__(options)
        self.picocom = None
        self.masterfd = None
        self.tftp_dir = None

    def get_bootarch(self):
        return 'armv7'

    def get_buildarchs(self):
        return ['armv7']

    # pandaboard specifics
    def get_platform(self):
        return 'omap44xx'

    def get_buildall_target(self):
        return "PandaboardES"

    def get_tftp_dir(self):
        if self.tftp_dir is None:
            self.tftp_dir = tempfile.mkdtemp(prefix="panda_")
        return self.tftp_dir

    def set_bootmodules(self, modules):
        menulst_fullpath = os.path.join(self.builddir,
                "platforms", "arm", "menu.lst.armv7_omap44xx")
        self._write_menu_lst(modules.get_menu_data("/"), menulst_fullpath)
        debug.verbose("building proper pandaboard image")
        debug.checkcmd(["make", IMAGE_NAME], cwd=self.builddir)

    def __usbboot(self):
        debug.verbose("Usbbooting pandaboard; press reset")
        debug.verbose("build dir: %s" % self.builddir)
        debug.checkcmd(["make", "usbboot_panda"], cwd=self.builddir)

    def _get_console_status(self):
        # for Pandaboards we cannot do console -i <machine> so we grab full -i
        # output and find relevant line here
        proc = subprocess.Popen(["console", "-i"], stdout=subprocess.PIPE)
        output = proc.communicate()[0]
        assert(proc.returncode == 0)
        output = map(str.strip, output.split("\n"))
        return filter(lambda l: l.startswith(self.get_machine_name()), output)[0]

    def lock(self):
        pass

    def unlock(self):
        pass

    def setup(self, builddir=None):
        self.builddir = builddir

    def reboot(self):
        self.__usbboot()

    def shutdown(self):
        '''shutdown: close picocom'''
        self.picocom.kill()
        # cleanup picocom lock file
        os.unlink("/var/lock/LCK..ttyUSB0")
        self.picocom = None
        self.masterfd = None

    def get_output(self):
        '''Use picocom to get output. This replicates part of
        ETHMachine.lock()'''
        (self.masterfd, slavefd) = pty.openpty()
        self.picocom = subprocess.Popen(
                ["picocom", "-b", "115200", "/dev/ttyUSB0"],
                close_fds=True, stdout=slavefd, stdin=slavefd)
        os.close(slavefd)
        self.console_out = os.fdopen(self.masterfd, 'rb', 0)
        return self.console_out

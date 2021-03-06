##########################################################################
# Copyright (c) 2009, 2010, 2011, 2016, ETH Zurich.
# All rights reserved.
#
# This file is distributed under the terms in the attached LICENSE file.
# If you do not find this file, copies can be found by writing to:
# ETH Zurich D-INFK, Universitaetstr 6, CH-8092 Zurich. Attn: Systems Group.
##########################################################################

import os.path
import builds
import re

class BootModules(object):
    """Modules to boot (ie. the menu.lst file)"""

    def __init__(self, machine):
        self.hypervisor = None
        self.kernel = (None, [])
        self.modules = []
        self.machine = machine

    def set_kernel(self, kernel, args=None):
        self.kernel = (kernel, args if args else [])

    def add_kernel_arg(self, arg):
        self.kernel[1].append(arg)

    def set_hypervisor(self, h):
        self.hypervisor = h

    # does modulespec describe modulename?
    def _module_matches(self, modulename, modulespec):
        if '/' in modulespec: # if the spec contains a path, they must be the same
            return modulespec == modulename
        else: # otherwise, we look at the last part of the name only
            return modulespec == modulename.rsplit('/',1)[-1]

    def add_module(self, module, args=None):

        # Support for build targets with / in their name (e.g. examples/xmpl-spawn)
        module = module.replace('$BUILD', os.path.dirname(self.kernel[0]))

        # XXX: workaround for backwards compatibility: prepend default path
        if not '/' in module:
            assert self.kernel
            module = os.path.join(os.path.dirname(self.kernel[0]), module)
        elif module.startswith('/'):
            # XXX: workaround to allow working around the previous workaround
            module = module[1:]
        self.modules.append((module, args if args else []))

    def add_module_arg(self, modulename, arg):
        for (mod, args) in self.modules:
            if self._module_matches(mod, modulename):
                args.append(arg)

    def del_module(self, name):
        self.modules = [(mod, arg) for (mod, arg) in self.modules
                                   if not self._module_matches(mod, name)]

    def reset_module(self, name, args=[]):
        self.del_module(name)
        self.add_module(name, args)

    def get_menu_data(self, path, root="(nd)"):
        assert(self.kernel[0])
        r = "timeout 0\n"
        r += "title Test image\n"
        r += "root %s\n" % root
        if self.hypervisor:
            r += "hypervisor %s\n" % os.path.join(path, self.hypervisor)
        r += "kernel %s %s\n" % (
                os.path.join(path, self.kernel[0]), " ".join(map(str, self.kernel[1])))
        for (module, args) in self.modules:
            r += "modulenounzip %s %s\n" % (os.path.join(path, module), " ".join(map(str, args)))
        return r

    # what targets do we need to build/install to run this test?
    def get_build_targets(self):
        def mktarget(modname):
            # workaround beehive's multi-core hack: discard everything after the |
            return modname.split('|',1)[0]

        ret = list(set([self.kernel[0]] + [mktarget(m) for (m, _) in self.modules]))

        if self.hypervisor:
            ret.append(self.hypervisor)

        if self.machine.get_bootarch() == "arm_gem5":
            ret.append('arm_gem5_image')
        elif self.machine.get_bootarch() == "armv7_gem5_2":
            ret.append('arm_gem5_image')
        elif self.machine.get_bootarch() == "arm_fvp":
            ret.append('arm_a9ve_image')

        return ret

def default_bootmodules(build, machine):
    """Returns the default boot module configuration for the given machine."""
    # FIXME: clean up / separate platform-specific logic

    a = machine.get_bootarch()
    m = BootModules(machine)

    # set the kernel: cpu driver for our platform
    m.set_kernel("%s/sbin/cpu_%s" % (a, machine.get_platform()), machine.get_kernel_args())
    m.add_module("%s/sbin/cpu_%s" % (a, machine.get_platform()), machine.get_kernel_args())

    m.add_module("%s/sbin/init" % a)

    return m

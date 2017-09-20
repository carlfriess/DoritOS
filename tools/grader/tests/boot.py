import tests
from common import TestCommon
from results import PassFailResult

@tests.add_test
class BootTest(TestCommon):
    '''Basic boot test'''
    name = "hello"

    def get_modules(self, build, machine):
        modules = super(BootTest, self).get_modules(build, machine)
        return modules

    def get_finish_string(self):
        return "Hello, world! from userspace"

    def process_data(self, testdir, rawiter):
        # the test passed iff the last line is the finish string
        lastline = ''
        for line in rawiter:
            lastline = line
        passed = lastline.startswith(self.get_finish_string())
        return PassFailResult(passed)

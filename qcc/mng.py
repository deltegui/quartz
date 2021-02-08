#!/usr/bin/python3

import os
import sys
import pathlib
import subprocess

PATH = pathlib.Path().absolute()
exe = os.system
BUILD_FOLDER = 'build'

class Unittest:
    RELATIVE_PATH = './unittest'

    def clean(self):
        print('Removing unittest build folder!')
        exe(f'rm -rf {PATH}/{Unittest.RELATIVE_PATH}/{BUILD_FOLDER}')

    def build(self):
        print('Building unittests!')
        path = f'{PATH}/{Unittest.RELATIVE_PATH}/{BUILD_FOLDER}'
        exe(f'mkdir {path}')
        p = subprocess.Popen(['cmake', '..'], cwd=f'{path}')
        p.wait()
        p = subprocess.Popen(['make'], cwd=f'{path}')
        p.wait()

    def run(self):
        path = f'{PATH}/{Unittest.RELATIVE_PATH}/{BUILD_FOLDER}'
        exe(f'{path}/qcc_test')


class Compiler:
    CC='clang++'

    def clean(self):
        print('Removing qcc build folder!')
        exe(f'rm -rf {PATH}/{BUILD_FOLDER}')

    def build(self):
        print('Building qcc!')
        path = f'{PATH}/{BUILD_FOLDER}'
        exe(f'mkdir {path}')
        exe(f'{Compiler.CC} ./*.cc -o ./{BUILD_FOLDER}/qcc')

    def run(self):
        if (len(sys.argv) <= 2):
            print('You must pass the file to compile!');
            return
        path = f'{PATH}/{BUILD_FOLDER}'
        exe(f'{path}/qcc {sys.argv[2]}')


targets = [
    Compiler(),
    Unittest(),
]


def clean_all():
    print('Removing all compiled files!')
    for t in targets:
        t.clean()


def build_all():
    print('Build all!')
    for t in targets:
        t.build()


def run_tests():
    print('Running tests...')
    u = Unittest()
    u.build()
    u.run()


def run_qcc():
    print('Running qcc...')
    c = Compiler()
    c.build()
    c.run()


if __name__ == "__main__":
    if (len(sys.argv) <= 1):
        build_all()
        exit(0)

    if (sys.argv[1] == '--clean'):
        clean_all()
        exit(0)

    if (sys.argv[1] == '--test'):
        run_tests()
        exit(0)

    if (sys.argv[1] == '--run'):
        run_qcc()
        exit(0)

    print('Error! Nothing to do!')
    exit(1)
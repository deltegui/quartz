#!/usr/bin/python3

import os
import sys
import pathlib
import subprocess

PATH = pathlib.Path().absolute()
exe = os.system
BUILD_FOLDER = 'build'


def clean_all():
    print('Removing all compiled files!')
    exe(f'rm -rf {PATH}/{BUILD_FOLDER}')


def build_all():
    print('Build all!')
    exe(f'mkdir {PATH}/{BUILD_FOLDER}')
    p = subprocess.Popen(['cmake', '..'], cwd=f'{PATH}/{BUILD_FOLDER}')
    p.wait()
    p = subprocess.Popen(['make'], cwd=f'{PATH}/{BUILD_FOLDER}')
    p.wait()


def run_tests():
    print('Running tests...')
    os.system(f'{PATH}/{BUILD_FOLDER}/test/qcc_test')


def run_qcc():
    print('Running qcc...')
    if (len(sys.argv) <= 2):
        print('You must pass the file to compile!');
        return
    os.system(f'{PATH}/{BUILD_FOLDER}/bin/qcc {sys.argv[2]}')


if __name__ == "__main__":
    if (len(sys.argv) <= 1):
        build_all()
        exit(0)

    if (sys.argv[1] == '--clean'):
        clean_all()
        exit(0)

    if (sys.argv[1] == '--test'):
        build_all()
        run_tests()
        exit(0)

    if (sys.argv[1] == '--run'):
        build_all()
        run_qcc()
        exit(0)

    print('Error! Nothing to do!')
    exit(1)
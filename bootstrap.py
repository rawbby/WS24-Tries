#!/bin/python3

import glob
import os
import sys
from os.path import join, dirname, abspath, exists

if os.name == 'nt':
    import msvcrt
else:
    import fcntl


class FileLock:
    def __init__(self, filename: str):
        self.filename: str = filename

    def __enter__(self):
        self.fp = open(self.filename, 'wb')
        if os.name == 'nt':
            self.fp.seek(0)
            msvcrt.locking(self.fp.fileno(), msvcrt.LK_LOCK, 1)
        else:
            fcntl.flock(self.fp.fileno(), fcntl.LOCK_EX)

    def __exit__(self, _type, value, tb):
        if os.name == 'nt':
            self.fp.seek(0)
            msvcrt.locking(self.fp.fileno(), msvcrt.LK_UNLCK, 1)
        else:
            fcntl.flock(self.fp.fileno(), fcntl.LOCK_UN)
        self.fp.close()


def lock_venv():
    venv_path = abspath(join(dirname(__file__), '.venv'))
    os.makedirs(venv_path, exist_ok=True)
    return FileLock(join(venv_path, 'venv.lock'))


def run(command: [str], cwd=None) -> str:
    from subprocess import Popen, PIPE, STDOUT

    cwd = cwd if cwd is not None else join(dirname(__file__))

    print(' '.join(command), flush=True)
    process = Popen(command, stdout=PIPE, stderr=STDOUT, text=True, cwd=cwd)
    stdout = ''
    for stdout_line in iter(process.stdout.readline, ''):
        stdout += stdout_line
        print(stdout_line, end='', flush=True)
    process.stdout.close()
    status = process.wait()
    if status:
        sys.exit(status)
    return stdout


def ensure_venv():
    venv_path = abspath(join(dirname(__file__), '.venv'))
    if os.name == 'nt':
        venv_executable = join(venv_path, 'Scripts', 'python.exe')
    else:
        venv_executable = join(venv_path, 'bin', 'python')

    with lock_venv():
        if not exists(venv_executable):
            run([sys.executable, '-m', 'venv', '.venv'])
            run([venv_executable, '-m', 'pip', 'install', '--upgrade', 'pip'])

        packages = ['numpy', 'matplotlib', 'pandas']
        upgrade_packages = []
        site_package_dir = glob.glob(join(venv_path, 'lib', 'python*', 'site-packages'))[0]
        for package in packages:
            if not exists(join(site_package_dir, package)):
                upgrade_packages.append(package)

        if len(upgrade_packages) > 0:
            run([venv_executable, '-m', 'pip', 'install', '--upgrade', 'pip'] + upgrade_packages)

    if sys.executable != venv_executable:
        run([venv_executable, __file__] + sys.argv[1:])
        sys.exit()


def main():
    ...


if __name__ == '__main__':
    ensure_venv()
    main()

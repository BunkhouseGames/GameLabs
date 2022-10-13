# Copyright 2019-2020 Directive Games, Inc. All Rights Reserved.
# kamofile.py
# functions to read or write file atomically on windows or linux.
# The intention is to provide something that can work with a similar system in UE4.
#
# There is no requirement to Lock-read-modify-write-unlock.  Rather, lock-write-unlock and lock-read-unlock
# are the only requirements.
#
# There are two implementations here: the rename kind and the locking kind.
#
# Locking is nice, but it is advisory and requires all parties to use the same mechanism.
# On windows, python open() uses underneath a CreateFile() with FILE_SHARE_READ and
# FILE_SHARE_WRITE, which means that there will be no sharing conflict when file is opened.
# but we then lock the file using LockFileEx() which provides a blocking lock.  This is an
# advisory lock so all parties must use this.
# To sync with UE4's own file writes from FFileHelper, that doesn't quite do it.  Because that
# one uses proper FILE_SHARE primitives, but no FileLockEx.
# We could write a version that uses the proper sharing primitives, but then we would need to
# implement our own CreateFile open.  Both the CreateFile() sharing permissions and the LockFile
# advisory locking work over NFS.
#
# On linux, we similarly can implement file locking using fcntl().  This also works over NFS
# and is advisory.  It also allows blocking waits for the lock.  However, UE4 does not natively
# use fcntl, rather it uses flock(), and that, only for files open for write!.  flock() does not
# work over NFS, so this kind of locking is useless for jiving with UE4.
#
# A separate kind of atomic file access is in many ways simpler to implement:  temporary files
# and renames.  Then, a temporary file is created and written to, then renamed to the target
# file in an atomic operation.
# On Unix, this works transparently because of the nature of inodes and file handles on unix.
# a reader of a file is not disturbed if its entry in a directory is replaced with a new file.
# On windows, python refuses to rename() onto an existing file, rather we have to call the win32
# function MoveFileEx() to do it.  This function, again, can get access violations when renaming
# if the destination file is open at the time of rename.
#
# if we want to synchronize with UE4 using locking we have to:
# a) implement fcntl() based locking access in UE4 for unix.
# b) Implement retry logic for CreateFile() in UE4 fo windows
# c) Implement CreateFile() based file operations (with retries on access failure) in python.
#
# However, for us to use rename logic we only have to 
# a) implement MoveFileEx in python and retry logic
# b) Implement MoveFileEx in UE4 for windows and retry logic.
#
# On the balance, a temp file based solution seems simpler.  A further bonus is that such
# an approach works for both Windows and Linux sharing a common folder.


import os
import errno
import contextlib
import builtins
import tempfile
try:
    import win32file
    import win32con
    import pywintypes
    import msvcrt
    windows = True
except ImportError:
    import fcntl
    win32file = None
    windows = False

# windows error codes used
ERROR_ACCESS_DENIED = 5

# define this to make us actually use strict sharing on windows.
# in this case, the extra LockFileEx is actually redundant
# but kept in place for visibility
STRICT_SHARE = True


def atomic_read_rename(filename, binary=False):
    mode = 'rb' if binary else 'r'
    with open_file(filename, mode) as file:
        return file.read()


def atomic_write_rename(filename, data, binary=False):
    path, name = os.path.split(filename)
    mode = 'wb' if binary else 'w'
    with tempfile.NamedTemporaryFile(mode=mode, dir=path, prefix=(name+'.'), suffix='.tmp', delete=False) as tf:
        tmpname = tf.name
        tf.write(data)
    try:
        retry_access(lambda: move_file(tmpname, filename))
    except Exception:
        os.unlink(tmpname)
        raise


def atomic_write_locked(filename, data, binary=False):
    """
    Write the file in one chunk, without disturbing any readers.
    """
    # open file for append, since we only want to truncate it if we get the lock
    mode = 'ab' if binary else 'a'
    with retry_access(lambda: open_file(filename, mode)) as f:
        with Lock(f):
            os.truncate(f.fileno(), 0)
            f.write(data)


def atomic_read_locked(filename, binary=False):
    """
    Read a file in one chunk, without it being overwritten at the same time
    """
    # open file for append, since we only want to truncate it if we get the lock
    mode = 'rb' if binary else 'r'
    with retry_access(lambda: open_file(filename, mode)) as f:
        with Lock(f, exclusive=False):
            return f.read()

def open_lockable(mode='r', *args, **kwargs):
    # return a lockable file
    if STRICT_SHARE:
        return LockableFile(open_file(*args, **kwargs))
    else:
        return LockableFile(builtins.open(*args, **kwargs))



@contextlib.contextmanager
def Lock(f, exclusive=True, blocking=True):
    "Contextmanager to lock a file"
    success = f.lock(exclusive, blocking)
    try:
        yield success
    finally:
        if success:
            f.unlock()


class LockableFileBase(object):
    """
    A class that wraps a file object, but provides lock() and unlock()
    methods to lock the file
    """
    def __init__(self, file):
        self._file = file
    
    def __enter__(self):
        self._file.__enter__()
        return self

    def __exit__(self, *args):
        return self._file.__exit__(*args)

    # delegate the rest to the file object
    def __getattr__(self, attr):
        return getattr(self._file, attr)


class LockableFileWin(LockableFileBase):
    def __init__(self, file):
        super().__init__(file)
        self._osfhandle = msvcrt.get_osfhandle(file.fileno())

    def lock(self, exclusive=True, blocking=True):
        'Acquire a lock'
        flags = win32con.LOCKFILE_EXCLUSIVE_LOCK if exclusive else 0
        if not blocking:
            flags |= win32con.LOCKFILE_FAIL_IMMEDIATELY
        # lock enatire range.  overlapped contains 0 as file offset
        # we provide 64 bits of length for file length.
        ov = pywintypes.OVERLAPPED()
        result = win32file.LockFileEx(self._osfhandle, flags, 0xffffffff, 0xffffffff, ov)
        return result

    def unlock(self):
        "release a lock"
        ov = pywintypes.OVERLAPPED()
        win32file.UnlockFileEx(self._osfhandle, 0xffffffff, 0xffffffff, ov)


class LockableFileUnix(LockableFileBase):   
    def lock(self, exclusive=True, blocking=True):
        "Acquire a lock"
        flags = fcntl.LOCK_EX if exclusive else fcntl.LOCK_SH
        if not blocking:
            flags |= fcntl.LOCK_NB
        try:
            fcntl.lockf(self._file.fileno(), flags)
        except OSError as e:
            if e.errno in (errno.EAGAIN, errno.EACCESS):
                return False
            raise
        return True
        
    def unlock(self):
        "Release a lock"
        fcntl.lockf(self._file.fileno(), fcntl.LOCK_UN)

def open_file_win(filename, mode, *args, **kwargs):
    """
    Open a file with either read sharing or no sharing
    """
    readonly = 'r' in mode
    access = win32file.GENERIC_READ if readonly else win32file.GENERIC_WRITE
    share = win32file.FILE_SHARE_READ if readonly else 0
    create = win32file.OPEN_EXISTING if readonly else win32file.OPEN_ALWAYS
    hfile = win32file.CreateFile(filename, access, share, None, create, 0, None)
    flags = os.O_RDONLY if readonly else os.O_WRONLY
    return os.fdopen(msvcrt.open_osfhandle(hfile.Detach(), flags), mode, *args, **kwargs)

def move_file_win(source, dest):
    win32file.MoveFileEx(source, dest, win32file.MOVEFILE_REPLACE_EXISTING)
 
def retry_access_win(callable, tries=5, sleeptime=0.001):
    '''
    Retry an operation until there is no access conflict
    '''
    for i in range(tries - 1):
        try:
            return callable()
        except pywintypes.error as e:
            if e.winerror != ERROR_ACCESS_DENIED:
                raise
        except PermissionError:
            pass
        time.sleep(sleeptime)
    # final unguarded retry
    return callable()
        
def retry_access_unix(callable, tries=5, sleeptime=0.001):
    """
    retrying is not necssary on windows
    """
    return callable()


if windows:
    retry_access = retry_access_win
    open_file = open_file_win
    move_file = move_file_win
    LockableFile = LockableFileWin

else:
    retry_access = retry_access_unix
    open_file = builtins.open
    move_file = os.rename
    LockableFile = LockableFileUnix

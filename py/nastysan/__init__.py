#!/usr/bin/env python3
# vim: ts=4 sw=4 et
#
# This Works is placed under the terms of the Copyright Less License,
# see file COPYRIGHT.CLL.  USE AT OWN RISK, ABSOLUTELY NO WARRANTY.

import os
import sys
import string
import logging
import traceback

import enum
import errno

import llfuse
import paramiko

logging.basicConfig()
log = logging.getLogger('nastysan')

printable = string.ascii_letters + string.digits + string.punctuation + ' '

def esc(a):
    return ''.join(c if c in printable else r'\x{0:02x}'.format(ord(c)) if c<= '\xff' else c.encode('unicode_escape').decode('ascii') for c in a)

def OOPS(*args, **kw):
    log.fatal("OOPS %s", ' '.join([repr(a) for a in args]), **kw)
    traceback.print_stack(sys.stdout)
    sys.exit(23)

class NoValue(enum.Enum):
    def __repr__(self):
        return '<%s.%s>' % (self.__class__.__name__, self.name)

@enum.unique
class Chars(NoValue):
    a   = 'access'
    c   = 'create'
    k   = 'destroy'
    O   = 'opendir'
    o   = 'open'
    L   = 'lookup'

def chars(*args):
    for a in args:
        for i in range(ord(a[0]), ord(a[1])+1):
            yield chr(i)










class Subsystem(object):

    def __init__(self, dest, logging=None):
        if not logging: logging = log
        self.log = logging.info

    def err(self, errno):
        raise llfuse.FUSEError(errno)

    def destroy(self):
        pass








# Longer comments are from example, will vanish when implemented
class Operations(llfuse.Operations):
    __mapchars = list(chars('az', 'AZ', '09'))
    __map = None

# Must not raise any exceptions (including `FUSEError`)
    def __init__(self, sub):
        super().__init__()
        self.sub = sub

    def isActive(self):
        return not (self.sub is None)

    def log(self, s, *args, **kw):
        if not self.__map:  self.__map_init()

        w = s.split(' ')[0]
        if not w in self.__map:
            print("\n    _ = '%s' # missing mapping" % (w), file=sys.stderr)
            self.__map_add(w)

        c = self.__map[w]
        sys.stdout.write(c)
        sys.stdout.flush()
        self.sub.log('op %s: %s', s, ' '.join(repr(a) for a in args), **kw)
        if not (c in self.__mapc):
            self.__mapc[c]  = s

        if s != self.__mapc[c]:
            OOPS('nonunique mapping:', c, 'maps to', self.__map[c], 'and', s)

    def __map_init(self):
        self.__mapc = {}
        self.__map  = dict((c.value, c.name) for c in Chars)

    def __map_add(self, w):
        while 1:
            c = self.__mapchars.pop()
            if not ( c in self.__mapc ):
                self.__map[w] = c;
                return
        OOPS('out of mapping characters for', w)

    def Enotsupp(self):
        if not hasattr(errno, 'EOPNOTSUPP'):
            self.err(errno.ENOTSUP)
        self.err(errno.EOPNOTSUPP)

    def Eaccess(self):  self.sub.err(errno.EACCES)
    def Einval(self):   self.sub.err(errno.EINVAL)
    def Eio(self):      self.sub.err(errno.EIO)
    def Eisdir(self):   self.sub.err(errno.EISDIR)
    def Enoent(self):   self.sub.err(errno.ENOENT)
    def Enospc(self):   self.sub.err(errno.ENOSPC)
    def Enotdir(self):  self.sub.err(errno.ENOTDIR)
    def Enotempty(self): self.sub.err(errno.ENOTEMPTY)
    def Enotty(self):   self.sub.err(errno.ENOTTY)
    def Eperm(self):    self.sub.err(errno.EPERM)
    def Enoattr(self):  self.sub.err(llfuse.ENOATTR)
    def EFinval(self):  self.sub.err(FUSEError.EINVAL())

    def destroy(self):
        self.log('destroy')
        self.sub.destroy()
        self.sub    = None

# This method will be called when `llfuse.close` has been called
# and the file system is about to be unmounted.
# 
# Since this handler is thus *not* run as part of the main loop,
# it is also *not* called with the global lock acquired (unless
# the caller of `llfuse.close` already holds the lock).
# 
# This method must not raise any exceptions (including
# `FUSEError`, since this method is not handling a particular
# syscall).

    def access(self, ino, mode, ctx):
        self.log('access', ino, mode)
        return False

    def create(self, ino, name, mode, flags, ctx):
        self.log('create', ino, name, mode, flags)
        self.Eio()
        # return (fd,EntryAttributes())
# Create a file with permissions *mode* and open it with *flags*
# 
# The method must return a tuple of the form *(fh, attr)*,
# where *fh* is a file handle like the one returned by `open`
# and *attr* is an `EntryAttributes` instance with the
# attributes of the newly created directory entry.
# 
# Once an inode has been returned by `lookup`, `create`,
# `symlink`, `link`, `mknod` or `mkdir`, it must be kept by the
# file system until it receives a `forget` request for the
# inode. If `unlink` or `rmdir` requests are received prior to
# the `forget` call, they are expect to remove only the
# directory entry for the inode and defer removal of the inode
# itself until the `forget` call.

    def flush(self, fd):
        self.log('flushf', fd)
        self.Eio()
# Handle close() syscall.
# 
# This method is called whenever a file descriptor is closed. It
# may be called multiple times for the same open file (e.g. if
# the file handle has been duplicated).
# 
# If the file system implements locking, this method must clear
# all locks belonging to the file handle's owner.

    def forget(self, inos):
        for a in inos:
            self.log('flushi', a)
        self.Eio()
#      Notify about inodes being removed from the kernel cache
#      
#      *inode_list* is a list of ``(inode, nlookup)`` tuples. This
#      method is called when the kernel removes the listed inodes
#      from its internal caches. *nlookup* is the number of times
#      that the inode has been looked up by calling either of the
#      `lookup`, `create`, `symlink`, `mknod`, `link` or `mkdir`
#      methods.
#      
#      The file system is expected to keep track of the number of
#      times an inode has been looked up and forgotten. No request
#      handlers other than `lookup` will be called for an inode with
#      a lookup count of zero.
#      
#      If the lookup count reaches zero after a call to `forget`, the
#      file system is expected to check if there are still directory
#      entries referring to this inode and, if not, delete the inode
#      itself.
#      
#      If the file system is unmounted, it will may not receive
#      `forget` calls for inodes that are still cached. The `destroy`
#      method may be used to clean up any remaining inodes for which
#      no `forget` call has been received.

    def fsync(self, fd, sync):
        self.log('fsync', a)
        self.Eio()
#      If *sync* is true, only the file contents should be
#      flushed (in contrast to the metadata about the file).

    def fsyncdir(self, fd, sync):
        self.log('fsyncdir', fd, sync)
        self.Eio()
#      Flush buffers for open directory *fd*
#      
#      If *sync* is true, only the directory contents should be
#      flushed (in contrast to metadata about the directory itself).

    def getattr(self, ino):
        self.log('getattr', ino)
        self.Eio()
        #return EntryAttributes()
#      This method should return an `EntryAttributes` instance with
#      the attributes of *inode*. The
#      `~EntryAttributes.entry_timeout` attribute is ignored in this context.

    def getxattr(self, ino, name):
        self.log('getxattr', ino, name)
        self.Enoattr()
        #return ?
#      Return extended attribute value
#      
#      If the attribute does not exist, the method must raise
#      `FUSEError` with an error code of `ENOATTR`.

    def link(self, ino, parent_ino, name):
        self.log('link', ino, parent_ino, name)
        self.Eio()
        #return EntryAttributes()
#      Create a hard link.
#      
#      The method must return an `EntryAttributes` instance with the
#      attributes of the newly created directory entry.
#      
#      Once an inode has been returned by `lookup`, `create`,
#      `symlink`, `link`, `mknod` or `mkdir`, it must be kept by the
#      file system until it receives a `forget` request for the
#      inode. If `unlink` or `rmdir` requests are received prior to
#      the `forget` call, they are expect to remove only the
#      directory entry for the inode and defer removal of the inode
#      itself until the `forget` call.

    def listxattr(self, ino):
        self.log('listxattr', ino)
        return list()
#      Get list of extended attribute names

    def lookup(self, ino, name):
        self.log('lookup', ino, name)
        self.Enoent()
        #return EntryAttributes()
#      Look up a directory entry by name and get its attributes.
#      
#      If the entry *name* does not exist in the directory with inode
#      *parent_inode*, this method must raise `FUSEError` with an
#      errno of `errno.ENOENT`. Otherwise it must return an
#      `EntryAttributes` instance.
#      
#      Once an inode has been returned by `lookup`, `create`,
#      `symlink`, `link`, `mknod` or `mkdir`, it must be kept by the
#      file system until it receives a `forget` request for the
#      inode. If `unlink` or `rmdir` requests are received prior to
#      the `forget` call, they are expect to remove only the
#      directory entry for the inode and defer removal of the inode
#      itself until the `forget` call.
#      
#      The file system must be able to handle lookups for :file:`.`
#      and :file:`..`, no matter if these entries are returned by
#      `readdir` or not.

    def mkdir(self, ino, name, mode, ctx):
        self.log('mkdir', ino, name, mode)
        self.Eperm()
        #return EntryAttributes()
#      Create a directory
#      
#      *ctx* will be a `RequestContext` instance. The method must
#      return an `EntryAttributes` instance with the attributes of
#      the newly created directory entry.
#      
#      Once an inode has been returned by `lookup`, `create`,
#      `symlink`, `link`, `mknod` or `mkdir`, it must be kept by the
#      file system until it receives a `forget` request for the
#      inode. If `unlink` or `rmdir` requests are received prior to
#      the `forget` call, they are expect to remove only the
#      directory entry for the inode and defer removal of the inode
#      itself until the `forget` call.

    def mknod(self, parent_inode, name, mode, rdev, ctx):
        self.log('mknod', ino, name, mode)
        self.Eperm()
        #return EntryAttributes()
#      Create (possibly special) file
#      
#      *ctx* will be a `RequestContext` instance. The method must
#      return an `EntryAttributes` instance with the attributes of
#      the newly created directory entry.
#      
#      Once an inode has been returned by `lookup`, `create`,
#      `symlink`, `link`, `mknod` or `mkdir`, it must be kept by the
#      file system until it receives a `forget` request for the
#      inode. If `unlink` or `rmdir` requests are received prior to
#      the `forget` call, they are expect to remove only the
#      directory entry for the inode and defer removal of the inode
#      itself until the `forget` call.

    def open(self, ino, flags):
        self.log('open', ino, flags)
        self.Eperm()
        #return FD
#      Open a file.
#      
#      *flags* will be a bitwise or of the open flags described in
#      the :manpage:`open(2)` manpage and defined in the `os` module
#      (with the exception of ``O_CREAT``, ``O_EXCL``, ``O_NOCTTY``
#      and ``O_TRUNC``)
#      
#      This method should return an integer file handle. The file
#      handle will be passed to the `read`, `write`, `flush`, `fsync`
#      and `release` methods to identify the open file.

    def opendir(self, ino):
        self.log('opendir', ino)
        self.Eperm()
        #return FD
#      Open a directory.
#      
#      This method should return an integer file handle. The file
#      handle will be passed to the `readdir`, `fsyncdir`
#      and `releasedir` methods to identify the directory.

    def read(self, fd, off, size):
        self.log('read', fd, off, size)
        self.Eperm()
        #return <bytes>
#      This function should return exactly the number of bytes
#      requested except on EOF or error, otherwise the rest of the
#      data will be substituted with zeroes.

    def readdir(self, fd, off):
        self.log('readdir', fd, off)
        self.Eperm()
        #yield((name, EntryAttributes(), off))
#      This method should return an iterator over the contents of
#      directory *fh*, starting at the entry identified by *off*.
#      Directory entries must be of type `bytes`.
#      
#      The iterator must yield tuples of the form :samp:`({name}, {attr},
#      {next_})`, where *attr* is an `EntryAttributes` instance and
#      *next_* gives an offset that can be passed as *off* to start
#      a successive `readdir` call at the right position.
#      
#      Iteration may be stopped as soon as enough elements have been
#      retrieved. The method should be prepared for this case.
#      
#      If entries are added or removed during a `readdir` cycle, they
#      may or may not be returned. However, they must not cause other
#      entries to be skipped or returned more than once.
#      
#      :file:`.` and :file:`..` entries may be included but are not
#      required.

    def readlink(self, ino):
        self.log('readlink', ino)
        self.Eperm()
        #return <string>
#      Return target of symbolic link
#      
#      The return value must have type `bytes`.

    def release(self, fd):
        self.log('release', fd)
        self.Eio()
#      Release open file
#      
#      This method will be called when the last file descriptor of
#      *fh* has been closed. Therefore it will be called exactly once
#      for each `open` call.

    def releasedir(self, fd):
        self.log('releasedir', fd)
        self.Eio()
#      Release open directory
#      
#      This method must be called exactly once for each `opendir` call.

    def removexattr(self, ino, name):
        self.log('removexattr', ino, name)
        self.Enoattr()
#      Remove extended attribute
#      
#      If the attribute does not exist, the method must raise
#      `FUSEError` with an error code of `ENOATTR`.

    def rename(self, ino_old, name_old, ino_new, name_new):
        self.log('rename', ino_old, name_old, ino_new, name_new)
        self.Eio()
#      Rename a directory entry
#      
#      If *name_new* already exists, it should be overwritten.
#      
#      If the file system has received a `lookup`, but no `forget`
#      call for the file that is about to be overwritten, `rename` is
#      expected to only overwrite the directory entry and defer
#      removal of the old inode with the its contents and metadata
#      until the `forget` call is received.

    def rmdir(self, ino, name):
        self.log('rmdir', ino, name)
        self.Eio()
#      Remove a directory
#      
#      If the file system has received a `lookup`, but no `forget`
#      call for this file yet, `unlink` is expected to remove only
#      the directory entry and defer removal of the inode with the
#      actual file contents and metadata until the `forget` call is
#      received.

    def setattr(self, ino, attr):
        self.log('setattr', ino, attr)
        self.Eio()
        #return EntryAttributes()
#      Change attributes of *inode*
#      
#      *attr* is an `EntryAttributes` instance with the new
#      attributes. Only the attributes `~EntryAttributes.st_size`,
#      `~EntryAttributes.st_mode`, `~EntryAttributes.st_uid`,
#      `~EntryAttributes.st_gid`, `~EntryAttributes.st_atime` and
#      `~EntryAttributes.st_mtime` are relevant. Unchanged attributes
#      will have a value `None`.
#      
#      The method should return a new `EntryAttributes` instance
#      with the updated attributes (i.e., all attributes except for
#      `~EntryAttributes.entry_timeout` should be set).

    def setxattr(self, ino, name, value):
        self.log('setxattr', ino, name, value)
        self.Eio()
#      Set an extended attribute.
#      
#      The attribute may or may not exist already.

    def statfs(self):
        self.log('statfs')
        self.Eio()
        #return StatvfsData()
#      The method is expected to return an appropriately filled
#      `StatvfsData` instance.

    def symlink(self, ino, name, target, ctx):
        self.log('symlink', ino, name, target)
        self.Eio()
        #return EntryAttributes()
#      Create a symbolic link
#      
#      *ctx* will be a `RequestContext` instance. The method must
#      return an `EntryAttributes` instance with the attributes of
#      the newly created directory entry.
#      
#      Once an inode has been returned by `lookup`, `create`,
#      `symlink`, `link`, `mknod` or `mkdir`, it must be kept by the
#      file system until it receives a `forget` request for the
#      inode. If `unlink` or `rmdir` requests are received prior to
#      the `forget` call, they are expect to remove only the
#      directory entry for the inode and defer removal of the inode
#      itself until the `forget` call.

    def unlink(self, ino, name):
        self.log('unlink', ino, name)
        self.Eio()
#      Remove a (possibly special) file
#      
#      If the file system has received a `lookup`, but no `forget`
#      call for this file yet, `unlink` is expected to remove only
#      the directory entry and defer removal of the inode with the
#      actual file contents and metadata until the `forget` call is
#      received.
#      
#      Note that an unlinked file may also appear again if it gets a
#      new directory entry by the `link` method.

    def write(self, fd, off, buf):
        self.log('write', fd, off, buf)
        self.Eperm()
        #return len(buf)
#      Write *buf* into *fh* at *off*
#      
#      This method should return the number of bytes written. If no
#      error occured, this should be exactly :samp:`len(buf)`.

def main(*args):
    if len(args)!=1:
        OOPS("please give destination")
    arg = args[0]

    sub = Subsystem(arg)
    ops = Operations(sub)

    opt = []    # root: 'allow_other'

    mnt = os.path.join(os.path.expanduser('~/mnt'), arg)

    llfuse.init(ops, mnt, opt)
    #while ops.isActive():
    #    log.debug('calling llfuse.main')
    # apparently, we do not need to loop here
    llfuse.main()
    # looks like if llfuse.main() returns,
    # everything has come to an end
    llfuse.close()

if __name__ == '__main__':
    main(*sys.argv[1:])


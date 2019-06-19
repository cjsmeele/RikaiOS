/* Copyright 2019 Chris Smeele
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "vfs.hh"
#include "filesystem/filesystem.hh"
#include "process/proc.hh"
#include "filesystem/pipe.hh"

namespace Vfs {

    /**
     * A lock that prevents multiple threads from opening / closing files
     * simultaneously, among other operations.
     *
     * This is a shotgun-approach to preventing synchronisation issues:
     * A significant amount of potential performance is sacrificed, but in
     * return our open() and close() functions can avoid some complexity.
     *
     * To give an idea of what issues we would have to deal with:
     * Consider two threads trying to open the same file simultaneously.
     * The first thread creates the open_file_t structure, since none yet
     * exists. However, the thread needs to read from disk in order to fill the
     * struct. This disk read blocks the thread.
     * Now thread 2 comes in. It finds the not-yet-initialised file struct for
     * the path it's trying to open, so it must wait until thread 1 finishes
     * the initialisation.
     * Thread 1 may find out however that for the open_flags it has, it cannot
     * open the file. Now thread 2 must somehow be given the authority to
     * continue the initialisation. On the other hand, if thread 1 was
     * succesful, it may decide to close the file immediately after open()
     * completes. If thread 2 did not register a file handle, it will have a
     * dangling open file pointer and must redo the entire open() sequence.
     * If thread 2 *did* register a file handle, it has an open file, but needs
     * to somehow mark the handle unusable for the duration of thread 1's open
     * call.
     *
     * This, and more, is avoided by making certain "open" and "close" actions
     * atomic relative to themselves and eachother. i.e. all open and close
     * calls are serialised.
     */
    mutex_t vfs_lock;

    /**
     * Waits until the vfs is unlocked.
     *
     * The kernel is not pre-emptible, so when this function returns, the lock
     * is guaranteed to remain unlocked until the next yield.
     */
    void wait_until_lockable() {
        mutex_lock(vfs_lock);
        mutex_unlock(vfs_lock);
    }

    using namespace FileSystem;

    /// The /dev filesystem holds all device files.
    static DevFs *devfs = nullptr;
    DevFs *get_devfs() { return devfs; }

    /// Every file that is opened (that is: it has one or more handles pointing
    /// to it), has a unique entry in the open_files array.
    static Array<       file_t*, max_open_files>   open_files;

    /// Each file descriptor (fd) has a unique entry in handle array.
    static Array<file_handle_t*, max_file_handles> handles;

    // TODO: -> limits.hh
    static constexpr size_t max_mounts = 32;

    /// Structure representing a mounted filesystem.
    struct mount_t {
        file_name_t     name; /// Mountpoint name right under '/' (e.g. "dev").
        FileSystem::Fs &fs;   /// A unique filesystem instance.

        mount_t(StringView x, FileSystem::Fs &y)
            : name(x), fs(y) { }
    };

    /// A fixed-capacity array of all current mountpoints.
    static Array<mount_t*, max_mounts> mounts;

    /// Find a mount by name.
    static mount_t *get_mount(StringView name) {

        for (mount_t *m : mounts) {
            if (m && m->name == name)
                return m;
        }
        return nullptr;
    }

    errno_t mount(FileSystem::Fs &fs, StringView name) {

        int mount_i = ERR_limit;

        for (auto [i, mount] : enumerate(mounts)) {
            if (mount) {
                if (mount->name == name)
                    return ERR_exists;
            } else if (mount_i == ERR_limit) {
                mount_i = i;
            }
        }

        if (mount_i < 0)
            return mount_i;

        mount_t *m = new mount_t(name, fs);
        if (!m) return ERR_nomem;

        mounts[mount_i] = m;

        klog("vfs: mounted {} on /{} type {}\n"
            ,m->fs.name(), m->name, m->fs.type());

        return ERR_success;
    }

    void dump_mounts() {
        for (mount_t *m : mounts) {
            if (m)
                kprint("  {} on /{} type {}\n", m->fs.name(), m->name, m->fs.type());
        }
    }

    void dump_open_files() {
        kprint("  {-4} {-4} {-5} {-24} {}\n", "NO", "FD", "FLAGS", "PATH", "PROC");
        for (file_handle_t *h : handles) {
            if (h)
                kprint("  { 4} { 4} {05x} {-24} {}\n"
                      , h->handle_i
                      , h->procfd
                      , h->flags
                      , h->file->path
                      ,*h->proc);
        }
    }

    /// Find a child node within a directory node.
    static errno_t lookup(inode_t &node_, inode_t &dest, path_t path) {

        inode_t tmp_node = node_;

        if (path[0] == '/')
            path.pop_front();

        while (path.length()) {
            file_name_t component = "";

            for (char c : path) {
                if (c == '/') break;
                component += c;
            }

            path.pop_front(component.length() + 1);

            assert(node_.fs, "fs not set in inode");
            errno_t err = node_.fs->lookup(tmp_node, component, dest);
            if (err) return err;

            tmp_node = dest;
        }

        assert(dest.fs, "no filesystem set for inode in lookup");

        return ERR_success;
    }

    // Functions that allocate file numbers. {{{

    // None of these functions actually register a number as in-use,
    // that is the responsibility of the caller.

    static int alloc_proc_handle_i() {
        Process::proc_t *proc = Process::current_proc();
        assert(proc, "tried to open a file without a running process");

        for (auto [i, handle] : enumerate(proc->files)) {
            if (i <3)
                // Don't reassign stdin, etc.
                continue;

            if (handle == nullptr)
                return i;
        }

        return ERR_limit;
    }

    static int alloc_handle_i() {

        for (auto [i, handle] : enumerate(handles)) {
            if (handle == nullptr)
                return i;
        }
        return ERR_limit;
    }

    static int find_open_file(const path_t &path) {
        for (auto [i, open_file] : enumerate(open_files)) {
            if (open_file && open_file->path == path)
                return i;
        }
        return -1;
    }

    static int alloc_file_i() {
        for (auto [i, open_file] : enumerate(open_files)) {
            if (open_file == nullptr)
                return i;
        }
        return ERR_limit;
    }

    // }}}

    /// Pop a file/directory name off the right side of a path.
    static void pop_path_component(path_t &path) {
        if (path.length() <= 1)
            return;

        if (path[path.length()-1] == '/')
            path.pop();

        while (path[path.length()-1] != '/')
            path.pop();

        if (path.length() > 1)
            path.pop();
    }

    /**
     * Splits a path into a basename and its parent directory.
     *
     * (path is assumed to be canonicalized)
     *
     * For example:
     *
     * basename("/disk0p0/home/sint/hello.txt")
     *   => "hello.txt"
     *   path is modified to: "/disk0p0/home/sint"
     */
    static file_name_t basename(path_t &path) {
        assert(path.length() && path[0] == '/'
              ,"input path was not canonicalized");

        if (path == "/") return "/";

        ssize_t slash_pos = -1;
        for (ssize_t i : range((ssize_t)path.length()-1, -1, -1)) {
            if (path[i] == '/') {
                slash_pos = i;
                break;
            }
        }
        assert(slash_pos >= 0
              ,"input path was not canonicalized");

        file_name_t base = "";
        for (size_t i : range(slash_pos+1, path.length()))
            base += path[i];

        path.pop(base.length()+1);
        if (path.empty()) path = "/";

        return base;
    }

    path_t canonicalise_path(StringView in) {
        String<max_path_length>      path;
        String<max_file_name_length> cur_component;

        if (!in.length())
            return "/";

        Process::proc_t *proc = Process::current_proc();
        assert(proc, "tried to canonicalise a path without a running process");

        path_t working_directory = proc->working_directory;

        // Appends the current component to path.
        auto push_component = [&] {
            if (cur_component == "..") {
                // Parent directory, pop.
                pop_path_component(path);
            } else if (cur_component == ".") {
                // Current directory, skip.
            } else if (cur_component.length()) {
                if (!path.length() || path[path.length()-1] != '/')
                    path += '/';
                path += cur_component;
            }
            cur_component = "";
        };

        for (auto [i, ch] : enumerate(in)) {
            if (i == 0) {
                if (ch == '/') {
                    // Absolute path.
                    path += '/';
                } else {
                    // Relative path, prepend the working directory.
                    path += working_directory;
                    cur_component += ch;
                }
            } else if (ch == '/') {
                push_component();
            } else {
                cur_component += ch;
            }
        }
        push_component();

        return path;
    }

    /// Fill an inode struct with file information for the given path.
    static errno_t get_inode(const path_t &path, inode_t &dest) {

        dest = inode_t {};

        if (path == "/") {
            // An inode with i 0 and fs nullptr is the root of the vfs.
            // We can go home early today!
            dest.i    = 0;
            dest.type = t_dir;
            dest.perm = 0500;

            return ERR_success;

        } else {
            // First figure out which filesystem is responsible for this path.

            // Make a copy of the path, so we can chop the mountpoint off.
            path_t tmp_path = path;
            tmp_path.pop_front(); // pop leading slash.

            file_name_t mount_name = "";
            for (char c : tmp_path) {
                if (c == '/') break;
                mount_name += c;
            }
            assert(mount_name.length(), "path canonicalisation failed catastrophically");
            tmp_path.pop_front(mount_name.length()+1);

            // We now have, e.g.:
            //   path       = /disk0/home/sint
            //   mount_name = disk0
            //   tmp_path   = home/sint

            mount_t *mount = get_mount(mount_name);

            if (!mount)
                // Mount does not exist.
                return ERR_not_exists;

            FileSystem::Fs &fs = mount->fs;

            if (tmp_path == "")
                // Return the root node directly.
                return fs.get_root_node(dest);

            // Find the file within the root node.
            inode_t root_node;
            errno_t err = fs.get_root_node(root_node);
            if (err) return err;
            return lookup(root_node, dest, tmp_path);
        }
    }

    /// Finds a file and fills a file struct.
    static errno_t make_file_struct(const path_t &path, file_t &dest) {

        dest      = file_t {};
        dest.path = path;

        return get_inode(path, dest.inode);
    }

    fd_t open(int fd, StringView path_, open_flags_t flags) {

        locked_within_scope _(vfs_lock);

        // First of all, add some implied open flags for the convenience of the user.
        if (flags & o_dir)      flags |= o_read ;
        if (flags & o_append)   flags |= o_write;
        if (flags & o_truncate) flags |= o_write;
        if (flags & o_create)   flags |= o_write;

        path_t path = canonicalise_path(path_);

        // kprint("open <{}>\n", path);

        errno_t err = ERR_success;

        // A temporary file struct that will be used when opening a file
        // for which no file_t struct has been allocated yet.
        file_t tmp_file;

        // We might have this file open somewhere already.
        int open_file_i = find_open_file(path);

        file_t *open_file = nullptr;

        if (open_file_i >= 0) {
            // We do! Simply use that one.
            open_file = open_files[open_file_i];

        } else {
            // File is not open yet: We need to find it first.

            // But first, make sure we actually have room for opening a new file.
            open_file_i = alloc_file_i();
            if (open_file_i < 0) return open_file_i;

            // Find the file, make sure it exists.
            err = make_file_struct(path, tmp_file);
            if (err) return err;

            // We succesfully found the file.
            // (we will put this in the open_files list once we're done below)
            open_file = &tmp_file;
        }

        // Okay, we now have an open file.
        // First, verify that the user can open the file with the specified flags.
        if (open_file->inode.type == t_dir && !(flags & o_dir))   return ERR_type;
        if (open_file->inode.type != t_dir &&  (flags & o_dir))   return ERR_type;
        if (open_file->inode.type == t_dir &&  (flags & o_write)) return ERR_type;

        // Here we currently make no distinction between different "users":
        // If either user, group or other is allowed something, we allow it.
        // (the reason we check this at all currently is that it is user-friendly
        //  to deny opening certain devices for writing)
        if ((flags & o_read)  && !(open_file->inode.perm & 0444)) return ERR_perm;
        if ((flags & o_write) && !(open_file->inode.perm & 0222)) return ERR_perm;

        // All checks passed. We can now create a file handle.

        Process::proc_t *proc = Process::current_proc();
        assert(proc, "tried to open a file without a running process");

        // Allocate handle numbers (once again, this may fail if we reach a limit).
        int handle_i      = alloc_handle_i();      if (handle_i      < 0) return handle_i;
        int proc_handle_i = alloc_proc_handle_i(); if (proc_handle_i < 0) return proc_handle_i;

        if (fd >= 0) {
            // User supplied a fd number, try to open a file "in place".
            if ((size_t)fd < proc->files.size()
             && !proc->files[fd]) {
                proc_handle_i = fd;
            } else {
                return ERR_bad_fd;
            }
        }

        // Note that at this point we have not made any permanent changes or allocations.
        // From this point we will actually start allocating memory and registering information.

        file_handle_t *handle = new file_handle_t;
        if (!handle) return ERR_nomem;

        if (open_file == &tmp_file) {
            // We created a temporary new file struct earlier, now that we know
            // the file can be opened, we can register it.

            open_file = new file_t(tmp_file);
            if (!open_file) { delete handle; return ERR_nomem; }

            open_file->file_i       = open_file_i;
            open_files[open_file_i] = open_file;
        }

        // Fill handle struct.
        handle->handle_i = handle_i;
        handle->file     = open_file;
        handle->flags    = flags;
        handle->pos      = 0;
        handle->proc     = proc;
        handle->procfd   = proc_handle_i;

        // Insert the handle in the global list of handles.
        handles[handle_i] = handle;

        // Insert the handle in the list of handles of this file.
        if (open_file->last_handle) {
            open_file->last_handle->next = handle;
            handle->prev = open_file->last_handle;
            open_file->last_handle = handle;
        } else {
            open_file->first_handle = handle;
            open_file->last_handle  = handle;
        }

        // Insert the handle in the list of handles of the current process.
        proc->files[proc_handle_i] = handle;

        // All done!
        return proc_handle_i;
    }

    /// Find a file handle by its file descriptor within the current process.
    static file_handle_t *handle_by_fd(fd_t fd) {
        Process::proc_t *proc = Process::current_proc();
        assert(proc, "no running process");

        if (fd >= 0 && (size_t)fd < proc->files.size())
             return proc->files[fd];
        else return nullptr;
    }

    fd_t duplicate_fd(fd_t dest, fd_t fd) {

        locked_within_scope _(vfs_lock);

        file_handle_t *src = handle_by_fd(fd);
        if (!src) return ERR_bad_fd;

        Process::proc_t *proc = Process::current_proc();

        // Allocate handle numbers.
        int handle_i      = alloc_handle_i();      if (handle_i      < 0) return handle_i;
        int proc_handle_i = alloc_proc_handle_i(); if (proc_handle_i < 0) return proc_handle_i;

        if (dest >= 0) {
            // User supplied a fd number, try to duplicate a file "in place".
            if ((size_t)dest < proc->files.size()
             && !proc->files[dest]) {
                proc_handle_i = dest;
            } else {
                return ERR_bad_fd;
            }
        }

        file_handle_t *handle = new file_handle_t(*src);
        if (!handle) return ERR_nomem;

        handle->handle_i = handle_i;
        handle->procfd   = proc_handle_i;

        // Insert the handle in the global list of handles.
        handles[handle_i] = handle;

        // Insert the handle in the list of handles of this file.
        assert(src->file->last_handle, "invalid file handle for dup");
        src->file->last_handle->next = handle;
        handle->prev = src->file->last_handle;
        src->file->last_handle = handle;

        // Insert the handle in the list of handles of the current process.
        proc->files[proc_handle_i] = handle;

        return proc_handle_i;
    }

    errno_t transplant_fd(Process::proc_t *dst_proc, fd_t dstfd, fd_t srcfd) {

        // Transfer a fd from the current process to another.

        Process::proc_t *src_proc = Process::current_proc();
        assert(src_proc, "no running process");
        assert(dst_proc, "no target process");

        file_handle_t *handle = handle_by_fd(srcfd);
        if (!handle) return ERR_bad_fd;

        if (!mutex_try_lock(handle->lock)) {
            // We cannot wait: Once we yield / return, the dst_proc may be
            // scheduled to run.
            kprint("kernel bug: transplant_fd cannot wait for handle lock\n");
            return ERR_timeout;
        }

        // Steal from the running process.
        src_proc->files[srcfd] = nullptr;
        dst_proc->files[dstfd] = handle;

        handle->proc   = dst_proc;
        handle->procfd = dstfd;

        mutex_unlock(handle->lock);

        return ERR_success;
    }

    errno_t make_pipe(fd_t &in, fd_t &out) {

        locked_within_scope _(vfs_lock);

        Process::proc_t *proc = Process::current_proc();
        assert(proc, "no running process");

        // Allocate file and handle numbers.

        int open_file_i = alloc_file_i();
        if (open_file_i < 0) return open_file_i;

        // We need to do a little juggling to allocate two handle numbers at once.

        int  in_handle_i = alloc_handle_i();
        if ( in_handle_i < 0) return in_handle_i;
        handles[in_handle_i] = (file_handle_t*)1;
        int out_handle_i = alloc_handle_i();
        handles[in_handle_i] = nullptr;
        if (out_handle_i < 0) return out_handle_i;

        int  in_proc_handle_i = alloc_proc_handle_i();
        if ( in_proc_handle_i < 0) return in_proc_handle_i;
        proc->files[in_proc_handle_i] = (file_handle_t*)1;
        int out_proc_handle_i = alloc_proc_handle_i();
        proc->files[in_proc_handle_i] = nullptr;
        if (out_proc_handle_i < 0) return out_proc_handle_i;

        // Allocate structures.

        file_t *file = new file_t;
        if (!file) return ERR_nomem;

        pipe_t *pipe = new pipe_t;
        if (!pipe) { delete file; return ERR_nomem; }

        file_handle_t *in_handle  = new file_handle_t;
        if (! in_handle) { delete pipe; delete file; return ERR_nomem; }
        file_handle_t *out_handle = new file_handle_t;
        if (!out_handle) { delete in_handle; delete pipe; delete file; return ERR_nomem; }

        // Fill handle structs.
         in_handle->handle_i =  in_handle_i;
        out_handle->handle_i = out_handle_i;
         in_handle->file     = file;
        out_handle->file     = file;
         in_handle->flags    = o_read;
        out_handle->flags    = o_write;
         in_handle->pos      = 0;
        out_handle->pos      = 0;
         in_handle->proc     = proc;
        out_handle->proc     = proc;
         in_handle->procfd   =  in_proc_handle_i;
        out_handle->procfd   = out_proc_handle_i;
         in_handle->next     = out_handle;
        out_handle->prev     =  in_handle;

        // Fill file struct.
        file->file_i = open_file_i;
        fmt(file->path, "pipe-{}", pipe);
        file->first_handle =  in_handle;
        file->last_handle  = out_handle;

        // Fill inode struct.
        file->inode.i    = (u64)pipe;
        file->inode.type = t_pipe;
        file->inode.perm = 0600;
        file->inode.size = pipe_t::pipe_size;
        file->inode.fs   = nullptr;

        // Register file and handles.
        open_files[open_file_i] = file;

        handles[ in_handle_i] =  in_handle;
        handles[out_handle_i] = out_handle;

        proc->files[ in_proc_handle_i] =  in_handle;
        proc->files[out_proc_handle_i] = out_handle;

        in  =  in_proc_handle_i;
        out = out_proc_handle_i;

        return ERR_success;
    }

    errno_t close(fd_t fd) {

        locked_within_scope _(vfs_lock);

        Process::proc_t *proc = Process::current_proc();
        assert(proc, "no running process");

        file_handle_t *handle = handle_by_fd(fd);
        if (!handle) return ERR_bad_fd;

        // Wait until nobody else is using the handle.
        mutex_lock(handle->lock);

        proc->files[fd]           = nullptr;
        handles[handle->handle_i] = nullptr;

        file_t *file = handle->file;

        // Update linked handle list for this file.
        if (handle->prev) handle->prev->next = handle->next;
        if (handle->next) handle->next->prev = handle->prev;

        // Update first/last pointers for this file.
        if (file->first_handle == handle) file->first_handle = handle->next;
        if (file->last_handle  == handle) file->last_handle  = handle->prev;

        delete handle;

        if (file->first_handle == nullptr) {
            // We deleted the last handle for this open file.
            // Clean up the file.

            if (file->inode.type == t_pipe) {
                // File was a pipe, clean up pipe resources.
                delete (pipe_t*)file->inode.i;
            }

            open_files[file->file_i] = nullptr;
            delete file;
        }

        return ERR_success;
    }

    errno_t seek(fd_t fd, seek_t dir, s64 off) {

        file_handle_t *handle = handle_by_fd(fd);
        if (!handle) return ERR_bad_fd;

        if (handle->file->inode.type == t_pipe)
            return ERR_type;

        locked_within_scope _(handle->lock);

        return handle->file->inode.fs->seek(handle->file->inode, handle->pos, dir, off);
    }

    ssize_t read(fd_t fd, void *buffer, size_t nbytes) {

        // Acquire a lock on the handle, so we don't read and write at the same time.
        file_handle_t *handle = handle_by_fd(fd);
        if (!handle) return ERR_bad_fd;

        // Lock till read is done.
        locked_within_scope _(handle->lock);

        if (  handle->flags & o_dir)   return ERR_type;
        if (!(handle->flags & o_read)) return ERR_type;

        if (nbytes == 0) return 0;

        assert(handle->file->inode.fs ,"no filesystem set for inode");

        ssize_t res = handle->file->inode.fs->read(handle->file->inode
                                                  ,handle->pos
                                                  ,buffer, nbytes);

        if (res < 0) return res;
        handle->pos += res;
        return res;
    }

    ssize_t write(fd_t fd, const void *buffer, size_t nbytes) {

        // Acquire a lock on the handle, so we don't read and write at the same time.
        file_handle_t *handle = handle_by_fd(fd);
        if (!handle) return ERR_bad_fd;

        // Lock till write is done.
        locked_within_scope _(handle->lock);

        if (  handle->flags & o_dir)    return ERR_type;
        if (!(handle->flags & o_write)) return ERR_type;

        if (nbytes == 0) return 0;

        assert(handle->file->inode.fs ,"no filesystem set for inode");

        ssize_t res = handle->file->inode.fs->write(handle->file->inode
                                                   ,handle->pos
                                                   ,buffer, nbytes);

        if (res < 0) return res;
        handle->pos += res;
        return res;
    }

    ssize_t read_dir(fd_t fd, dir_entry_t &dest) {

        file_handle_t *handle = handle_by_fd(fd);
        if (!handle) return ERR_bad_fd;

        if (!(handle->flags & o_dir)) return ERR_type;

        locked_within_scope _(handle->lock);

        if (!handle->file->inode.fs && handle->file->inode.i == 0) {
            // This is the VFS root dir.
            size_t mount_exist_i = 0;
            for (auto mount : mounts) {
                if (mount) {
                    if (mount_exist_i == handle->pos) {
                        dest.name   = mount->name;
                        errno_t err = mount->fs.get_root_node(dest.inode);
                        if (!err) handle->pos++;
                        return 1;
                    }
                    mount_exist_i++;
                }
            }
            return 0;

        } else {
            assert(handle->file->inode.fs, "handle does not have a fs");

            // deref all the things!
            errno_t err = handle->file->inode.fs->read_dir(handle->file->inode, handle->pos, dest);
            // handle->pos is updated by the read_dir call above.

            return err;
        }
    }

    ssize_t unlink(StringView path_) {

        return ERR_not_supported; // This is currently not available in any FS.

        path_t path = canonicalise_path(path_);

        locked_within_scope _(vfs_lock);

        // Prevent removing files that are currently open.
        if (find_open_file(path) >= 0) return ERR_in_use;

        file_name_t base = basename(path);

        inode_t parent;
        errno_t err = get_inode(path, parent);

        if (err < 0)              return err;
        if (parent.type != t_dir) return ERR_type;
        if (!parent.fs)           return ERR_not_supported; // (for the vfs root)

        return parent.fs->unlink(parent, base);
    }

    ssize_t rmdir (StringView path_) {

        return ERR_not_supported; // This is currently not available in any FS.

        path_t path      = canonicalise_path(path_);
        file_name_t base = basename(path);

        locked_within_scope _(vfs_lock);

        inode_t parent;
        errno_t err = get_inode(path, parent);

        if (err < 0)              return err;
        if (parent.type != t_dir) return ERR_type;
        if (!parent.fs)           return ERR_not_supported; // (for the vfs root)

        return parent.fs->rmdir(parent, base);
    }

    ssize_t mkdir (StringView path_) {

        return ERR_not_supported; // This is currently not available in any FS.

        path_t path      = canonicalise_path(path_);
        file_name_t base = basename(path);

        locked_within_scope _(vfs_lock);

        inode_t parent;
        errno_t err = get_inode(path, parent);

        if (err < 0)              return err;
        if (parent.type != t_dir) return ERR_type;
        if (!parent.fs)           return ERR_not_supported; // (for the vfs root)

        return parent.fs->mkdir(parent, base);
    }

    ssize_t rename(StringView src_, StringView dst_) {

        return ERR_not_supported; // This is currently not available in any FS.

        // NOTE: Ideally, we would be able to move and remove files that are
        // currently opened. However this complicates FS implementations:
        // While it may work without much trouble for a filesystem like ext2,
        // it creates problems for FAT32, where file metadata on disk is stored
        // in directory entries instead of in a separate inode structure.

        path_t src           = canonicalise_path(src_);
        path_t dst           = canonicalise_path(dst_);
        file_name_t src_base = basename(src);
        file_name_t dst_base = basename(dst);

        // (src and dst are now the paths to the containing directories)

        locked_within_scope _(vfs_lock);

        errno_t err;

        inode_t src_parent;
        inode_t dst_parent;

        err = get_inode(src, src_parent); if (err < 0) return err;
        err = get_inode(dst, dst_parent); if (err < 0) return err;

        if (!src_parent.fs || !dst_parent.fs)
            return ERR_not_supported; // (for the vfs root)

        if (src_parent.fs != dst_parent.fs)
            // TODO: To support cross-fs renames, we need to copy and unlink.
            // (or maybe we can leave this to userland instead?)
            return ERR_not_supported;

        return src_parent.fs->rename(src_parent, src_base
                                    ,dst_parent, dst_base);
    }

    void init() {
        devfs = new DevFs;
        assert(devfs, "could not allocate devfs");
        assert(mount(*devfs, "dev") == 0, "could not mount devfs");

        // FIXME: This does not belong here.
        static DevFs::memory_device_t text {Memory::region_t{ 0xb8000, 25*80*2 } };
        devfs->register_device("video-text", text, 0600);
    }
}

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build darwin dragonfly freebsd linux netbsd openbsd solaris

package os_test

import (
	"io"
	"io/ioutil"
	. "os"
	"path/filepath"
	"runtime"
	"strings"
	"syscall"
	"testing"
)

func init() {
	isReadonlyError = func(err error) bool { return err == syscall.EROFS }
}

func checkUidGid(t *testing.T, path string, uid, gid int) {
	dir, err := Lstat(path)
	if err != nil {
		t.Fatalf("Lstat %q (looking for uid/gid %d/%d): %s", path, uid, gid, err)
	}
	sys := dir.Sys().(*syscall.Stat_t)
	if int(sys.Uid) != uid {
		t.Errorf("Lstat %q: uid %d want %d", path, sys.Uid, uid)
	}
	if int(sys.Gid) != gid {
		t.Errorf("Lstat %q: gid %d want %d", path, sys.Gid, gid)
	}
}

func TestChown(t *testing.T) {
	// Use TempDir() to make sure we're on a local file system,
	// so that the group ids returned by Getgroups will be allowed
	// on the file. On NFS, the Getgroups groups are
	// basically useless.
	f := newFile("TestChown", t)
	defer Remove(f.Name())
	defer f.Close()
	dir, err := f.Stat()
	if err != nil {
		t.Fatalf("stat %s: %s", f.Name(), err)
	}

	// Can't change uid unless root, but can try
	// changing the group id. First try our current group.
	gid := Getgid()
	t.Log("gid:", gid)
	if err = Chown(f.Name(), -1, gid); err != nil {
		t.Fatalf("chown %s -1 %d: %s", f.Name(), gid, err)
	}
	sys := dir.Sys().(*syscall.Stat_t)
	checkUidGid(t, f.Name(), int(sys.Uid), gid)

	// Then try all the auxiliary groups.
	groups, err := Getgroups()
	if err != nil {
		t.Fatalf("getgroups: %s", err)
	}
	t.Log("groups: ", groups)
	for _, g := range groups {
		if err = Chown(f.Name(), -1, g); err != nil {
			t.Fatalf("chown %s -1 %d: %s", f.Name(), g, err)
		}
		checkUidGid(t, f.Name(), int(sys.Uid), g)

		// change back to gid to test fd.Chown
		if err = f.Chown(-1, gid); err != nil {
			t.Fatalf("fchown %s -1 %d: %s", f.Name(), gid, err)
		}
		checkUidGid(t, f.Name(), int(sys.Uid), gid)
	}
}

func TestFileChown(t *testing.T) {
	// Use TempDir() to make sure we're on a local file system,
	// so that the group ids returned by Getgroups will be allowed
	// on the file. On NFS, the Getgroups groups are
	// basically useless.
	f := newFile("TestFileChown", t)
	defer Remove(f.Name())
	defer f.Close()
	dir, err := f.Stat()
	if err != nil {
		t.Fatalf("stat %s: %s", f.Name(), err)
	}

	// Can't change uid unless root, but can try
	// changing the group id. First try our current group.
	gid := Getgid()
	t.Log("gid:", gid)
	if err = f.Chown(-1, gid); err != nil {
		t.Fatalf("fchown %s -1 %d: %s", f.Name(), gid, err)
	}
	sys := dir.Sys().(*syscall.Stat_t)
	checkUidGid(t, f.Name(), int(sys.Uid), gid)

	// Then try all the auxiliary groups.
	groups, err := Getgroups()
	if err != nil {
		t.Fatalf("getgroups: %s", err)
	}
	t.Log("groups: ", groups)
	for _, g := range groups {
		if err = f.Chown(-1, g); err != nil {
			t.Fatalf("fchown %s -1 %d: %s", f.Name(), g, err)
		}
		checkUidGid(t, f.Name(), int(sys.Uid), g)

		// change back to gid to test fd.Chown
		if err = f.Chown(-1, gid); err != nil {
			t.Fatalf("fchown %s -1 %d: %s", f.Name(), gid, err)
		}
		checkUidGid(t, f.Name(), int(sys.Uid), gid)
	}
}

func TestLchown(t *testing.T) {
	// Use TempDir() to make sure we're on a local file system,
	// so that the group ids returned by Getgroups will be allowed
	// on the file. On NFS, the Getgroups groups are
	// basically useless.
	f := newFile("TestLchown", t)
	defer Remove(f.Name())
	defer f.Close()
	dir, err := f.Stat()
	if err != nil {
		t.Fatalf("stat %s: %s", f.Name(), err)
	}

	linkname := f.Name() + "2"
	if err := Symlink(f.Name(), linkname); err != nil {
		if runtime.GOOS == "android" && IsPermission(err) {
			t.Skip("skipping test on Android; permission error creating symlink")
		}
		t.Fatalf("link %s -> %s: %v", f.Name(), linkname, err)
	}
	defer Remove(linkname)

	// Can't change uid unless root, but can try
	// changing the group id. First try our current group.
	gid := Getgid()
	t.Log("gid:", gid)
	if err = Lchown(linkname, -1, gid); err != nil {
		t.Fatalf("lchown %s -1 %d: %s", linkname, gid, err)
	}
	sys := dir.Sys().(*syscall.Stat_t)
	checkUidGid(t, linkname, int(sys.Uid), gid)

	// Then try all the auxiliary groups.
	groups, err := Getgroups()
	if err != nil {
		t.Fatalf("getgroups: %s", err)
	}
	t.Log("groups: ", groups)
	for _, g := range groups {
		if err = Lchown(linkname, -1, g); err != nil {
			t.Fatalf("lchown %s -1 %d: %s", linkname, g, err)
		}
		checkUidGid(t, linkname, int(sys.Uid), g)

		// Check that link target's gid is unchanged.
		checkUidGid(t, f.Name(), int(sys.Uid), int(sys.Gid))
	}
}

// Issue 16919: Readdir must return a non-empty slice or an error.
func TestReaddirRemoveRace(t *testing.T) {
	oldStat := *LstatP
	defer func() { *LstatP = oldStat }()
	*LstatP = func(name string) (FileInfo, error) {
		if strings.HasSuffix(name, "some-file") {
			// Act like it's been deleted.
			return nil, ErrNotExist
		}
		return oldStat(name)
	}
	dir := newDir("TestReaddirRemoveRace", t)
	defer RemoveAll(dir)
	if err := ioutil.WriteFile(filepath.Join(dir, "some-file"), []byte("hello"), 0644); err != nil {
		t.Fatal(err)
	}
	d, err := Open(dir)
	if err != nil {
		t.Fatal(err)
	}
	defer d.Close()
	fis, err := d.Readdir(2) // notably, greater than zero
	if len(fis) == 0 && err == nil {
		// This is what used to happen (Issue 16919)
		t.Fatal("Readdir = empty slice & err == nil")
	}
	if len(fis) != 0 || err != io.EOF {
		t.Errorf("Readdir = %d entries: %v; want 0, io.EOF", len(fis), err)
		for i, fi := range fis {
			t.Errorf("  entry[%d]: %q, %v", i, fi.Name(), fi.Mode())
		}
		t.FailNow()
	}
}

// Copyright 2011 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Package terminal provides support functions for dealing with terminals, as
// commonly found on UNIX systems.
//
// Putting a terminal into raw mode is the most common requirement:
//
// 	oldState, err := terminal.MakeRaw(0)
// 	if err != nil {
// 	        panic(err.String())
// 	}
// 	defer terminal.Restore(0, oldState)
package terminal

import (
	"io"
	"os"
	"syscall"
)

// State contains the state of a terminal.
type State struct {
	termios syscall.Termios
}

// IsTerminal returns true if the given file descriptor is a terminal.
func IsTerminal(fd int) bool {
	var termios syscall.Termios
	e := syscall.Tcgetattr(fd, &termios)
	return e == 0
}

// MakeRaw put the terminal connected to the given file descriptor into raw
// mode and returns the previous state of the terminal so that it can be
// restored.
func MakeRaw(fd int) (*State, error) {
	var oldState State
	if e := syscall.Tcgetattr(fd, &oldState.termios); e != 0 {
		return nil, os.Errno(e)
	}

	newState := oldState.termios
	newState.Iflag &^= syscall.ISTRIP | syscall.INLCR | syscall.ICRNL | syscall.IGNCR | syscall.IXON | syscall.IXOFF
	newState.Lflag &^= syscall.ECHO | syscall.ICANON | syscall.ISIG
	if e := syscall.Tcsetattr(fd, syscall.TCSANOW, &newState); e != 0 {
		return nil, os.Errno(e)
	}

	return &oldState, nil
}

// Restore restores the terminal connected to the given file descriptor to a
// previous state.
func Restore(fd int, state *State) error {
	e := syscall.Tcsetattr(fd, syscall.TCSANOW, &state.termios)
	return os.Errno(e)
}

// ReadPassword reads a line of input from a terminal without local echo.  This
// is commonly used for inputting passwords and other sensitive data. The slice
// returned does not include the \n.
func ReadPassword(fd int) ([]byte, error) {
	var oldState syscall.Termios
	if e := syscall.Tcgetattr(fd, &oldState); e != 0 {
		return nil, os.Errno(e)
	}

	newState := oldState
	newState.Lflag &^= syscall.ECHO
	if e := syscall.Tcsetattr(fd, syscall.TCSANOW, &newState); e != 0 {
		return nil, os.Errno(e)
	}

	defer func() {
		syscall.Tcsetattr(fd, syscall.TCSANOW, &oldState)
	}()

	var buf [16]byte
	var ret []byte
	for {
		n, errno := syscall.Read(fd, buf[:])
		if errno != 0 {
			return nil, os.Errno(errno)
		}
		if n == 0 {
			if len(ret) == 0 {
				return nil, io.EOF
			}
			break
		}
		if buf[n-1] == '\n' {
			n--
		}
		ret = append(ret, buf[:n]...)
		if n < len(buf) {
			break
		}
	}

	return ret, nil
}

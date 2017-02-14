// Copyright 2016 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Package protopprof converts the runtime's raw profile logs
// to Profile structs containing a representation of the pprof
// protocol buffer profile format.
package protopprof

import (
	"fmt"
	"os"
	"runtime"
	"time"
	"unsafe"

	"internal/pprof/profile"
)

// TranslateCPUProfile parses binary CPU profiling stack trace data
// generated by runtime.CPUProfile() into a profile struct.
func TranslateCPUProfile(b []byte, startTime time.Time) (*profile.Profile, error) {
	const wordSize = unsafe.Sizeof(uintptr(0))
	const minRawProfile = 5 * wordSize // Need a minimum of 5 words.
	if uintptr(len(b)) < minRawProfile {
		return nil, fmt.Errorf("truncated profile")
	}
	n := int(uintptr(len(b)) / wordSize)
	data := ((*[1 << 28]uintptr)(unsafe.Pointer(&b[0])))[:n:n]
	period := data[3]
	data = data[5:] // skip header

	// profile initialization taken from pprof tool
	p := &profile.Profile{
		Period:     int64(period) * 1000,
		PeriodType: &profile.ValueType{Type: "cpu", Unit: "nanoseconds"},
		SampleType: []*profile.ValueType{
			{Type: "samples", Unit: "count"},
			{Type: "cpu", Unit: "nanoseconds"},
		},
		TimeNanos:     int64(startTime.UnixNano()),
		DurationNanos: time.Since(startTime).Nanoseconds(),
	}
	// Parse CPU samples from the profile.
	locs := make(map[uint64]*profile.Location)
	for len(b) > 0 {
		if len(data) < 2 || uintptr(len(data)) < 2+data[1] {
			return nil, fmt.Errorf("truncated profile")
		}
		count := data[0]
		nstk := data[1]
		if uintptr(len(data)) < 2+nstk {
			return nil, fmt.Errorf("truncated profile")
		}
		stk := data[2 : 2+nstk]
		data = data[2+nstk:]

		if count == 0 && nstk == 1 && stk[0] == 0 {
			// end of data marker
			break
		}

		sloc := make([]*profile.Location, len(stk))
		for i, addr := range stk {
			addr := uint64(addr)
			// Addresses from stack traces point to the next instruction after
			// each call.  Adjust by -1 to land somewhere on the actual call
			// (except for the leaf, which is not a call).
			if i > 0 {
				addr--
			}
			loc := locs[addr]
			if loc == nil {
				loc = &profile.Location{
					ID:      uint64(len(p.Location) + 1),
					Address: addr,
				}
				locs[addr] = loc
				p.Location = append(p.Location, loc)
			}
			sloc[i] = loc
		}
		p.Sample = append(p.Sample, &profile.Sample{
			Value:    []int64{int64(count), int64(count) * int64(p.Period)},
			Location: sloc,
		})
	}

	if runtime.GOOS == "linux" {
		if err := addMappings(p); err != nil {
			return nil, err
		}
	}
	return p, nil
}

func addMappings(p *profile.Profile) error {
	// Parse memory map from /proc/self/maps
	f, err := os.Open("/proc/self/maps")
	if err != nil {
		return err
	}
	defer f.Close()
	return p.ParseMemoryMap(f)
}

// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// GOMAXPROCS=10 go test

package sync_test

import (
	"runtime"
	. "sync"
	"testing"
)

func HammerSemaphore(s *uint32, loops int, cdone chan bool) {
	for i := 0; i < loops; i++ {
		Runtime_Semacquire(s)
		Runtime_Semrelease(s)
	}
	cdone <- true
}

func TestSemaphore(t *testing.T) {
	s := new(uint32)
	*s = 1
	c := make(chan bool)
	for i := 0; i < 10; i++ {
		go HammerSemaphore(s, 1000, c)
	}
	for i := 0; i < 10; i++ {
		<-c
	}
}

func BenchmarkUncontendedSemaphore(b *testing.B) {
	s := new(uint32)
	*s = 1
	HammerSemaphore(s, b.N, make(chan bool, 2))
}

func BenchmarkContendedSemaphore(b *testing.B) {
	b.StopTimer()
	s := new(uint32)
	*s = 1
	c := make(chan bool)
	defer runtime.GOMAXPROCS(runtime.GOMAXPROCS(2))
	b.StartTimer()

	go HammerSemaphore(s, b.N/2, c)
	go HammerSemaphore(s, b.N/2, c)
	<-c
	<-c
}

func HammerMutex(m *Mutex, loops int, cdone chan bool) {
	for i := 0; i < loops; i++ {
		m.Lock()
		m.Unlock()
	}
	cdone <- true
}

func TestMutex(t *testing.T) {
	m := new(Mutex)
	c := make(chan bool)
	for i := 0; i < 10; i++ {
		go HammerMutex(m, 1000, c)
	}
	for i := 0; i < 10; i++ {
		<-c
	}
}

func TestMutexPanic(t *testing.T) {
	defer func() {
		if recover() == nil {
			t.Fatalf("unlock of unlocked mutex did not panic")
		}
	}()

	var mu Mutex
	mu.Lock()
	mu.Unlock()
	mu.Unlock()
}

func BenchmarkMutexUncontended(b *testing.B) {
	type PaddedMutex struct {
		Mutex
		pad [128]uint8
	}
	b.RunParallel(func(pb *testing.PB) {
		var mu PaddedMutex
		for pb.Next() {
			mu.Lock()
			mu.Unlock()
		}
	})
}

func benchmarkMutex(b *testing.B, slack, work bool) {
	var mu Mutex
	if slack {
		b.SetParallelism(10)
	}
	b.RunParallel(func(pb *testing.PB) {
		foo := 0
		for pb.Next() {
			mu.Lock()
			mu.Unlock()
			if work {
				for i := 0; i < 100; i++ {
					foo *= 2
					foo /= 2
				}
			}
		}
		_ = foo
	})
}

func BenchmarkMutex(b *testing.B) {
	benchmarkMutex(b, false, false)
}

func BenchmarkMutexSlack(b *testing.B) {
	benchmarkMutex(b, true, false)
}

func BenchmarkMutexWork(b *testing.B) {
	benchmarkMutex(b, false, true)
}

func BenchmarkMutexWorkSlack(b *testing.B) {
	benchmarkMutex(b, true, true)
}

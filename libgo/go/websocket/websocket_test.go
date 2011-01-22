// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package websocket

import (
	"bufio"
	"bytes"
	"fmt"
	"http"
	"io"
	"log"
	"net"
	"sync"
	"testing"
)

var serverAddr string
var once sync.Once

func echoServer(ws *Conn) { io.Copy(ws, ws) }

func startServer() {
	l, e := net.Listen("tcp", "127.0.0.1:0") // any available address
	if e != nil {
		log.Exitf("net.Listen tcp :0 %v", e)
	}
	serverAddr = l.Addr().String()
	log.Print("Test WebSocket server listening on ", serverAddr)
	http.Handle("/echo", Handler(echoServer))
	http.Handle("/echoDraft75", Draft75Handler(echoServer))
	go http.Serve(l, nil)
}

// Test the getChallengeResponse function with values from section
// 5.1 of the specification steps 18, 26, and 43 from
// http://www.whatwg.org/specs/web-socket-protocol/
func TestChallenge(t *testing.T) {
	var part1 uint32 = 777007543
	var part2 uint32 = 114997259
	key3 := []byte{0x47, 0x30, 0x22, 0x2D, 0x5A, 0x3F, 0x47, 0x58}
	expected := []byte("0st3Rl&q-2ZU^weu")

	response, err := getChallengeResponse(part1, part2, key3)
	if err != nil {
		t.Errorf("getChallengeResponse: returned error %v", err)
		return
	}
	if !bytes.Equal(expected, response) {
		t.Errorf("getChallengeResponse: expected %q got %q", expected, response)
	}
}

func TestEcho(t *testing.T) {
	once.Do(startServer)

	// websocket.Dial()
	client, err := net.Dial("tcp", "", serverAddr)
	if err != nil {
		t.Fatal("dialing", err)
	}
	ws, err := newClient("/echo", "localhost", "http://localhost",
		"ws://localhost/echo", "", client, handshake)
	if err != nil {
		t.Errorf("WebSocket handshake error: %v", err)
		return
	}

	msg := []byte("hello, world\n")
	if _, err := ws.Write(msg); err != nil {
		t.Errorf("Write: %v", err)
	}
	var actual_msg = make([]byte, 512)
	n, err := ws.Read(actual_msg)
	if err != nil {
		t.Errorf("Read: %v", err)
	}
	actual_msg = actual_msg[0:n]
	if !bytes.Equal(msg, actual_msg) {
		t.Errorf("Echo: expected %q got %q", msg, actual_msg)
	}
	ws.Close()
}

func TestEchoDraft75(t *testing.T) {
	once.Do(startServer)

	// websocket.Dial()
	client, err := net.Dial("tcp", "", serverAddr)
	if err != nil {
		t.Fatal("dialing", err)
	}
	ws, err := newClient("/echoDraft75", "localhost", "http://localhost",
		"ws://localhost/echoDraft75", "", client, draft75handshake)
	if err != nil {
		t.Errorf("WebSocket handshake: %v", err)
		return
	}

	msg := []byte("hello, world\n")
	if _, err := ws.Write(msg); err != nil {
		t.Errorf("Write: error %v", err)
	}
	var actual_msg = make([]byte, 512)
	n, err := ws.Read(actual_msg)
	if err != nil {
		t.Errorf("Read: error %v", err)
	}
	actual_msg = actual_msg[0:n]
	if !bytes.Equal(msg, actual_msg) {
		t.Errorf("Echo: expected %q got %q", msg, actual_msg)
	}
	ws.Close()
}

func TestWithQuery(t *testing.T) {
	once.Do(startServer)

	client, err := net.Dial("tcp", "", serverAddr)
	if err != nil {
		t.Fatal("dialing", err)
	}

	ws, err := newClient("/echo?q=v", "localhost", "http://localhost",
		"ws://localhost/echo?q=v", "", client, handshake)
	if err != nil {
		t.Errorf("WebSocket handshake: %v", err)
		return
	}
	ws.Close()
}

func TestWithProtocol(t *testing.T) {
	once.Do(startServer)

	client, err := net.Dial("tcp", "", serverAddr)
	if err != nil {
		t.Fatal("dialing", err)
	}

	ws, err := newClient("/echo", "localhost", "http://localhost",
		"ws://localhost/echo", "test", client, handshake)
	if err != nil {
		t.Errorf("WebSocket handshake: %v", err)
		return
	}
	ws.Close()
}

func TestHTTP(t *testing.T) {
	once.Do(startServer)

	// If the client did not send a handshake that matches the protocol
	// specification, the server should abort the WebSocket connection.
	_, _, err := http.Get(fmt.Sprintf("http://%s/echo", serverAddr))
	if err == nil {
		t.Error("Get: unexpected success")
		return
	}
	urlerr, ok := err.(*http.URLError)
	if !ok {
		t.Errorf("Get: not URLError %#v", err)
		return
	}
	if urlerr.Error != io.ErrUnexpectedEOF {
		t.Errorf("Get: error %#v", err)
		return
	}
}

func TestHTTPDraft75(t *testing.T) {
	once.Do(startServer)

	r, _, err := http.Get(fmt.Sprintf("http://%s/echoDraft75", serverAddr))
	if err != nil {
		t.Errorf("Get: error %#v", err)
		return
	}
	if r.StatusCode != http.StatusBadRequest {
		t.Errorf("Get: got status %d", r.StatusCode)
	}
}

func TestTrailingSpaces(t *testing.T) {
	// http://code.google.com/p/go/issues/detail?id=955
	// The last runs of this create keys with trailing spaces that should not be
	// generated by the client.
	once.Do(startServer)
	for i := 0; i < 30; i++ {
		// body
		_, err := Dial(fmt.Sprintf("ws://%s/echo", serverAddr), "",
			"http://localhost/")
		if err != nil {
			panic("Dial failed: " + err.String())
		}
	}
}

func TestSmallBuffer(t *testing.T) {
	// http://code.google.com/p/go/issues/detail?id=1145
	// Read should be able to handle reading a fragment of a frame.
	once.Do(startServer)

	// websocket.Dial()
	client, err := net.Dial("tcp", "", serverAddr)
	if err != nil {
		t.Fatal("dialing", err)
	}
	ws, err := newClient("/echo", "localhost", "http://localhost",
		"ws://localhost/echo", "", client, handshake)
	if err != nil {
		t.Errorf("WebSocket handshake error: %v", err)
		return
	}

	msg := []byte("hello, world\n")
	if _, err := ws.Write(msg); err != nil {
		t.Errorf("Write: %v", err)
	}
	var small_msg = make([]byte, 8)
	n, err := ws.Read(small_msg)
	if err != nil {
		t.Errorf("Read: %v", err)
	}
	if !bytes.Equal(msg[:len(small_msg)], small_msg) {
		t.Errorf("Echo: expected %q got %q", msg[:len(small_msg)], small_msg)
	}
	var second_msg = make([]byte, len(msg))
	n, err = ws.Read(second_msg)
	if err != nil {
		t.Errorf("Read: %v", err)
	}
	second_msg = second_msg[0:n]
	if !bytes.Equal(msg[len(small_msg):], second_msg) {
		t.Errorf("Echo: expected %q got %q", msg[len(small_msg):], second_msg)
	}
	ws.Close()

}

func testSkipLengthFrame(t *testing.T) {
	b := []byte{'\x80', '\x01', 'x', 0, 'h', 'e', 'l', 'l', 'o', '\xff'}
	buf := bytes.NewBuffer(b)
	br := bufio.NewReader(buf)
	bw := bufio.NewWriter(buf)
	ws := newConn("http://127.0.0.1/", "ws://127.0.0.1/", "", bufio.NewReadWriter(br, bw), nil)
	msg := make([]byte, 5)
	n, err := ws.Read(msg)
	if err != nil {
		t.Errorf("Read: %v", err)
	}
	if !bytes.Equal(b[4:8], msg[0:n]) {
		t.Errorf("Read: expected %q got %q", msg[4:8], msg[0:n])
	}
}

func testSkipNoUTF8Frame(t *testing.T) {
	b := []byte{'\x01', 'n', '\xff', 0, 'h', 'e', 'l', 'l', 'o', '\xff'}
	buf := bytes.NewBuffer(b)
	br := bufio.NewReader(buf)
	bw := bufio.NewWriter(buf)
	ws := newConn("http://127.0.0.1/", "ws://127.0.0.1/", "", bufio.NewReadWriter(br, bw), nil)
	msg := make([]byte, 5)
	n, err := ws.Read(msg)
	if err != nil {
		t.Errorf("Read: %v", err)
	}
	if !bytes.Equal(b[4:8], msg[0:n]) {
		t.Errorf("Read: expected %q got %q", msg[4:8], msg[0:n])
	}
}

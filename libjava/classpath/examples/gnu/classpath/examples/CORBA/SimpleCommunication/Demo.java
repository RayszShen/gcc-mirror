/* Demo.java -- Demonstrates simple CORBA client-server communications.
   Copyright (C) 2005 Free Software Foundation, Inc.

This file is part of GNU Classpath.

GNU Classpath is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Classpath is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Classpath; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA.

Linking this library statically or dynamically with other modules is
making a combined work based on this library.  Thus, the terms and
conditions of the GNU General Public License cover the whole
combination.

As a special exception, the copyright holders of this library give you
permission to link this library with independent modules to produce an
executable, regardless of the license terms of these independent
modules, and to copy and distribute the resulting executable under
terms of your choice, provided that you also meet, for each linked
independent module, the terms and conditions of the license of that
module.  An independent module is a module which is not derived from
or based on this library.  If you modify this library, you may extend
this exception to your version of the library, but you are not
obligated to do so.  If you do not wish to do so, delete this
exception statement from your version. */


package gnu.classpath.examples.CORBA.SimpleCommunication;

import gnu.classpath.examples.CORBA.SimpleCommunication.communication.DirectTest;
import gnu.classpath.examples.CORBA.SimpleCommunication.communication.RequestTest;


/**
 * This sample illustrates the CORBA communication between server
 * and client. In this simple example both server and client are
 * started on the same virtual machine. For the real interoperability
 * tests, however, the server is started on the platform (library+jvm) of
 * one vendor, and the client on the platform of another vendor.
 *
 * The interoperability is currently tested with Sun Microystems
 * jre 1.4.
 *
 * This example required the current folder to be writable to pass
 * the IOR references via shared file.
 *
 * @author Audrius Meskauskas, Lithuania (AudriusA@Bioinformatics.org)
 */
public class Demo
{
  public static void main(final String[] args)
  {
    // Start the server.
    new Thread()
    {
      public void run()
      {
        comServer.start_server(args);
      }
    }.start();

    System.out.println("Waiting for three seconds for the server to start...");

    // Pause some time for the server to start.
    try {
      Thread.sleep(3000);
    }
    catch (InterruptedException ex) {
    }

    // Test the stream oriented communication.
    DirectTest.main(args);
    // Test the request oriented communication.
    RequestTest.main(args);

    System.exit(0);
  }
}

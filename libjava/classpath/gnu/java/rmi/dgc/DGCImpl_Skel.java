/* DGCImpl_Skel.java
   Copyright (C) 2002 Free Software Foundation, Inc.

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


// Skel class generated by rmic - DO NOT EDIT!

package gnu.java.rmi.dgc;

public final class DGCImpl_Skel
    implements java.rmi.server.Skeleton
{
    private static final long interfaceHash = -669196253586618813L;
    
    private static final java.rmi.server.Operation[] operations = {
        new java.rmi.server.Operation("void clean(java.rmi.server.ObjID[], long, java.rmi.dgc.VMID, boolean"),
        new java.rmi.server.Operation("java.rmi.dgc.Lease dirty(java.rmi.server.ObjID[], long, java.rmi.dgc.Lease")
    };
    
    public java.rmi.server.Operation[] getOperations() {
        return ((java.rmi.server.Operation[]) operations.clone());
    }
    
    public void dispatch(java.rmi.Remote obj, java.rmi.server.RemoteCall call, int opnum, long hash) throws java.lang.Exception {
        if (opnum < 0) {
            if (hash == -5803803475088455571L) {
                opnum = 0;
            }
            else if (hash == -8139341527526761862L) {
                opnum = 1;
            }
            else {
                throw new java.rmi.server.SkeletonMismatchException("interface hash mismatch");
            }
        }
        else if (hash != interfaceHash) {
            throw new java.rmi.server.SkeletonMismatchException("interface hash mismatch");
        }
        
        gnu.java.rmi.dgc.DGCImpl server = (gnu.java.rmi.dgc.DGCImpl)obj;
        switch (opnum) {
        case 0:
        {
            java.rmi.server.ObjID[] $param_0;
            long $param_1;
            java.rmi.dgc.VMID $param_2;
            boolean $param_3;
            try {
                java.io.ObjectInput in = call.getInputStream();
                $param_0 = (java.rmi.server.ObjID[])in.readObject();
                $param_1 = (long)in.readLong();
                $param_2 = (java.rmi.dgc.VMID)in.readObject();
                $param_3 = (boolean)in.readBoolean();
                
            }
            catch (java.io.IOException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            catch (java.lang.ClassCastException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            finally {
                call.releaseInputStream();
            }
            server.clean($param_0, $param_1, $param_2, $param_3);
            try {
                java.io.ObjectOutput out = call.getResultStream(true);
            }
            catch (java.io.IOException e) {
                throw new java.rmi.MarshalException("error marshalling return", e);
            }
            break;
        }
        
        case 1:
        {
            java.rmi.server.ObjID[] $param_0;
            long $param_1;
            java.rmi.dgc.Lease $param_2;
            try {
                java.io.ObjectInput in = call.getInputStream();
                $param_0 = (java.rmi.server.ObjID[])in.readObject();
                $param_1 = (long)in.readLong();
                $param_2 = (java.rmi.dgc.Lease)in.readObject();
                
            }
            catch (java.io.IOException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            catch (java.lang.ClassCastException e) {
                throw new java.rmi.UnmarshalException("error unmarshalling arguments", e);
            }
            finally {
                call.releaseInputStream();
            }
            java.rmi.dgc.Lease $result = server.dirty($param_0, $param_1, $param_2);
            try {
                java.io.ObjectOutput out = call.getResultStream(true);
                out.writeObject($result);
            }
            catch (java.io.IOException e) {
                throw new java.rmi.MarshalException("error marshalling return", e);
            }
            break;
        }
        
        default:
            throw new java.rmi.UnmarshalException("invalid method number");
        }
    }
}

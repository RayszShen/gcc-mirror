/* DynamicImplementation.java --
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


package org.omg.CORBA;

import gnu.CORBA.Unexpected;
import gnu.CORBA.gnuAny;
import gnu.CORBA.gnuNVList;

import org.omg.CORBA.portable.ObjectImpl;
import org.omg.CORBA.portable.OutputStream;

/**
 * This class was probably originally thinked as a base of all CORBA
 * object implementations. However the code, generated by IDL to
 * java compilers almost never use it, preferring to derive the
 * object implementation bases directly from the {@link ObjectImpl}.
 * The class has become deprecated since the 1.4 release.
 *
 * @deprecated since 1.4.
 *
 * @author Audrius Meskauskas, Lithuania (AudriusA@Bioinformatics.org)
 */
public class DynamicImplementation
  extends ObjectImpl
{
  /**
   * Invoke the method of the CORBA object. After converting the parameters,
   * this method delegates call to the {@link ObjectImpl#invoke}.
   * 
   * @deprecated since 1.4.
   * 
   * @param request the container for both passing and returing the parameters,
   * also contains the method name and thrown exceptions.
   */
  public void invoke(ServerRequest request)
  {
    Request r = _request(request.operation());

    // Copy the parameters.
    NVList args = new gnuNVList();
    request.arguments(args);
    NamedValue v;
    int i = 0;

    try
      {
        // Set the arguments.
        for (i = 0; i < args.count(); i++)
          {
            v = args.item(i);
            Any n;
            OutputStream out;

            switch (v.flags())
              {
                case ARG_IN.value:
                  out = v.value().create_output_stream();
                  v.value().write_value(out);
                  n = r.add_named_in_arg(v.name());
                  n.read_value(out.create_input_stream(), v.value().type());                  
                  break;
                case ARG_INOUT.value:
                  out = v.value().create_output_stream();
                  v.value().write_value(out);
                  n = r.add_named_inout_arg(v.name());
                  n.read_value(out.create_input_stream(), v.value().type());                  
                  break;
                case ARG_OUT.value:
                  r.add_named_out_arg(v.name());
                  break;

                default:
                  throw new InternalError("Invalid flags " + v.flags());
              }
          }
      }
    catch (Bounds b)
      {
        throw new Unexpected(args.count() + "[" + i + "]", b);
      }

    // Set context.
    r.ctx(request.ctx());
    
    // Set the return type (expects that the ServerRequest will initialise
    // the passed Any.
    
    gnuAny g = new gnuAny();
    request.result(g);
    r.set_return_type(g.type());

    // Invoke the method.
    r.invoke();

    // Transfer the returned values.
    NVList r_args = r.arguments();

    try
      {
        // API states that the ServerRequest.arguments must be called only
        // once. Hence we assume we can just modify the previously returned
        // value <code>args</code>, and the ServerRequest will preserve the 
        // reference.
        for (i = 0; i < args.count(); i++)
          {
            v = args.item(i);

            if (v.flags() == ARG_OUT.value || v.flags() == ARG_INOUT.value)
              {
                OutputStream out = r_args.item(i).value().create_output_stream();
                r_args.item(i).value().write_value(out);
                v.value().read_value(out.create_input_stream(),
                  v.value().type());
              }
          }
      }
    catch (Bounds b)
      {
        throw new Unexpected(args.count() + "[" + i + "]", b);
      }

    // Set the returned result (if any).
    NamedValue returns = r.result();
    if (returns != null)
      request.set_result(returns.value());
  }

  /**
   * Returns the array of the repository ids, supported by this object.
   * In this implementation, the method must be overrridden to return
   * a sendible object-specific information. The default method returns
   * an empty array.
   *
   * @deprecated since 1.4.
   *
   * @return the empty array, always.
   */
  public String[] _ids()
  {
    return new String[ 0 ];
  }
}

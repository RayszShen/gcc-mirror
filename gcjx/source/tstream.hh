// Token stream.

// Copyright (C) 2004 Free Software Foundation, Inc.
//
// This file is part of GCC.
//
// gcjx is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// gcjx is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with gcjx; see the file COPYING.LIB.  If
// not, write to the Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef GCJX_SOURCE_TSTREAM_HH
#define GCJX_SOURCE_TSTREAM_HH

#include <stack>

#include "source/lex.hh"

// The token stream wraps the lexer with a buffer.  The user can set a
// mark, and no token before the mark will be lost.  The token stream
// also understands a couple of special cases for java, and will
// filter the actual tokens according to what the parser requires.
class token_stream : public lexer
{
  // Our buffer.  FIXME: size and stuff.. use some STL thing.
  token *buffer;

  // Size of buffer.
  int buffer_size;

  // Position of next free slot in buffer.
  int buffer_end;

  // Position of next unread element in buffer.
  int read_position;

  // All the marks we've set.
  std::stack<int> marks;

  // True if we're buffering tokens because there is a mark.
  bool mark_buffering;

  // True if we're buffering tokens because we're peeking.
  bool peek_buffering;
  

  // True if the parser can usefully interpret a javadoc comment as
  // the next token.  When false, we filter out such comments.
  bool javadoc_is_ok;


  // Set a mark at the current point.  Only called by marker class.
  // Returns the position.
  int set_mark ();

  // Unset the mark at position.  Only called by marker class.
  void unset_mark (int);

  // Reset the read pointer to a position.  Only called by marker
  // class.
  void reset_to_mark (int);

  friend class marker;


  // Return a token before any filtering is applied.
  token get_unfiltered_token ();

public:

  token_stream (ucs2_reader *source, const char *file)
    : lexer (source, file),
      buffer (NULL),
      buffer_size (0),
      buffer_end (0),
      read_position (0),
      mark_buffering (false),
      peek_buffering (false),
      javadoc_is_ok (false)
  {
  }

  ~token_stream ()
  {
    if (buffer != NULL)
      delete [] buffer;
  }

  // Indicate that it is ok for the next token to be TOKEN_JAVADOC.
  // The token stream imposes some restraints on this that conform to
  // javadoc processing.
  void javadoc_ok ()
  {
    javadoc_is_ok = true;
  }

  // Fetch the next token.
  token get_token ();

  // Look at the next token without committing to it.
  token peek_token ();

  // Look ahead two tokens without committing.  This is ad hoc, but it
  // seems to be all the parser requires.
  token peek_token1 ();
};

// This marks a location in a token stream buffer and ensures that no
// token past that point is destroyed.
class marker
{
  // The location of this mark.
  int location;

  // Associated token stream.
  token_stream *stream;

public:

  marker (token_stream *s)
    : stream (s)
  {
    location = stream->set_mark ();
  }

  ~marker ()
  {
    stream->unset_mark (location);
  }

  void backtrack ()
  {
    stream->reset_to_mark (location);
  }
};

#endif // GCJX_SOURCE_TSTREAM_HH

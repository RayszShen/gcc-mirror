/* CPP Library - lexical analysis.
   Copyright (C) 2000 Free Software Foundation, Inc.
   Contributed by Per Bothner, 1994-95.
   Based on CCCP program by Paul Rubin, June 1986
   Adapted to ANSI C, Richard Stallman, Jan 1987
   Broken out to separate file, Zack Weinberg, Mar 2000
   Single-pass line tokenization by Neil Booth, April 2000

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* This lexer works with a single pass of the file.  Recently I
   re-wrote it to minimize the places where we step backwards in the
   input stream, to make future changes to support multi-byte
   character sets fairly straight-forward.

   There is now only one routine where we do step backwards:
   skip_escaped_newlines.  This routine could probably also be changed
   so that it doesn't need to step back.  One possibility is to use a
   trick similar to that used in lex_period and lex_percent.  Two
   extra characters might be needed, but skip_escaped_newlines itself
   would probably be the only place that needs to be aware of that,
   and changes to the remaining routines would probably only be needed
   if they process a backslash.  */

#include "config.h"
#include "system.h"
#include "cpplib.h"
#include "cpphash.h"
#include "symcat.h"

/* Tokens with SPELL_STRING store their spelling in the token list,
   and it's length in the token->val.name.len.  */
enum spell_type
{
  SPELL_OPERATOR = 0,
  SPELL_CHAR,
  SPELL_IDENT,
  SPELL_STRING,
  SPELL_NONE
};

struct token_spelling
{
  enum spell_type category;
  const unsigned char *name;
};

const unsigned char *digraph_spellings [] = {U"%:", U"%:%:", U"<:",
					     U":>", U"<%", U"%>"};

#define OP(e, s) { SPELL_OPERATOR, U s           },
#define TK(e, s) { s,              U STRINGX (e) },
const struct token_spelling token_spellings [N_TTYPES] = {TTYPE_TABLE };
#undef OP
#undef TK

#define TOKEN_SPELL(token) (token_spellings[(token)->type].category)
#define TOKEN_NAME(token) (token_spellings[(token)->type].name)

static cppchar_t handle_newline PARAMS ((cpp_buffer *, cppchar_t));
static cppchar_t skip_escaped_newlines PARAMS ((cpp_buffer *, cppchar_t));
static cppchar_t get_effective_char PARAMS ((cpp_buffer *));

static int skip_block_comment PARAMS ((cpp_reader *));
static int skip_line_comment PARAMS ((cpp_reader *));
static void adjust_column PARAMS ((cpp_reader *));
static void skip_whitespace PARAMS ((cpp_reader *, cppchar_t));
static cpp_hashnode *parse_identifier PARAMS ((cpp_reader *, cppchar_t));
static void parse_number PARAMS ((cpp_reader *, cpp_string *, cppchar_t, int));
static int unescaped_terminator_p PARAMS ((cpp_reader *, const U_CHAR *));
static void parse_string PARAMS ((cpp_reader *, cpp_token *, cppchar_t));
static void unterminated PARAMS ((cpp_reader *, int));
static int trigraph_ok PARAMS ((cpp_reader *, cppchar_t));
static void save_comment PARAMS ((cpp_reader *, cpp_token *, const U_CHAR *));
static void lex_percent PARAMS ((cpp_buffer *, cpp_token *));
static void lex_dot PARAMS ((cpp_reader *, cpp_token *));
static int name_p PARAMS ((cpp_reader *, const cpp_string *));

static cpp_chunk *new_chunk PARAMS ((unsigned int));
static int chunk_suitable PARAMS ((cpp_pool *, cpp_chunk *, unsigned int));

/* Utility routine:

   Compares, the token TOKEN to the NUL-terminated string STRING.
   TOKEN must be a CPP_NAME.  Returns 1 for equal, 0 for unequal.  */

int
cpp_ideq (token, string)
     const cpp_token *token;
     const char *string;
{
  if (token->type != CPP_NAME)
    return 0;

  return !ustrcmp (token->val.node->name, (const U_CHAR *) string);
}

/* Call when meeting a newline.  Returns the character after the newline
   (or carriage-return newline combination), or EOF.  */
static cppchar_t
handle_newline (buffer, newline_char)
     cpp_buffer *buffer;
     cppchar_t newline_char;
{
  cppchar_t next = EOF;

  buffer->col_adjust = 0;
  buffer->lineno++;
  buffer->line_base = buffer->cur;

  /* Handle CR-LF and LF-CR combinations, get the next character.  */
  if (buffer->cur < buffer->rlimit)
    {
      next = *buffer->cur++;
      if (next + newline_char == '\r' + '\n')
	{
	  buffer->line_base = buffer->cur;
	  if (buffer->cur < buffer->rlimit)
	    next = *buffer->cur++;
	  else
	    next = EOF;
	}
    }

  buffer->read_ahead = next;
  return next;
}

/* Subroutine of skip_escaped_newlines; called when a trigraph is
   encountered.  It warns if necessary, and returns true if the
   trigraph should be honoured.  FROM_CHAR is the third character of a
   trigraph, and presumed to be the previous character for position
   reporting.  */
static int
trigraph_ok (pfile, from_char)
     cpp_reader *pfile;
     cppchar_t from_char;
{
  int accept = CPP_OPTION (pfile, trigraphs);
  
  /* Don't warn about trigraphs in comments.  */
  if (CPP_OPTION (pfile, warn_trigraphs) && !pfile->state.lexing_comment)
    {
      cpp_buffer *buffer = pfile->buffer;
      if (accept)
	cpp_warning_with_line (pfile, buffer->lineno, CPP_BUF_COL (buffer) - 2,
			       "trigraph ??%c converted to %c",
			       (int) from_char,
			       (int) _cpp_trigraph_map[from_char]);
      else if (buffer->cur != buffer->last_Wtrigraphs)
	{
	  buffer->last_Wtrigraphs = buffer->cur;
	  cpp_warning_with_line (pfile, buffer->lineno,
				 CPP_BUF_COL (buffer) - 2,
				 "trigraph ??%c ignored", (int) from_char);
	}
    }

  return accept;
}

/* Assumes local variables buffer and result.  */
#define ACCEPT_CHAR(t) \
  do { result->type = t; buffer->read_ahead = EOF; } while (0)

/* When we move to multibyte character sets, add to these something
   that saves and restores the state of the multibyte conversion
   library.  This probably involves saving and restoring a "cookie".
   In the case of glibc it is an 8-byte structure, so is not a high
   overhead operation.  In any case, it's out of the fast path.  */
#define SAVE_STATE() do { saved_cur = buffer->cur; } while (0)
#define RESTORE_STATE() do { buffer->cur = saved_cur; } while (0)

/* Skips any escaped newlines introduced by NEXT, which is either a
   '?' or a '\\'.  Returns the next character, which will also have
   been placed in buffer->read_ahead.  This routine performs
   preprocessing stages 1 and 2 of the ISO C standard.  */
static cppchar_t
skip_escaped_newlines (buffer, next)
     cpp_buffer *buffer;
     cppchar_t next;
{
  /* Only do this if we apply stages 1 and 2.  */
  if (!buffer->from_stage3)
    {
      cppchar_t next1;
      const unsigned char *saved_cur;
      int space;

      do
	{
	  if (buffer->cur == buffer->rlimit)
	    break;
      
	  SAVE_STATE ();
	  if (next == '?')
	    {
	      next1 = *buffer->cur++;
	      if (next1 != '?' || buffer->cur == buffer->rlimit)
		{
		  RESTORE_STATE ();
		  break;
		}

	      next1 = *buffer->cur++;
	      if (!_cpp_trigraph_map[next1]
		  || !trigraph_ok (buffer->pfile, next1))
		{
		  RESTORE_STATE ();
		  break;
		}

	      /* We have a full trigraph here.  */
	      next = _cpp_trigraph_map[next1];
	      if (next != '\\' || buffer->cur == buffer->rlimit)
		break;
	      SAVE_STATE ();
	    }

	  /* We have a backslash, and room for at least one more character.  */
	  space = 0;
	  do
	    {
	      next1 = *buffer->cur++;
	      if (!is_nvspace (next1))
		break;
	      space = 1;
	    }
	  while (buffer->cur < buffer->rlimit);

	  if (!is_vspace (next1))
	    {
	      RESTORE_STATE ();
	      break;
	    }

	  if (space && !buffer->pfile->state.lexing_comment)
	    cpp_warning (buffer->pfile,
			 "backslash and newline separated by space");

	  next = handle_newline (buffer, next1);
	  if (next == EOF)
	    cpp_pedwarn (buffer->pfile, "backslash-newline at end of file");
	}
      while (next == '\\' || next == '?');
    }

  buffer->read_ahead = next;
  return next;
}

/* Obtain the next character, after trigraph conversion and skipping
   an arbitrary string of escaped newlines.  The common case of no
   trigraphs or escaped newlines falls through quickly.  */
static cppchar_t
get_effective_char (buffer)
     cpp_buffer *buffer;
{
  cppchar_t next = EOF;

  if (buffer->cur < buffer->rlimit)
    {
      next = *buffer->cur++;

      /* '?' can introduce trigraphs (and therefore backslash); '\\'
	 can introduce escaped newlines, which we want to skip, or
	 UCNs, which, depending upon lexer state, we will handle in
	 the future.  */
      if (next == '?' || next == '\\')
	next = skip_escaped_newlines (buffer, next);
    }

  buffer->read_ahead = next;
  return next;
}

/* Skip a C-style block comment.  We find the end of the comment by
   seeing if an asterisk is before every '/' we encounter.  Returns
   non-zero if comment terminated by EOF, zero otherwise.  */
static int
skip_block_comment (pfile)
     cpp_reader *pfile;
{
  cpp_buffer *buffer = pfile->buffer;
  cppchar_t c = EOF, prevc = EOF;

  pfile->state.lexing_comment = 1;
  while (buffer->cur != buffer->rlimit)
    {
      prevc = c, c = *buffer->cur++;

    next_char:
      /* FIXME: For speed, create a new character class of characters
	 of interest inside block comments.  */
      if (c == '?' || c == '\\')
	c = skip_escaped_newlines (buffer, c);

      /* People like decorating comments with '*', so check for '/'
	 instead for efficiency.  */
      if (c == '/')
	{
	  if (prevc == '*')
	    break;

	  /* Warn about potential nested comments, but not if the '/'
	     comes immediately before the true comment delimeter.
	     Don't bother to get it right across escaped newlines.  */
	  if (CPP_OPTION (pfile, warn_comments)
	      && buffer->cur != buffer->rlimit)
	    {
	      prevc = c, c = *buffer->cur++;
	      if (c == '*' && buffer->cur != buffer->rlimit)
		{
		  prevc = c, c = *buffer->cur++;
		  if (c != '/') 
		    cpp_warning_with_line (pfile, CPP_BUF_LINE (buffer),
					   CPP_BUF_COL (buffer),
					   "\"/*\" within comment");
		}
	      goto next_char;
	    }
	}
      else if (is_vspace (c))
	{
	  prevc = c, c = handle_newline (buffer, c);
	  goto next_char;
	}
      else if (c == '\t')
	adjust_column (pfile);
    }

  pfile->state.lexing_comment = 0;
  buffer->read_ahead = EOF;
  return c != '/' || prevc != '*';
}

/* Skip a C++ line comment.  Handles escaped newlines.  Returns
   non-zero if a multiline comment.  The following new line, if any,
   is left in buffer->read_ahead.  */
static int
skip_line_comment (pfile)
     cpp_reader *pfile;
{
  cpp_buffer *buffer = pfile->buffer;
  unsigned int orig_lineno = buffer->lineno;
  cppchar_t c;

  pfile->state.lexing_comment = 1;
  do
    {
      c = EOF;
      if (buffer->cur == buffer->rlimit)
	break;

      c = *buffer->cur++;
      if (c == '?' || c == '\\')
	c = skip_escaped_newlines (buffer, c);
    }
  while (!is_vspace (c));

  pfile->state.lexing_comment = 0;
  buffer->read_ahead = c;	/* Leave any newline for caller.  */
  return orig_lineno != buffer->lineno;
}

/* pfile->buffer->cur is one beyond the \t character.  Update
   col_adjust so we track the column correctly.  */
static void
adjust_column (pfile)
     cpp_reader *pfile;
{
  cpp_buffer *buffer = pfile->buffer;
  unsigned int col = CPP_BUF_COL (buffer) - 1; /* Zero-based column.  */

  /* Round it up to multiple of the tabstop, but subtract 1 since the
     tab itself occupies a character position.  */
  buffer->col_adjust += (CPP_OPTION (pfile, tabstop)
			 - col % CPP_OPTION (pfile, tabstop)) - 1;
}

/* Skips whitespace, saving the next non-whitespace character.
   Adjusts pfile->col_adjust to account for tabs.  Without this,
   tokens might be assigned an incorrect column.  */
static void
skip_whitespace (pfile, c)
     cpp_reader *pfile;
     cppchar_t c;
{
  cpp_buffer *buffer = pfile->buffer;
  unsigned int warned = 0;

  do
    {
      /* Horizontal space always OK.  */
      if (c == ' ')
	;
      else if (c == '\t')
	adjust_column (pfile);
      /* Just \f \v or \0 left.  */
      else if (c == '\0')
	{
	  if (!warned)
	    {
	      cpp_warning (pfile, "null character(s) ignored");
	      warned = 1;
	    }
	}
      else if (pfile->state.in_directive && CPP_PEDANTIC (pfile))
	cpp_pedwarn_with_line (pfile, CPP_BUF_LINE (buffer),
			       CPP_BUF_COL (buffer),
			       "%s in preprocessing directive",
			       c == '\f' ? "form feed" : "vertical tab");

      c = EOF;
      if (buffer->cur == buffer->rlimit)
	break;
      c = *buffer->cur++;
    }
  /* We only want non-vertical space, i.e. ' ' \t \f \v \0. */
  while (is_nvspace (c));

  /* Remember the next character.  */
  buffer->read_ahead = c;
}

/* See if the characters of a number token are valid in a name (no
   '.', '+' or '-').  */
static int
name_p (pfile, string)
     cpp_reader *pfile;
     const cpp_string *string;
{
  unsigned int i;

  for (i = 0; i < string->len; i++)
    if (!is_idchar (string->text[i]))
      return 0;

  return 1;  
}

/* Parse an identifier, skipping embedded backslash-newlines.
   Calculate the hash value of the token while parsing, for improved
   performance.  The hashing algorithm *must* match cpp_lookup().  */

static cpp_hashnode *
parse_identifier (pfile, c)
     cpp_reader *pfile;
     cppchar_t c;
{
  cpp_hashnode *result;
  cpp_buffer *buffer = pfile->buffer;
  unsigned char *dest, *limit;
  unsigned int r = 0, saw_dollar = 0;

  dest = POOL_FRONT (&pfile->ident_pool);
  limit = POOL_LIMIT (&pfile->ident_pool);

  do
    {
      do
	{
	  /* Need room for terminating null.  */
	  if (dest + 1 >= limit)
	    limit = _cpp_next_chunk (&pfile->ident_pool, 0, &dest);

	  *dest++ = c;
	  r = HASHSTEP (r, c);

	  if (c == '$')
	    saw_dollar++;

	  c = EOF;
	  if (buffer->cur == buffer->rlimit)
	    break;

	  c = *buffer->cur++;
	}
      while (is_idchar (c));

      /* Potential escaped newline?  */
      if (c != '?' && c != '\\')
	break;
      c = skip_escaped_newlines (buffer, c);
    }
  while (is_idchar (c));

  /* Remember the next character.  */
  buffer->read_ahead = c;

  /* $ is not a identifier character in the standard, but is commonly
     accepted as an extension.  Don't warn about it in skipped
     conditional blocks.  */
  if (saw_dollar && CPP_PEDANTIC (pfile) && ! pfile->skipping)
    cpp_pedwarn (pfile, "'$' character(s) in identifier");

  /* Identifiers are null-terminated.  */
  *dest = '\0';

  /* This routine commits the memory if necessary.  */
  result = _cpp_lookup_with_hash (pfile,
				  dest - POOL_FRONT (&pfile->ident_pool), r);

  /* Some identifiers require diagnostics when lexed.  */
  if (result->flags & NODE_DIAGNOSTIC && !pfile->skipping)
    {
      /* It is allowed to poison the same identifier twice.  */
      if ((result->flags & NODE_POISONED) && !pfile->state.poisoned_ok)
	cpp_error (pfile, "attempt to use poisoned \"%s\"", result->name);

      /* Constraint 6.10.3.5: __VA_ARGS__ should only appear in the
	 replacement list of a variadic macro.  */
      if (result == pfile->spec_nodes.n__VA_ARGS__
	  && !pfile->state.va_args_ok)
	cpp_pedwarn (pfile, "__VA_ARGS__ can only appear in the expansion of a C99 variadic macro");
    }

  return result;
}

/* Parse a number, skipping embedded backslash-newlines.  */
static void
parse_number (pfile, number, c, leading_period)
     cpp_reader *pfile;
     cpp_string *number;
     cppchar_t c;
     int leading_period;
{
  cpp_buffer *buffer = pfile->buffer;
  cpp_pool *pool = &pfile->ident_pool;
  unsigned char *dest, *limit;

  dest = POOL_FRONT (pool);
  limit = POOL_LIMIT (pool);

  /* Place a leading period.  */
  if (leading_period)
    {
      if (dest >= limit)
	limit = _cpp_next_chunk (pool, 0, &dest);
      *dest++ = '.';
    }
  
  do
    {
      do
	{
	  /* Need room for terminating null.  */
	  if (dest + 1 >= limit)
	    limit = _cpp_next_chunk (pool, 0, &dest);
	  *dest++ = c;

	  c = EOF;
	  if (buffer->cur == buffer->rlimit)
	    break;

	  c = *buffer->cur++;
	}
      while (is_numchar (c) || c == '.' || VALID_SIGN (c, dest[-1]));

      /* Potential escaped newline?  */
      if (c != '?' && c != '\\')
	break;
      c = skip_escaped_newlines (buffer, c);
    }
  while (is_numchar (c) || c == '.' || VALID_SIGN (c, dest[-1]));

  /* Remember the next character.  */
  buffer->read_ahead = c;

  /* Null-terminate the number.  */
  *dest = '\0';

  number->text = POOL_FRONT (pool);
  number->len = dest - number->text;
  POOL_COMMIT (pool, number->len + 1);
}

/* Subroutine of parse_string.  Emits error for unterminated strings.  */
static void
unterminated (pfile, term)
     cpp_reader *pfile;
     int term;
{
  cpp_error (pfile, "missing terminating %c character", term);

  if (term == '\"' && pfile->mlstring_pos.line
      && pfile->mlstring_pos.line != pfile->lexer_pos.line)
    {
      cpp_error_with_line (pfile, pfile->mlstring_pos.line,
			   pfile->mlstring_pos.col,
			   "possible start of unterminated string literal");
      pfile->mlstring_pos.line = 0;
    }
}

/* Subroutine of parse_string.  */
static int
unescaped_terminator_p (pfile, dest)
     cpp_reader *pfile;
     const unsigned char *dest;
{
  const unsigned char *start, *temp;

  /* In #include-style directives, terminators are not escapeable.  */
  if (pfile->state.angled_headers)
    return 1;

  start = POOL_FRONT (&pfile->ident_pool);

  /* An odd number of consecutive backslashes represents an escaped
     terminator.  */
  for (temp = dest; temp > start && temp[-1] == '\\'; temp--)
    ;

  return ((dest - temp) & 1) == 0;
}

/* Parses a string, character constant, or angle-bracketed header file
   name.  Handles embedded trigraphs and escaped newlines.

   Multi-line strings are allowed, but they are deprecated within
   directives.  */
static void
parse_string (pfile, token, terminator)
     cpp_reader *pfile;
     cpp_token *token;
     cppchar_t terminator;
{
  cpp_buffer *buffer = pfile->buffer;
  cpp_pool *pool = &pfile->ident_pool;
  unsigned char *dest, *limit;
  cppchar_t c;
  unsigned int nulls = 0;

  dest = POOL_FRONT (pool);
  limit = POOL_LIMIT (pool);

  for (;;)
    {
      if (buffer->cur == buffer->rlimit)
	{
	  c = EOF;
	  unterminated (pfile, terminator);
	  break;
	}
      c = *buffer->cur++;

    have_char:
      /* Handle trigraphs, escaped newlines etc.  */
      if (c == '?' || c == '\\')
	c = skip_escaped_newlines (buffer, c);

      if (c == terminator && unescaped_terminator_p (pfile, dest))
	{
	  c = EOF;
	  break;
	}
      else if (is_vspace (c))
	{
	  /* In assembly language, silently terminate string and
	     character literals at end of line.  This is a kludge
	     around not knowing where comments are.  */
	  if (CPP_OPTION (pfile, lang) == CLK_ASM && terminator != '>')
	    break;

	  /* Character constants and header names may not extend over
	     multiple lines.  In Standard C, neither may strings.
	     Unfortunately, we accept multiline strings as an
	     extension, except in #include family directives.  */
	  if (terminator != '"' || pfile->state.angled_headers)
	    {
	      unterminated (pfile, terminator);
	      break;
	    }

	  if (pfile->mlstring_pos.line == 0)
	    {
	      pfile->mlstring_pos = pfile->lexer_pos;
	      if (CPP_PEDANTIC (pfile))
		cpp_pedwarn (pfile, "multi-line string constant");
	    }
	      
	  handle_newline (buffer, c);  /* Stores to read_ahead.  */
	  c = '\n';
	}
      else if (c == '\0')
	{
	  if (nulls++ == 0)
	    cpp_warning (pfile, "null character(s) preserved in literal");
	}

      /* No terminating null for strings - they could contain nulls.  */
      if (dest >= limit)
	limit = _cpp_next_chunk (pool, 0, &dest);
      *dest++ = c;

      /* If we had a new line, the next character is in read_ahead.  */
      if (c != '\n')
	continue;
      c = buffer->read_ahead;
      if (c != EOF)
	goto have_char;
    }

  /* Remember the next character.  */
  buffer->read_ahead = c;

  token->val.str.text = POOL_FRONT (pool);
  token->val.str.len = dest - token->val.str.text;
  POOL_COMMIT (pool, token->val.str.len);
}

/* The stored comment includes the comment start and any terminator.  */
static void
save_comment (pfile, token, from)
     cpp_reader *pfile;
     cpp_token *token;
     const unsigned char *from;
{
  unsigned char *buffer;
  unsigned int len;
  
  len = pfile->buffer->cur - from + 1; /* + 1 for the initial '/'.  */
  /* C++ comments probably (not definitely) have moved past a new
     line, which we don't want to save in the comment.  */
  if (pfile->buffer->read_ahead != EOF)
    len--;
  buffer = _cpp_pool_alloc (&pfile->ident_pool, len);
  
  token->type = CPP_COMMENT;
  token->val.str.len = len;
  token->val.str.text = buffer;

  buffer[0] = '/';
  memcpy (buffer + 1, from, len - 1);
}

/* Subroutine of lex_token to handle '%'.  A little tricky, since we
   want to avoid stepping back when lexing %:%X.  */
static void
lex_percent (buffer, result)
     cpp_buffer *buffer;
     cpp_token *result;
{
  cppchar_t c;

  result->type = CPP_MOD;
  /* Parsing %:%X could leave an extra character.  */
  if (buffer->extra_char == EOF)
    c = get_effective_char (buffer);
  else
    {
      c = buffer->read_ahead = buffer->extra_char;
      buffer->extra_char = EOF;
    }

  if (c == '=')
    ACCEPT_CHAR (CPP_MOD_EQ);
  else if (CPP_OPTION (buffer->pfile, digraphs))
    {
      if (c == ':')
	{
	  result->flags |= DIGRAPH;
	  ACCEPT_CHAR (CPP_HASH);
	  if (get_effective_char (buffer) == '%')
	    {
	      buffer->extra_char = get_effective_char (buffer);
	      if (buffer->extra_char == ':')
		{
		  buffer->extra_char = EOF;
		  ACCEPT_CHAR (CPP_PASTE);
		}
	      else
		/* We'll catch the extra_char when we're called back.  */
		buffer->read_ahead = '%';
	    }
	}
      else if (c == '>')
	{
	  result->flags |= DIGRAPH;
	  ACCEPT_CHAR (CPP_CLOSE_BRACE);
	}
    }
}

/* Subroutine of lex_token to handle '.'.  This is tricky, since we
   want to avoid stepping back when lexing '...' or '.123'.  In the
   latter case we should also set a flag for parse_number.  */
static void
lex_dot (pfile, result)
     cpp_reader *pfile;
     cpp_token *result;
{
  cpp_buffer *buffer = pfile->buffer;
  cppchar_t c;

  /* Parsing ..X could leave an extra character.  */
  if (buffer->extra_char == EOF)
    c = get_effective_char (buffer);
  else
    {
      c = buffer->read_ahead = buffer->extra_char;
      buffer->extra_char = EOF;
    }

  /* All known character sets have 0...9 contiguous.  */
  if (c >= '0' && c <= '9')
    {
      result->type = CPP_NUMBER;
      parse_number (pfile, &result->val.str, c, 1);
    }
  else
    {
      result->type = CPP_DOT;
      if (c == '.')
	{
	  buffer->extra_char = get_effective_char (buffer);
	  if (buffer->extra_char == '.')
	    {
	      buffer->extra_char = EOF;
	      ACCEPT_CHAR (CPP_ELLIPSIS);
	    }
	  else
	    /* We'll catch the extra_char when we're called back.  */
	    buffer->read_ahead = '.';
	}
      else if (c == '*' && CPP_OPTION (pfile, cplusplus))
	ACCEPT_CHAR (CPP_DOT_STAR);
    }
}

void
_cpp_lex_token (pfile, result)
     cpp_reader *pfile;
     cpp_token *result;
{
  cppchar_t c;
  cpp_buffer *buffer;
  const unsigned char *comment_start;
  unsigned char bol;

 skip:
  bol = pfile->state.next_bol;
 done_directive:
  buffer = pfile->buffer;
  pfile->state.next_bol = 0;
  result->flags = buffer->saved_flags;
  buffer->saved_flags = 0;
 next_char:
  pfile->lexer_pos.line = buffer->lineno;
 next_char2:
  pfile->lexer_pos.col = CPP_BUF_COLUMN (buffer, buffer->cur);

  c = buffer->read_ahead;
  if (c == EOF && buffer->cur < buffer->rlimit)
    {
      c = *buffer->cur++;
      pfile->lexer_pos.col++;
    }

 do_switch:
  buffer->read_ahead = EOF;
  switch (c)
    {
    case EOF:
      /* Non-empty files should end in a newline.  Ignore for command
	 line and _Pragma buffers.  */
      if (pfile->lexer_pos.col != 0 && !buffer->from_stage3)
	cpp_pedwarn (pfile, "no newline at end of file");
      pfile->state.next_bol = 1;
      pfile->skipping = 0;	/* In case missing #endif.  */
      result->type = CPP_EOF;
      /* Don't do MI optimisation.  */
      return;

    case ' ': case '\t': case '\f': case '\v': case '\0':
      skip_whitespace (pfile, c);
      result->flags |= PREV_WHITE;
      goto next_char2;

    case '\n': case '\r':
      if (!pfile->state.in_directive)
	{
	  handle_newline (buffer, c);
	  bol = 1;
	  pfile->lexer_pos.output_line = buffer->lineno;
	  /* This is a new line, so clear any white space flag.
	     Newlines in arguments are white space (6.10.3.10);
	     parse_arg takes care of that.  */
	  result->flags &= ~(PREV_WHITE | AVOID_LPASTE);
	  goto next_char;
	}

      /* Don't let directives spill over to the next line.  */
      buffer->read_ahead = c;
      pfile->state.next_bol = 1;
      result->type = CPP_EOF;
      /* Don't break; pfile->skipping might be true.  */
      return;

    case '?':
    case '\\':
      /* These could start an escaped newline, or '?' a trigraph.  Let
	 skip_escaped_newlines do all the work.  */
      {
	unsigned int lineno = buffer->lineno;

	c = skip_escaped_newlines (buffer, c);
	if (lineno != buffer->lineno)
	  /* We had at least one escaped newline of some sort, and the
	     next character is in buffer->read_ahead.  Update the
	     token's line and column.  */
	    goto next_char;

	/* We are either the original '?' or '\\', or a trigraph.  */
	result->type = CPP_QUERY;
	buffer->read_ahead = EOF;
	if (c == '\\')
	  goto random_char;
	else if (c != '?')
	  goto do_switch;
      }
      break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      result->type = CPP_NUMBER;
      parse_number (pfile, &result->val.str, c, 0);
      break;

    case '$':
      if (!CPP_OPTION (pfile, dollars_in_ident))
	goto random_char;
      /* Fall through... */

    case '_':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
      result->type = CPP_NAME;
      result->val.node = parse_identifier (pfile, c);

      /* 'L' may introduce wide characters or strings.  */
      if (result->val.node == pfile->spec_nodes.n_L)
	{
	  c = buffer->read_ahead; /* For make_string.  */
	  if (c == '\'' || c == '"')
	    {
	      ACCEPT_CHAR (c == '"' ? CPP_WSTRING: CPP_WCHAR);
	      goto make_string;
	    }
	}
      /* Convert named operators to their proper types.  */
      else if (result->val.node->flags & NODE_OPERATOR)
	{
	  result->flags |= NAMED_OP;
	  result->type = result->val.node->value.operator;
	}
      break;

    case '\'':
    case '"':
      result->type = c == '"' ? CPP_STRING: CPP_CHAR;
    make_string:
      parse_string (pfile, result, c);
      break;

    case '/':
      /* A potential block or line comment.  */
      comment_start = buffer->cur;
      result->type = CPP_DIV;
      c = get_effective_char (buffer);
      if (c == '=')
	ACCEPT_CHAR (CPP_DIV_EQ);
      if (c != '/' && c != '*')
	break;

      if (c == '*')
	{
	  if (skip_block_comment (pfile))
	    cpp_error_with_line (pfile, pfile->lexer_pos.line,
				 pfile->lexer_pos.col,
				 "unterminated comment");
	}
      else
	{
	  if (!CPP_OPTION (pfile, cplusplus_comments)
	      && !CPP_IN_SYSTEM_HEADER (pfile))
	    break;

	  /* Warn about comments only if pedantically GNUC89, and not
	     in system headers.  */
	  if (CPP_OPTION (pfile, lang) == CLK_GNUC89 && CPP_PEDANTIC (pfile)
	      && ! buffer->warned_cplusplus_comments)
	    {
	      cpp_pedwarn (pfile,
			   "C++ style comments are not allowed in ISO C89");
	      cpp_pedwarn (pfile,
			   "(this will be reported only once per input file)");
	      buffer->warned_cplusplus_comments = 1;
	    }

	  /* Skip_line_comment updates buffer->read_ahead.  */
	  if (skip_line_comment (pfile))
	    cpp_warning_with_line (pfile, pfile->lexer_pos.line,
				   pfile->lexer_pos.col,
				   "multi-line comment");
	}

      /* Skipping the comment has updated buffer->read_ahead.  */
      if (!pfile->state.save_comments)
	{
	  result->flags |= PREV_WHITE;
	  goto next_char;
	}

      /* Save the comment as a token in its own right.  */
      save_comment (pfile, result, comment_start);
      /* Don't do MI optimisation.  */
      return;

    case '<':
      if (pfile->state.angled_headers)
	{
	  result->type = CPP_HEADER_NAME;
	  c = '>';		/* terminator.  */
	  goto make_string;
	}

      result->type = CPP_LESS;
      c = get_effective_char (buffer);
      if (c == '=')
	ACCEPT_CHAR (CPP_LESS_EQ);
      else if (c == '<')
	{
	  ACCEPT_CHAR (CPP_LSHIFT);
	  if (get_effective_char (buffer) == '=')
	    ACCEPT_CHAR (CPP_LSHIFT_EQ);
	}
      else if (c == '?' && CPP_OPTION (pfile, cplusplus))
	{
	  ACCEPT_CHAR (CPP_MIN);
	  if (get_effective_char (buffer) == '=')
	    ACCEPT_CHAR (CPP_MIN_EQ);
	}
      else if (c == ':' && CPP_OPTION (pfile, digraphs))
	{
	  ACCEPT_CHAR (CPP_OPEN_SQUARE);
	  result->flags |= DIGRAPH;
	}
      else if (c == '%' && CPP_OPTION (pfile, digraphs))
	{
	  ACCEPT_CHAR (CPP_OPEN_BRACE);
	  result->flags |= DIGRAPH;
	}
      break;

    case '>':
      result->type = CPP_GREATER;
      c = get_effective_char (buffer);
      if (c == '=')
	ACCEPT_CHAR (CPP_GREATER_EQ);
      else if (c == '>')
	{
	  ACCEPT_CHAR (CPP_RSHIFT);
	  if (get_effective_char (buffer) == '=')
	    ACCEPT_CHAR (CPP_RSHIFT_EQ);
	}
      else if (c == '?' && CPP_OPTION (pfile, cplusplus))
	{
	  ACCEPT_CHAR (CPP_MAX);
	  if (get_effective_char (buffer) == '=')
	    ACCEPT_CHAR (CPP_MAX_EQ);
	}
      break;

    case '%':
      lex_percent (buffer, result);
      if (result->type == CPP_HASH)
	goto do_hash;
      break;

    case '.':
      lex_dot (pfile, result);
      break;

    case '+':
      result->type = CPP_PLUS;
      c = get_effective_char (buffer);
      if (c == '=')
	ACCEPT_CHAR (CPP_PLUS_EQ);
      else if (c == '+')
	ACCEPT_CHAR (CPP_PLUS_PLUS);
      break;

    case '-':
      result->type = CPP_MINUS;
      c = get_effective_char (buffer);
      if (c == '>')
	{
	  ACCEPT_CHAR (CPP_DEREF);
	  if (CPP_OPTION (pfile, cplusplus)
	      && get_effective_char (buffer) == '*')
	    ACCEPT_CHAR (CPP_DEREF_STAR);
	}
      else if (c == '=')
	ACCEPT_CHAR (CPP_MINUS_EQ);
      else if (c == '-')
	ACCEPT_CHAR (CPP_MINUS_MINUS);
      break;

    case '*':
      result->type = CPP_MULT;
      if (get_effective_char (buffer) == '=')
	ACCEPT_CHAR (CPP_MULT_EQ);
      break;

    case '=':
      result->type = CPP_EQ;
      if (get_effective_char (buffer) == '=')
	ACCEPT_CHAR (CPP_EQ_EQ);
      break;

    case '!':
      result->type = CPP_NOT;
      if (get_effective_char (buffer) == '=')
	ACCEPT_CHAR (CPP_NOT_EQ);
      break;

    case '&':
      result->type = CPP_AND;
      c = get_effective_char (buffer);
      if (c == '=')
	ACCEPT_CHAR (CPP_AND_EQ);
      else if (c == '&')
	ACCEPT_CHAR (CPP_AND_AND);
      break;
	  
    case '#':
      c = buffer->extra_char;	/* Can be set by error condition below.  */
      if (c != EOF)
	{
	  buffer->read_ahead = c;
	  buffer->extra_char = EOF;
	}
      else
	c = get_effective_char (buffer);

      if (c == '#')
	{
	  ACCEPT_CHAR (CPP_PASTE);
	  break;
	}

      result->type = CPP_HASH;
    do_hash:
      if (bol)
	{
	  if (pfile->state.parsing_args)
	    {
	      /* 6.10.3 paragraph 11: If there are sequences of
		 preprocessing tokens within the list of arguments that
		 would otherwise act as preprocessing directives, the
		 behavior is undefined.

		 This implementation will report a hard error, terminate
		 the macro invocation, and proceed to process the
		 directive.  */
	      cpp_error (pfile,
			 "directives may not be used inside a macro argument");

	      /* Put a '#' in lookahead, return CPP_EOF for parse_arg.  */
	      buffer->extra_char = buffer->read_ahead;
	      buffer->read_ahead = '#';
	      pfile->state.next_bol = 1;
	      result->type = CPP_EOF;

	      /* Get whitespace right - newline_in_args sets it.  */
	      if (pfile->lexer_pos.col == 1)
		result->flags &= ~(PREV_WHITE | AVOID_LPASTE);
	    }
	  else
	    {
	      /* This is the hash introducing a directive.  */
	      if (_cpp_handle_directive (pfile, result->flags & PREV_WHITE))
		goto done_directive; /* bol still 1.  */
	      /* This is in fact an assembler #.  */
	    }
	}
      break;

    case '|':
      result->type = CPP_OR;
      c = get_effective_char (buffer);
      if (c == '=')
	ACCEPT_CHAR (CPP_OR_EQ);
      else if (c == '|')
	ACCEPT_CHAR (CPP_OR_OR);
      break;

    case '^':
      result->type = CPP_XOR;
      if (get_effective_char (buffer) == '=')
	ACCEPT_CHAR (CPP_XOR_EQ);
      break;

    case ':':
      result->type = CPP_COLON;
      c = get_effective_char (buffer);
      if (c == ':' && CPP_OPTION (pfile, cplusplus))
	ACCEPT_CHAR (CPP_SCOPE);
      else if (c == '>' && CPP_OPTION (pfile, digraphs))
	{
	  result->flags |= DIGRAPH;
	  ACCEPT_CHAR (CPP_CLOSE_SQUARE);
	}
      break;

    case '~': result->type = CPP_COMPL; break;
    case ',': result->type = CPP_COMMA; break;
    case '(': result->type = CPP_OPEN_PAREN; break;
    case ')': result->type = CPP_CLOSE_PAREN; break;
    case '[': result->type = CPP_OPEN_SQUARE; break;
    case ']': result->type = CPP_CLOSE_SQUARE; break;
    case '{': result->type = CPP_OPEN_BRACE; break;
    case '}': result->type = CPP_CLOSE_BRACE; break;
    case ';': result->type = CPP_SEMICOLON; break;

    case '@':
      if (CPP_OPTION (pfile, objc))
	{
	  /* In Objective C, '@' may begin keywords or strings, like
	     @keyword or @"string".  It would be nice to call
	     get_effective_char here and test the result.  However, we
	     would then need to pass 2 characters to parse_identifier,
	     making it ugly and slowing down its main loop.  Instead,
	     we assume we have an identifier, and recover if not.  */
	  result->type = CPP_NAME;
	  result->val.node = parse_identifier (pfile, c);
	  if (result->val.node->length != 1)
	    break;

	  /* OK, so it wasn't an identifier.  Maybe a string?  */
	  if (buffer->read_ahead == '"')
	    {
	      c = '"';
	      ACCEPT_CHAR (CPP_OSTRING);
	      goto make_string;
	    }
	}
      goto random_char;

    random_char:
    default:
      result->type = CPP_OTHER;
      result->val.c = c;
      break;
    }

  if (pfile->skipping)
    goto skip;

  /* If not in a directive, this token invalidates controlling macros.  */
  if (!pfile->state.in_directive)
    pfile->mi_state = MI_FAILED;
}

/* An upper bound on the number of bytes needed to spell a token,
   including preceding whitespace.  */
unsigned int
cpp_token_len (token)
     const cpp_token *token;
{
  unsigned int len;

  switch (TOKEN_SPELL (token))
    {
    default:		len = 0;			break;
    case SPELL_STRING:	len = token->val.str.len;	break;
    case SPELL_IDENT:	len = token->val.node->length;	break;
    }
  /* 1 for whitespace, 4 for comment delimeters.  */
  return len + 5;
}

/* Write the spelling of a token TOKEN to BUFFER.  The buffer must
   already contain the enough space to hold the token's spelling.
   Returns a pointer to the character after the last character
   written.  */
unsigned char *
cpp_spell_token (pfile, token, buffer)
     cpp_reader *pfile;		/* Would be nice to be rid of this...  */
     const cpp_token *token;
     unsigned char *buffer;
{
  switch (TOKEN_SPELL (token))
    {
    case SPELL_OPERATOR:
      {
	const unsigned char *spelling;
	unsigned char c;

	if (token->flags & DIGRAPH)
	  spelling = digraph_spellings[token->type - CPP_FIRST_DIGRAPH];
	else if (token->flags & NAMED_OP)
	  goto spell_ident;
	else
	  spelling = TOKEN_NAME (token);
	
	while ((c = *spelling++) != '\0')
	  *buffer++ = c;
      }
      break;

    case SPELL_IDENT:
      spell_ident:
      memcpy (buffer, token->val.node->name, token->val.node->length);
      buffer += token->val.node->length;
      break;

    case SPELL_STRING:
      {
	int left, right, tag;
	switch (token->type)
	  {
	  case CPP_STRING:	left = '"';  right = '"';  tag = '\0'; break;
	  case CPP_WSTRING:	left = '"';  right = '"';  tag = 'L';  break;
	  case CPP_OSTRING:	left = '"';  right = '"';  tag = '@';  break;
	  case CPP_CHAR:	left = '\''; right = '\''; tag = '\0'; break;
    	  case CPP_WCHAR:	left = '\''; right = '\''; tag = 'L';  break;
	  case CPP_HEADER_NAME:	left = '<';  right = '>';  tag = '\0'; break;
	  default:		left = '\0'; right = '\0'; tag = '\0'; break;
	  }
	if (tag) *buffer++ = tag;
	if (left) *buffer++ = left;
	memcpy (buffer, token->val.str.text, token->val.str.len);
	buffer += token->val.str.len;
	if (right) *buffer++ = right;
      }
      break;

    case SPELL_CHAR:
      *buffer++ = token->val.c;
      break;

    case SPELL_NONE:
      cpp_ice (pfile, "Unspellable token %s", TOKEN_NAME (token));
      break;
    }

  return buffer;
}

/* Returns a token as a null-terminated string.  The string is
   temporary, and automatically freed later.  Useful for diagnostics.  */
unsigned char *
cpp_token_as_text (pfile, token)
     cpp_reader *pfile;
     const cpp_token *token;
{
  unsigned int len = cpp_token_len (token);
  unsigned char *start = _cpp_pool_alloc (&pfile->ident_pool, len), *end;

  end = cpp_spell_token (pfile, token, start);
  end[0] = '\0';

  return start;
}

/* Used by C front ends.  Should really move to using cpp_token_as_text.  */
const char *
cpp_type2name (type)
     enum cpp_ttype type;
{
  return (const char *) token_spellings[type].name;
}

/* Writes the spelling of token to FP.  Separate from cpp_spell_token
   for efficiency - to avoid double-buffering.  Also, outputs a space
   if PREV_WHITE is flagged.  */
void
cpp_output_token (token, fp)
     const cpp_token *token;
     FILE *fp;
{
  if (token->flags & PREV_WHITE)
    putc (' ', fp);

  switch (TOKEN_SPELL (token))
    {
    case SPELL_OPERATOR:
      {
	const unsigned char *spelling;

	if (token->flags & DIGRAPH)
	  spelling = digraph_spellings[token->type - CPP_FIRST_DIGRAPH];
	else if (token->flags & NAMED_OP)
	  goto spell_ident;
	else
	  spelling = TOKEN_NAME (token);

	ufputs (spelling, fp);
      }
      break;

    spell_ident:
    case SPELL_IDENT:
      ufputs (token->val.node->name, fp);
    break;

    case SPELL_STRING:
      {
	int left, right, tag;
	switch (token->type)
	  {
	  case CPP_STRING:	left = '"';  right = '"';  tag = '\0'; break;
	  case CPP_WSTRING:	left = '"';  right = '"';  tag = 'L';  break;
	  case CPP_OSTRING:	left = '"';  right = '"';  tag = '@';  break;
	  case CPP_CHAR:	left = '\''; right = '\''; tag = '\0'; break;
    	  case CPP_WCHAR:	left = '\''; right = '\''; tag = 'L';  break;
	  case CPP_HEADER_NAME:	left = '<';  right = '>';  tag = '\0'; break;
	  default:		left = '\0'; right = '\0'; tag = '\0'; break;
	  }
	if (tag) putc (tag, fp);
	if (left) putc (left, fp);
	fwrite (token->val.str.text, 1, token->val.str.len, fp);
	if (right) putc (right, fp);
      }
      break;

    case SPELL_CHAR:
      putc (token->val.c, fp);
      break;

    case SPELL_NONE:
      /* An error, most probably.  */
      break;
    }
}

/* Compare two tokens.  */
int
_cpp_equiv_tokens (a, b)
     const cpp_token *a, *b;
{
  if (a->type == b->type && a->flags == b->flags)
    switch (TOKEN_SPELL (a))
      {
      default:			/* Keep compiler happy.  */
      case SPELL_OPERATOR:
	return 1;
      case SPELL_CHAR:
	return a->val.c == b->val.c; /* Character.  */
      case SPELL_NONE:
	return (a->type != CPP_MACRO_ARG || a->val.arg_no == b->val.arg_no);
      case SPELL_IDENT:
	return a->val.node == b->val.node;
      case SPELL_STRING:
	return (a->val.str.len == b->val.str.len
		&& !memcmp (a->val.str.text, b->val.str.text,
			    a->val.str.len));
      }

  return 0;
}

#if 0
/* Compare two token lists.  */
int
_cpp_equiv_toklists (a, b)
     const struct toklist *a, *b;
{
  unsigned int i, count;

  count = a->limit - a->first;
  if (count != (b->limit - b->first))
    return 0;

  for (i = 0; i < count; i++)
    if (! _cpp_equiv_tokens (&a->first[i], &b->first[i]))
      return 0;

  return 1;
}
#endif

/* Determine whether two tokens can be pasted together, and if so,
   what the resulting token is.  Returns CPP_EOF if the tokens cannot
   be pasted, or the appropriate type for the merged token if they
   can.  */
enum cpp_ttype
cpp_can_paste (pfile, token1, token2, digraph)
     cpp_reader * pfile;
     const cpp_token *token1, *token2;
     int* digraph;
{
  enum cpp_ttype a = token1->type, b = token2->type;
  int cxx = CPP_OPTION (pfile, cplusplus);

  /* Treat named operators as if they were ordinary NAMEs.  */
  if (token1->flags & NAMED_OP)
    a = CPP_NAME;
  if (token2->flags & NAMED_OP)
    b = CPP_NAME;

  if (a <= CPP_LAST_EQ && b == CPP_EQ)
    return a + (CPP_EQ_EQ - CPP_EQ);

  switch (a)
    {
    case CPP_GREATER:
      if (b == a) return CPP_RSHIFT;
      if (b == CPP_QUERY && cxx)	return CPP_MAX;
      if (b == CPP_GREATER_EQ)	return CPP_RSHIFT_EQ;
      break;
    case CPP_LESS:
      if (b == a) return CPP_LSHIFT;
      if (b == CPP_QUERY && cxx)	return CPP_MIN;
      if (b == CPP_LESS_EQ)	return CPP_LSHIFT_EQ;
      if (CPP_OPTION (pfile, digraphs))
	{
	  if (b == CPP_COLON)
	    {*digraph = 1; return CPP_OPEN_SQUARE;} /* <: digraph */
	  if (b == CPP_MOD)
	    {*digraph = 1; return CPP_OPEN_BRACE;}	/* <% digraph */
	}
      break;

    case CPP_PLUS: if (b == a)	return CPP_PLUS_PLUS; break;
    case CPP_AND:  if (b == a)	return CPP_AND_AND; break;
    case CPP_OR:   if (b == a)	return CPP_OR_OR;   break;

    case CPP_MINUS:
      if (b == a)		return CPP_MINUS_MINUS;
      if (b == CPP_GREATER)	return CPP_DEREF;
      break;
    case CPP_COLON:
      if (b == a && cxx)	return CPP_SCOPE;
      if (b == CPP_GREATER && CPP_OPTION (pfile, digraphs))
	{*digraph = 1; return CPP_CLOSE_SQUARE;} /* :> digraph */
      break;

    case CPP_MOD:
      if (CPP_OPTION (pfile, digraphs))
	{
	  if (b == CPP_GREATER)
	    {*digraph = 1; return CPP_CLOSE_BRACE;}  /* %> digraph */
	  if (b == CPP_COLON)
	    {*digraph = 1; return CPP_HASH;}         /* %: digraph */
	}
      break;
    case CPP_DEREF:
      if (b == CPP_MULT && cxx)	return CPP_DEREF_STAR;
      break;
    case CPP_DOT:
      if (b == CPP_MULT && cxx)	return CPP_DOT_STAR;
      if (b == CPP_NUMBER)	return CPP_NUMBER;
      break;

    case CPP_HASH:
      if (b == a && (token1->flags & DIGRAPH) == (token2->flags & DIGRAPH))
	/* %:%: digraph */
	{*digraph = (token1->flags & DIGRAPH); return CPP_PASTE;}
      break;

    case CPP_NAME:
      if (b == CPP_NAME)	return CPP_NAME;
      if (b == CPP_NUMBER
	  && name_p (pfile, &token2->val.str)) return CPP_NAME;
      if (b == CPP_CHAR
	  && token1->val.node == pfile->spec_nodes.n_L) return CPP_WCHAR;
      if (b == CPP_STRING
	  && token1->val.node == pfile->spec_nodes.n_L) return CPP_WSTRING;
      break;

    case CPP_NUMBER:
      if (b == CPP_NUMBER)	return CPP_NUMBER;
      if (b == CPP_NAME)	return CPP_NUMBER;
      if (b == CPP_DOT)		return CPP_NUMBER;
      /* Numbers cannot have length zero, so this is safe.  */
      if ((b == CPP_PLUS || b == CPP_MINUS)
	  && VALID_SIGN ('+', token1->val.str.text[token1->val.str.len - 1]))
	return CPP_NUMBER;
      break;

    case CPP_OTHER:
      if (CPP_OPTION (pfile, objc) && token1->val.c == '@')
	{
	  if (b == CPP_NAME)	return CPP_NAME;
	  if (b == CPP_STRING)	return CPP_OSTRING;
	}

    default:
      break;
    }

  return CPP_EOF;
}

/* Returns nonzero if a space should be inserted to avoid an
   accidental token paste for output.  For simplicity, it is
   conservative, and occasionally advises a space where one is not
   needed, e.g. "." and ".2".  */

int
cpp_avoid_paste (pfile, token1, token2)
     cpp_reader *pfile;
     const cpp_token *token1, *token2;
{
  enum cpp_ttype a = token1->type, b = token2->type;
  cppchar_t c;

  if (token1->flags & NAMED_OP)
    a = CPP_NAME;
  if (token2->flags & NAMED_OP)
    b = CPP_NAME;

  c = EOF;
  if (token2->flags & DIGRAPH)
    c = digraph_spellings[b - CPP_FIRST_DIGRAPH][0];
  else if (token_spellings[b].category == SPELL_OPERATOR)
    c = token_spellings[b].name[0];

  /* Quickly get everything that can paste with an '='.  */
  if (a <= CPP_LAST_EQ && c == '=')
    return 1;

  switch (a)
    {
    case CPP_GREATER:	return c == '>' || c == '?';
    case CPP_LESS:	return c == '<' || c == '?' || c == '%' || c == ':';
    case CPP_PLUS:	return c == '+';
    case CPP_MINUS:	return c == '-' || c == '>';
    case CPP_DIV:	return c == '/' || c == '*'; /* Comments.  */
    case CPP_MOD:	return c == ':' || c == '>';
    case CPP_AND:	return c == '&';
    case CPP_OR:	return c == '|';
    case CPP_COLON:	return c == ':' || c == '>';
    case CPP_DEREF:	return c == '*';
    case CPP_DOT:	return c == '.' || c == '%' || b == CPP_NUMBER;
    case CPP_HASH:	return c == '#' || c == '%'; /* Digraph form.  */
    case CPP_NAME:	return ((b == CPP_NUMBER
				 && name_p (pfile, &token2->val.str))
				|| b == CPP_NAME
				|| b == CPP_CHAR || b == CPP_STRING); /* L */
    case CPP_NUMBER:	return (b == CPP_NUMBER || b == CPP_NAME
				|| c == '.' || c == '+' || c == '-');
    case CPP_OTHER:	return (CPP_OPTION (pfile, objc)
				&& token1->val.c == '@'
				&& (b == CPP_NAME || b == CPP_STRING));
    default:		break;
    }

  return 0;
}

/* Output all the remaining tokens on the current line, and a newline
   character, to FP.  Leading whitespace is removed.  */
void
cpp_output_line (pfile, fp)
     cpp_reader *pfile;
     FILE *fp;
{
  cpp_token token;

  cpp_get_token (pfile, &token);
  token.flags &= ~PREV_WHITE;
  while (token.type != CPP_EOF)
    {
      cpp_output_token (&token, fp);
      cpp_get_token (pfile, &token);
    }

  putc ('\n', fp);
}

/* Memory pools.  */

struct dummy
{
  char c;
  union
  {
    double d;
    int *p;
  } u;
};

#define DEFAULT_ALIGNMENT (offsetof (struct dummy, u))

static int
chunk_suitable (pool, chunk, size)
     cpp_pool *pool;
     cpp_chunk *chunk;
     unsigned int size;
{
  /* Being at least twice SIZE means we can use memcpy in
     _cpp_next_chunk rather than memmove.  Besides, it's a good idea
     anyway.  */
  return (chunk && pool->locked != chunk
	  && (unsigned int) (chunk->limit - chunk->base) >= size * 2);
}

/* Returns the end of the new pool.  PTR points to a char in the old
   pool, and is updated to point to the same char in the new pool.  */
unsigned char *
_cpp_next_chunk (pool, len, ptr)
     cpp_pool *pool;
     unsigned int len;
     unsigned char **ptr;
{
  cpp_chunk *chunk = pool->cur->next;

  /* LEN is the minimum size we want in the new pool.  */
  len += POOL_ROOM (pool);
  if (! chunk_suitable (pool, chunk, len))
    {
      chunk = new_chunk (POOL_SIZE (pool) * 2 + len);

      chunk->next = pool->cur->next;
      pool->cur->next = chunk;
    }

  /* Update the pointer before changing chunk's front.  */
  if (ptr)
    *ptr += chunk->base - POOL_FRONT (pool);

  memcpy (chunk->base, POOL_FRONT (pool), POOL_ROOM (pool));
  chunk->front = chunk->base;

  pool->cur = chunk;
  return POOL_LIMIT (pool);
}

static cpp_chunk *
new_chunk (size)
     unsigned int size;
{
  unsigned char *base;
  cpp_chunk *result;

  size = POOL_ALIGN (size, DEFAULT_ALIGNMENT);
  base = (unsigned char *) xmalloc (size + sizeof (cpp_chunk));
  /* Put the chunk descriptor at the end.  Then chunk overruns will
     cause obvious chaos.  */
  result = (cpp_chunk *) (base + size);
  result->base = base;
  result->front = base;
  result->limit = base + size;
  result->next = 0;

  return result;
}

void
_cpp_init_pool (pool, size, align, temp)
     cpp_pool *pool;
     unsigned int size, align, temp;
{
  if (align == 0)
    align = DEFAULT_ALIGNMENT;
  if (align & (align - 1))
    abort ();
  pool->align = align;
  pool->cur = new_chunk (size);
  pool->locked = 0;
  pool->locks = 0;
  if (temp)
    pool->cur->next = pool->cur;
}

void
_cpp_lock_pool (pool)
     cpp_pool *pool;
{
  if (pool->locks++ == 0)
    pool->locked = pool->cur;
}

void
_cpp_unlock_pool (pool)
     cpp_pool *pool;
{
  if (--pool->locks == 0)
    pool->locked = 0;
}

void
_cpp_free_pool (pool)
     cpp_pool *pool;
{
  cpp_chunk *chunk = pool->cur, *next;

  do
    {
      next = chunk->next;
      free (chunk->base);
      chunk = next;
    }
  while (chunk && chunk != pool->cur);
}

/* Reserve LEN bytes from a memory pool.  */
unsigned char *
_cpp_pool_reserve (pool, len)
     cpp_pool *pool;
     unsigned int len;
{
  len = POOL_ALIGN (len, pool->align);
  if (len > (unsigned int) POOL_ROOM (pool))
    _cpp_next_chunk (pool, len, 0);

  return POOL_FRONT (pool);
}

/* Allocate LEN bytes from a memory pool.  */
unsigned char *
_cpp_pool_alloc (pool, len)
     cpp_pool *pool;
     unsigned int len;
{
  unsigned char *result = _cpp_pool_reserve (pool, len);

  POOL_COMMIT (pool, len);
  return result;
}

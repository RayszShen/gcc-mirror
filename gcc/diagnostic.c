/* Language-independent diagnostic subroutines for the GNU C compiler
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.
   Contributed by Gabriel Dos Reis <gdr@codesourcery.com>

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


/* This file implements the language independent aspect of diagnostic
   message module.  */

#include "config.h"
#undef FLOAT /* This is for hpux. They should change hpux.  */
#undef FFS  /* Some systems define this in param.h.  */
#include "system.h"

#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "flags.h"
#include "input.h"
#include "insn-attr.h"
#include "insn-config.h"
#include "toplev.h"
#include "intl.h"
#include "diagnostic.h"

#define output_formatted_integer(BUFFER, FORMAT, INTEGER) \
  do {                                                    \
    sprintf ((BUFFER)->digit_buffer, FORMAT, INTEGER);    \
    output_add_string (BUFFER, (BUFFER)->digit_buffer);   \
  } while (0)

#define output_text_length(BUFFER) (BUFFER)->line_length
#define is_starting_newline(BUFFER) (output_text_length (BUFFER) == 0)
#define output_prefix(BUFFER) (BUFFER)->state.prefix
#define line_wrap_cutoff(BUFFER) (BUFFER)->state.maximum_length
#define prefix_was_emitted_for(BUFFER) (BUFFER)->state.emitted_prefix_p
#define output_buffer_ptr_to_format_args(BUFFER) (BUFFER)->state.format_args

#define diagnostic_args output_buffer_ptr_to_format_args (diagnostic_buffer)
#define diagnostic_msg output_buffer_text_cursor (diagnostic_buffer)

/* Prototypes.  */
static void diagnostic_finish PARAMS ((output_buffer *));
static void output_do_verbatim PARAMS ((output_buffer *,
                                        const char *, va_list *));
static void output_buffer_to_stream PARAMS ((output_buffer *));
static void output_format PARAMS ((output_buffer *));
static void output_indent PARAMS ((output_buffer *));

static char *vbuild_message_string PARAMS ((const char *, va_list))
     ATTRIBUTE_PRINTF (1, 0);
static char *build_message_string PARAMS ((const char *, ...))
     ATTRIBUTE_PRINTF_1;
static void output_do_printf PARAMS ((output_buffer *, const char *))
     ATTRIBUTE_PRINTF (2, 0);
static void format_with_decl PARAMS ((output_buffer *, tree));
static void file_and_line_for_asm PARAMS ((rtx, const char **, int *));
static void diagnostic_for_asm PARAMS ((rtx, const char *, va_list *, int));
static void diagnostic_for_decl PARAMS ((tree, const char *, va_list *, int));
static void set_real_maximum_length PARAMS ((output_buffer *));

static void output_unsigned_decimal PARAMS ((output_buffer *, unsigned int));
static void output_long_decimal PARAMS ((output_buffer *, long int));
static void output_long_unsigned_decimal PARAMS ((output_buffer *,
                                                  long unsigned int));
static void output_octal PARAMS ((output_buffer *, unsigned int));
static void output_long_octal PARAMS ((output_buffer *, unsigned long int));
static void output_hexadecimal PARAMS ((output_buffer *, unsigned int));
static void output_long_hexadecimal PARAMS ((output_buffer *,
                                             unsigned long int));
static void output_append_r PARAMS ((output_buffer *, const char *, int));
static void wrap_text PARAMS ((output_buffer *, const char *, const char *));
static void maybe_wrap_text PARAMS ((output_buffer *, const char *,
                                     const char *));
static void clear_diagnostic_info PARAMS ((output_buffer *));

static void default_diagnostic_starter PARAMS ((output_buffer *,
                                                diagnostic_context *));
static void default_diagnostic_finalizer PARAMS ((output_buffer *,
                                                  diagnostic_context *));

static void error_recursion PARAMS ((void)) ATTRIBUTE_NORETURN;
static diagnostic_state *diagnostic_push_state PARAMS ((output_buffer *));
static void diagnostic_pop_state PARAMS ((output_buffer *));

extern int rtl_dump_and_exit;
extern int warnings_are_errors;

/* A diagnostic_context surrogate for stderr.  */
static diagnostic_context global_diagnostic_context;
diagnostic_context *global_dc = &global_diagnostic_context;

/* This will be removed shortly.  */
output_buffer *diagnostic_buffer = &global_diagnostic_context.buffer;

/* Function of last error message;
   more generally, function such that if next error message is in it
   then we don't have to mention the function name.  */
static tree last_error_function = NULL;

/* Used to detect when input_file_stack has changed since last described.  */
static int last_error_tick;

/* Hooks for language specific diagnostic messages pager and finalizer.  */
diagnostic_starter_fn lang_diagnostic_starter;
diagnostic_finalizer_fn lang_diagnostic_finalizer;

/* Maximum characters per line in automatic line wrapping mode.
   Zero means don't wrap lines. */

void (*print_error_function) PARAMS ((diagnostic_context *, const char *))
     = default_print_error_function;

/* Prevent recursion into the error handler.  */
static int diagnostic_lock;


/* Return truthvalue if current input file is different from the most recent
   file involved in a diagnostic message.  */

int
error_module_changed ()
{
  return last_error_tick != input_file_stack_tick;
}

/* Remember current file as being the most recent file involved in a
   diagnostic message.  */

void
record_last_error_module ()
{
  last_error_tick = input_file_stack_tick;
}

/* Same as error_module_changed, but for function.  */

int
error_function_changed ()
{
  return last_error_function != current_function_decl;
}

/* Same as record_last_error_module, but for function.  */

void
record_last_error_function ()
{
  last_error_function = current_function_decl;
}

/* Initialize the diagnostic message outputting machinery.  */

void
diagnostic_initialize (context)
     diagnostic_context *context;
{
  memset (context, 0, sizeof *context);
  context->buffer.messages = dyn_string_new (0);
  context->buffer.ds = ((diagnostic_state *) 
			xcalloc (1, sizeof (diagnostic_state)));

  /* By default, diagnostics are sent to stderr.  */
  output_buffer_attached_stream (&context->buffer) = stderr;

  /* By default, we emit prefixes once per message.  */
  diagnostic_prefixing_rule (context) = DIAGNOSTICS_SHOW_PREFIX_ONCE;

  diagnostic_starter (context) = default_diagnostic_starter;
  diagnostic_finalizer (context) = default_diagnostic_finalizer;
}

/* Returns true if BUFFER is in line-wrappind mode.  */

int
output_is_line_wrapping (buffer)
     output_buffer *buffer;
{
  return diagnostic_line_cutoff (buffer) > 0;
}

/* Return BUFFER's prefix.  */

const char *
output_get_prefix (buffer)
     const output_buffer *buffer;
{
  return output_prefix (buffer);
}

/* Subroutine of output_set_maximum_length.  Set up BUFFER's
   internal maximum characters per line.  */

static void
set_real_maximum_length (buffer)
     output_buffer *buffer;
{
  /* If we're told not to wrap lines then do the obvious thing.  In case
   we'll emit prefix only once per diagnostic message, it is appropriate
  not to increase unncessarily the line-length cut-off.  */
  if (! output_is_line_wrapping (buffer)
      || diagnostic_prefixing_rule (buffer) == DIAGNOSTICS_SHOW_PREFIX_ONCE
      || diagnostic_prefixing_rule (buffer) == DIAGNOSTICS_SHOW_PREFIX_NEVER)
    line_wrap_cutoff (buffer) = diagnostic_line_cutoff (buffer);
  else
    {
      int prefix_length =
        output_prefix (buffer) ? strlen (output_prefix (buffer)) : 0;
      /* If the prefix is ridiculously too long, output at least
         32 characters.  */
      if (diagnostic_line_cutoff (buffer) - prefix_length < 32)
        line_wrap_cutoff (buffer) = diagnostic_line_cutoff (buffer) + 32;
      else
        line_wrap_cutoff (buffer) = diagnostic_line_cutoff (buffer);
    }
}

/* Sets the number of maximum characters per line BUFFER can output
   in line-wrapping mode.  A LENGTH value 0 suppresses line-wrapping.  */

void
output_set_maximum_length (buffer, length)
     output_buffer *buffer;
     int length;
{
  diagnostic_line_cutoff (buffer) = length;
  set_real_maximum_length (buffer);
}

/* Sets BUFFER's PREFIX.  */

void
output_set_prefix (buffer, prefix)
     output_buffer *buffer;
     const char *prefix;
{
  output_prefix (buffer) = prefix;
  set_real_maximum_length (buffer);
  prefix_was_emitted_for (buffer) = 0;
  output_indentation (buffer) = 0;
}

/*  Return a pointer to the last character emitted in the current
    diagnostic message.  */

const char *
output_last_position (buffer)
     const output_buffer *buffer;
{
  const char *end;

  end = buffer->messages->s + dyn_string_length (buffer->messages);

  if (end != buffer->messages->s)
    return end - 1;

  return NULL;
}

/* Free BUFFER's prefix, a previously malloc'd string.  */

void
output_destroy_prefix (buffer)
     output_buffer *buffer;
{
  if (output_prefix (buffer) != NULL)
    {
      free ((char *) output_prefix (buffer));
      output_prefix (buffer) = NULL;
    }
}

/* Remove any queued messages from the BUFFER without issuing them.  */

void
output_clear_message_text (buffer)
     output_buffer *buffer;
{
  dyn_string_clear (buffer->messages);
  output_text_length (buffer) = 0;
}

/* Zero out any diagnostic data used so far by BUFFER.  */

static void
clear_diagnostic_info (buffer)
     output_buffer *buffer;
{
  output_buffer_text_cursor (buffer) = NULL;
  output_buffer_ptr_to_format_args (buffer) = NULL;
  prefix_was_emitted_for (buffer) = 0;
  output_indentation (buffer) = 0;
}

/* Construct an output BUFFER with PREFIX and of MAXIMUM_LENGTH
   characters per line.  */

void
init_output_buffer (buffer, prefix, maximum_length)
     output_buffer *buffer;
     const char *prefix;
     int maximum_length;
{
  memset (buffer, 0, sizeof (output_buffer));
  buffer->messages = dyn_string_new (0);
  output_buffer_attached_stream (buffer) = stderr;
  diagnostic_line_cutoff (buffer) = maximum_length;
  diagnostic_prefixing_rule (buffer) = diagnostic_prefixing_rule (global_dc);
  output_set_prefix (buffer, prefix);
  output_text_length (buffer) = 0;
  clear_diagnostic_info (buffer);  
  buffer->ds = ((diagnostic_state *) 
		xcalloc (1, sizeof (diagnostic_state)));
}

/* Reinitialize BUFFER.  */

void
output_clear (buffer)
     output_buffer *buffer;
{
  output_clear_message_text (buffer);
  clear_diagnostic_info (buffer);
}

void
flush_diagnostic_buffer ()
{
  output_buffer_to_stream (diagnostic_buffer);
}

/* Return the amount of characters BUFFER can accept to
   make a full line.  */

int
output_space_left (buffer)
     const output_buffer *buffer;
{
  return line_wrap_cutoff (buffer) - output_text_length (buffer);
}

/* Write out BUFFER's prefix.  */

void
output_emit_prefix (buffer)
     output_buffer *buffer;
{
  if (output_prefix (buffer) != NULL)
    {
      switch (diagnostic_prefixing_rule (buffer))
        {
        default:
        case DIAGNOSTICS_SHOW_PREFIX_NEVER:
          break;

        case DIAGNOSTICS_SHOW_PREFIX_ONCE:
          if (prefix_was_emitted_for (buffer))
            {
              output_indent (buffer);
              break;
            }
          output_indentation (buffer) += 3;          
          /* Fall through.  */

        case DIAGNOSTICS_SHOW_PREFIX_EVERY_LINE:
          {
            int prefix_length = strlen (output_prefix (buffer));
            output_append_r (buffer, output_prefix (buffer), prefix_length);
            prefix_was_emitted_for (buffer) = 1;
          }
          break;
        }
    }
}

/* Have BUFFER start a new line.  */

void
output_add_newline (buffer)
     output_buffer *buffer;
{
  dyn_string_append_char (buffer->messages, '\n');
  output_text_length (buffer) = 0;
}

/* Appends a character to BUFFER.  */

void
output_add_character (buffer, c)
     output_buffer *buffer;
     int c;
{
  if (output_is_line_wrapping (buffer) && output_space_left (buffer) <= 0)
    output_add_newline (buffer);
  dyn_string_append_char (buffer->messages, c);
  ++output_text_length (buffer);
}

/* Adds a space to BUFFER.  */

void
output_add_space (buffer)
     output_buffer *buffer;
{
  if (output_is_line_wrapping (buffer) && output_space_left (buffer) <= 0)
    {
      output_add_newline (buffer);
      return;
    }
  dyn_string_append_char (buffer->messages, ' ');
  ++output_text_length (buffer);
}

/* These functions format an INTEGER into BUFFER as suggested by their
   names.  */

void
output_decimal (buffer, i)
     output_buffer *buffer;
     int i;
{
  output_formatted_integer (buffer, "%d", i);
}

static void
output_long_decimal (buffer, i)
     output_buffer *buffer;
     long int i;
{
  output_formatted_integer (buffer, "%ld", i);
}

static void
output_unsigned_decimal (buffer, i)
     output_buffer *buffer;
     unsigned int i;
{
  output_formatted_integer (buffer, "%u", i);
}

static void
output_long_unsigned_decimal (buffer, i)
     output_buffer *buffer;
     long unsigned int i;
{
  output_formatted_integer (buffer, "%lu", i);
}

static void
output_octal (buffer, i)
     output_buffer *buffer;
     unsigned int i;
{
  output_formatted_integer (buffer, "%o", i);
}

static void
output_long_octal (buffer, i)
     output_buffer *buffer;
     unsigned long int i;
{
  output_formatted_integer (buffer, "%lo", i);
}

static void
output_hexadecimal (buffer, i)
     output_buffer *buffer;
     unsigned int i;
{
  output_formatted_integer (buffer, "%x", i);
}

static void
output_long_hexadecimal (buffer, i)
     output_buffer *buffer;
     unsigned long int i;
{
  output_formatted_integer (buffer, "%lx", i);
}

/* Append to BUFFER a string specified by its STARTING character
   and LENGTH.  */

static void
output_append_r (buffer, start, length)
     output_buffer *buffer;
     const char *start;
     int length;
{
  dyn_string_append_cstr_len (buffer->messages, start, length);
  output_text_length (buffer) += length;
}

/* Append a string deliminated by START and END to BUFFER.  No wrapping is
   done.  However, if beginning a new line then emit output_prefix (BUFFER)
   and skip any leading whitespace if appropriate.  The caller must ensure
   that it is safe to do so.  */

void
output_append (buffer, start, end)
     output_buffer *buffer;
     const char *start;
     const char *end;
{
  /* Emit prefix and skip whitespace if we're starting a new line.  */
  if (is_starting_newline (buffer))
    {
      output_emit_prefix (buffer);
      if (output_is_line_wrapping (buffer))
        while (start != end && *start == ' ')
          ++start;
    }
  output_append_r (buffer, start, end - start);
}

static void
output_indent (buffer)
     output_buffer *buffer;
{
  int n = output_indentation (buffer);
  int i;

  for (i = 0; i < n; ++i)
    output_add_character (buffer, ' ');
}

/* Wrap a text delimited by START and END into BUFFER.  */

static void
wrap_text (buffer, start, end)
     output_buffer *buffer;
     const char *start;
     const char *end;
{
  int is_wrapping = output_is_line_wrapping (buffer);
  
  while (start != end)
    {
      /* Dump anything bodered by whitespaces.  */ 
      {
        const char *p = start;
        while (p != end && *p != ' ' && *p != '\n')
          ++p;
        if (is_wrapping && p - start >= output_space_left (buffer))
          output_add_newline (buffer);
        output_append (buffer, start, p);
        start = p;
      }

      if (start != end && *start == ' ')
        {
          output_add_space (buffer);
          ++start;
        }
      if (start != end && *start == '\n')
        {
          output_add_newline (buffer);
          ++start;
        }
    }
}

/* Same as wrap_text but wrap text only when in line-wrapping mode.  */

static void
maybe_wrap_text (buffer, start, end)
     output_buffer *buffer;
     const char *start;
     const char *end;
{
  if (output_is_line_wrapping (buffer))
    wrap_text (buffer, start, end);
  else
    output_append (buffer, start, end);
}


/* Append a STRING to BUFFER; the STRING might be line-wrapped if in
   appropriate mode.  */

void
output_add_string (buffer, str)
     output_buffer *buffer;
     const char *str;
{
  maybe_wrap_text (buffer, str, str + (str ? strlen (str) : 0));
}

/* Ouput the current pending diagnostic message to the BUFFER.  */

static void
output_buffer_to_stream (buffer)
     output_buffer *buffer;
{
  output_add_newline (buffer);

  if (!buffer->ds->tentative_diagnostic)
    {
      const char *text = buffer->messages->s;
      fputs (text, output_buffer_attached_stream (buffer));
      output_clear_message_text (buffer);
    }
}

/* Format a message pointed to by output_buffer_text_cursor (BUFFER) using
   output_buffer_format_args (BUFFER) as appropriate.  The following format
   specifiers are recognized as being language independent:
   %d, %i: (signed) integer in base ten.
   %u: unsigned integer in base ten.
   %o: unsigned integer in base eight.
   %x: unsigned integer in base sixteen.
   %ld, %li, %lo, %lu, %lx: long versions of the above.
   %c: character.
   %s: string.
   %%: `%'.
   %*.s: a substring the length of which is specified by an integer.  */

static void
output_format (buffer)
     output_buffer *buffer;
{
  for (; *output_buffer_text_cursor (buffer);
       ++output_buffer_text_cursor (buffer))
    {
      int long_integer = 0;

      /* Ignore text.  */
      {
        const char *p = output_buffer_text_cursor (buffer);
        while (*p && *p != '%')
          ++p;
        wrap_text (buffer, output_buffer_text_cursor (buffer), p);
        output_buffer_text_cursor (buffer) = p;
      }

      if (!*output_buffer_text_cursor (buffer))
        break;

      /* We got a '%'.  Let's see what happens. Record whether we're
         parsing a long integer format specifier.  */
      if (*++output_buffer_text_cursor (buffer) == 'l')
        {
          long_integer = 1;
          ++output_buffer_text_cursor (buffer);
        }

      /* Handle %c, %d, %i, %ld, %li, %lo, %lu, %lx, %o, %s, %u,
         %x, %.*s; %%.  And nothing else.  Front-ends should install
         printers to grok language specific format specifiers.  */
      switch (*output_buffer_text_cursor (buffer))
        {
        case 'c':
          output_add_character
            (buffer, va_arg (output_buffer_format_args (buffer), int));
          break;
          
        case 'd':
        case 'i':
          if (long_integer)
            output_long_decimal
              (buffer, va_arg (output_buffer_format_args (buffer), long int));
          else
            output_decimal
              (buffer, va_arg (output_buffer_format_args (buffer), int));
          break;

        case 'o':
          if (long_integer)
            output_long_octal (buffer,
                               va_arg (output_buffer_format_args (buffer),
                                       unsigned long int));
          else
            output_octal (buffer,
                          va_arg (output_buffer_format_args (buffer),
                                  unsigned int));
          break;

        case 's':
          output_add_string (buffer,
                             va_arg (output_buffer_format_args (buffer),
                                     const char *));
          break;

        case 'u':
          if (long_integer)
            output_long_unsigned_decimal
              (buffer, va_arg (output_buffer_format_args (buffer),
                               long unsigned int));
          else
            output_unsigned_decimal
              (buffer, va_arg (output_buffer_format_args (buffer),
                               unsigned int));
          break;
          
        case 'x':
          if (long_integer)
            output_long_hexadecimal
              (buffer, va_arg (output_buffer_format_args (buffer),
                               unsigned long int));
          else
            output_hexadecimal
              (buffer, va_arg (output_buffer_format_args (buffer),
                               unsigned int));
          break;

        case '%':
          output_add_character (buffer, '%');
          break;

        case '.':
          {
            int n;
            const char *s;
            /* We handle no precision specifier but `%.*s'.  */
            if (*++output_buffer_text_cursor (buffer) != '*')
              abort ();
            else if (*++output_buffer_text_cursor (buffer) != 's')
              abort();
            n = va_arg (output_buffer_format_args (buffer), int);
            s = va_arg (output_buffer_format_args (buffer), const char *);
            output_append (buffer, s, s + n);
          }
          break;

        default:
          if (!buffer->format_decoder || !(*buffer->format_decoder) (buffer))
            {
              /* Hmmm.  The front-end failed to install a format translator
                 but called us with an unrecognized format.  Sorry.  */
              abort ();
            }
        }
    }
}

static char *
vbuild_message_string (msg, ap)
     const char *msg;
     va_list ap;
{
  char *str;

  vasprintf (&str, msg, ap);
  return str;
}

/*  Return a malloc'd string containing MSG formatted a la
    printf.  The caller is reponsible for freeing the memory.  */

static char *
build_message_string VPARAMS ((const char *msg, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *msg;
#endif
  va_list ap;
  char *str;

  VA_START (ap, msg);

#ifndef ANSI_PROTOTYPES
  msg = va_arg (ap, const char *);
#endif

  str = vbuild_message_string (msg, ap);

  va_end (ap);

  return str;
}

/* Return a malloc'd string describing a location.  The caller is
   responsible for freeing the memory.  */

char *
context_as_prefix (file, line, warn)
     const char *file;
     int line;
     int warn;
{
  if (file)
    {
      if (warn)
	return build_message_string (_("%s:%d: warning: "), file, line);
      else
	return build_message_string ("%s:%d: ", file, line);
    }
  else
    {
      if (warn)
	return build_message_string (_("%s: warning: "), progname);
      else
	return build_message_string ("%s: ", progname);
    }
}

/* Same as context_as_prefix, but only the source FILE is given.  */

char *
file_name_as_prefix (f)
     const char *f;
{
  return build_message_string ("%s: ", f);
}

/* Format a MESSAGE into BUFFER.  Automatically wrap lines.  */

static void
output_do_printf (buffer, msg)
     output_buffer *buffer;
     const char *msg;
{
  char *message = vbuild_message_string (msg,
                                         output_buffer_format_args (buffer));

  wrap_text (buffer, message, message + strlen (message));
  free (message);
}


/* Format a message into BUFFER a la printf.  */

void
output_printf VPARAMS ((struct output_buffer *buffer, const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  struct output_buffer *buffer;
  const char *msgid;
#endif
  va_list ap;
  va_list *old_args;

  VA_START (ap, msgid);
#ifndef ANSI_PROTOTYPES
  buffer = va_arg (ap, output_buffer *);
  msgid = va_arg (ap, const char *);
#endif
  old_args = output_buffer_ptr_to_format_args (buffer);
  output_buffer_ptr_to_format_args (buffer) = &ap;
  output_do_printf (buffer, _(msgid));
  output_buffer_ptr_to_format_args (buffer) = old_args;
  va_end (ap);
}

/* Print a message relevant to the given DECL.  */

static void
format_with_decl (buffer, decl)
     output_buffer *buffer;
     tree decl;
{
  const char *p;
  
  /* Do magic to get around lack of varargs support for insertion
     of arguments into existing list.  We know that the decl is first;
     we ass_u_me that it will be printed with "%s".  */
  for (p = output_buffer_text_cursor (buffer); *p; ++p)
    {
      if (*p == '%')
	{
	  if (*(p + 1) == '%')
	    ++p;
	  else if (*(p + 1) != 's')
	    abort ();
	  else
	    break;
	}
    }

  /* Print the left-hand substring.  */
  maybe_wrap_text (buffer, output_buffer_text_cursor (buffer), p);
  
  if (*p == '%')		/* Print the name.  */
    {
      const char *n = (DECL_NAME (decl)
		 ? (*decl_printable_name) (decl, 2)
		 : _("((anonymous))"));
      output_add_string (buffer, n);
      while (*p)
	{
	  ++p;
	  if (ISALPHA (*(p - 1) & 0xFF))
	    break;
	}
    }

  if (*p)			/* Print the rest of the message.  */
    {
      output_buffer_text_cursor (buffer) = p;
      output_format (buffer);
    }
}

/* Figure file and line of the given INSN.  */

static void
file_and_line_for_asm (insn, pfile, pline)
     rtx insn;
     const char **pfile;
     int *pline;
{
  rtx body = PATTERN (insn);
  rtx asmop;

  /* Find the (or one of the) ASM_OPERANDS in the insn.  */
  if (GET_CODE (body) == SET && GET_CODE (SET_SRC (body)) == ASM_OPERANDS)
    asmop = SET_SRC (body);
  else if (GET_CODE (body) == ASM_OPERANDS)
    asmop = body;
  else if (GET_CODE (body) == PARALLEL
	   && GET_CODE (XVECEXP (body, 0, 0)) == SET)
    asmop = SET_SRC (XVECEXP (body, 0, 0));
  else if (GET_CODE (body) == PARALLEL
	   && GET_CODE (XVECEXP (body, 0, 0)) == ASM_OPERANDS)
    asmop = XVECEXP (body, 0, 0);
  else
    asmop = NULL;

  if (asmop)
    {
      *pfile = ASM_OPERANDS_SOURCE_FILE (asmop);
      *pline = ASM_OPERANDS_SOURCE_LINE (asmop);
    }
  else
    {
      *pfile = input_filename;
      *pline = lineno;
    }
}

/* Report a diagnostic MESSAGE (an errror or a WARNING) at the line number
   of the insn INSN.  This is used only when INSN is an `asm' with operands,
   and each ASM_OPERANDS records its own source file and line.  */

static void
diagnostic_for_asm (insn, msg, args_ptr, warn)
     rtx insn;
     const char *msg;
     va_list *args_ptr;
     int warn;
{
  diagnostic_context dc;

  set_diagnostic_context (&dc, msg, args_ptr, NULL, 0, warn);
  file_and_line_for_asm (insn, &diagnostic_file_location (&dc),
                         &diagnostic_line_location (&dc));
  report_diagnostic (&dc);
}

/* Report a diagnostic MESSAGE at the declaration DECL.
   MSG is a format string which uses %s to substitute the declaration
   name; subsequent substitutions are a la output_format.  */

static void
diagnostic_for_decl (decl, msgid, args_ptr, warn)
     tree decl;
     const char *msgid;
     va_list *args_ptr;
     int warn;
{
  output_state os;

  if (diagnostic_lock++)
    error_recursion ();

  if (count_error (warn))
    {
      os = output_buffer_state (diagnostic_buffer);
      report_error_function (DECL_SOURCE_FILE (decl));
      output_set_prefix
	(diagnostic_buffer, context_as_prefix
	 (DECL_SOURCE_FILE (decl), DECL_SOURCE_LINE (decl), warn));
      output_buffer_ptr_to_format_args (diagnostic_buffer) = args_ptr;
      output_buffer_text_cursor (diagnostic_buffer) = _(msgid);
      format_with_decl (diagnostic_buffer, decl);
      diagnostic_finish ((output_buffer *)global_dc);
      output_destroy_prefix (diagnostic_buffer);
  
      output_buffer_state (diagnostic_buffer) = os;
    }
  diagnostic_lock--;
}


/* Push a new diagnostic_state entry on the stack.  */

static diagnostic_state *
diagnostic_push_state (buffer)
     output_buffer *buffer;
{
  diagnostic_state *ds;

  /* Create a new entry to put on the stack.  */
  ds = (diagnostic_state *) xmalloc (sizeof (diagnostic_state));
  /* Copy the data from the previous entry.  */
  *ds = *buffer->ds;
  /* Assume that we will not issue diagnostics tentatively.  */
  ds->tentative_diagnostic = 0;
  /* And that there are no saved messages.  */
  ds->saved_messages = NULL;
  /* Link this entry onto the stack.  */
  ds->next = buffer->ds;
  buffer->ds = ds;

  return ds;
}

/* Remove the topmost entry from the stack.  */

static void
diagnostic_pop_state (buffer)
     output_buffer *buffer;
{
  diagnostic_state *ds;

  /* Get the active state.  */
  ds = buffer->ds;
  /* Unlink it from the list.  */
  buffer->ds = ds->next;
  /* Free the associated storage.  */
  free (ds);
}

/* Begin issuing diagnostics tentatively.  When a diagnostic is issued,
   no message will be printed.  Instead, the message will be stored.
   Later, if diagnostic_commit is called, the message will be
   printed.  Otherwise, if diagnostic_rollback is called, it will be
   as if the diagnostic was never issued.  */

void 
diagnostic_issue_tentatively (buffer)
     output_buffer *buffer;
{
  diagnostic_state *ds;

  /* Create a new diagnostic_state entry.  */
  ds = diagnostic_push_state (buffer);
  /* Save the location of the next diagnostic.  */
  ds->tentative_diagnostic 
    = dyn_string_length (buffer->messages) + 1;
}

/* Commit to those diagnostic messages that were issued tentatively.  */

void
diagnostic_commit (buffer)
     output_buffer *buffer;
{
  /* Copy the diagnostic counts back to the previous entry on the
     stack.  */
  memcpy (buffer->ds->next->diagnostic_count, 
	  buffer->ds->diagnostic_count,
	  sizeof (buffer->ds->diagnostic_count));
  /* Pop the stack.  */
  diagnostic_pop_state (buffer);
  /* If we're popping all the way back to a non-tentative level, and
     there's a new diagnostic, issue the message now.  */
  if (!buffer->ds->tentative_diagnostic
      && dyn_string_length (buffer->messages))
    {
      fputs (buffer->messages->s,
	     output_buffer_attached_stream (buffer));
      output_clear_message_text (buffer);
    }
}

/* Rollback the tentatively issued diagnostic messages.  */

void
diagnostic_rollback (buffer)
     output_buffer *buffer;
{
  /* Delete any diagnostics that we've issued.  */
  dyn_string_terminate (buffer->messages, 
			buffer->ds->tentative_diagnostic - 1);
  /* Free the now-unused state.  */
  diagnostic_pop_state (buffer);
}

/* Begin issuing diagnostics immediately (rather than tentatively)
   until diagnostic_cease_issuing_immediatley is called.  */

void
diagnostic_issue_immediately (buffer)
     output_buffer *buffer;
{
  diagnostic_state *ds;

  /* Create a new entry on the stack.  */
  ds = diagnostic_push_state (buffer);
  /* Save away the old messages.  */
  ds->saved_messages = buffer->messages;
  /* Set up a buffer for storing new messages.  */
  buffer->messages = dyn_string_new (0);
}

/* Stop issuing diagnostics immediately.  */

void
diagnostic_cease_issuing_immediately (buffer)
     output_buffer *buffer;
{
  /* Delete the temporary message buffer.  */
  dyn_string_delete (buffer->messages);
  /* And restore the old one.  */
  buffer->messages = buffer->ds->saved_messages;
  /* Pop this entry from the stack.  */
  diagnostic_pop_state (buffer);
}

/* Count an error or warning.  Return 1 if the message should be printed.  */

int
count_error (warningp)
     int warningp;
{
  if (warningp && !diagnostic_report_warnings_p ())
    return 0;

  if (warningp && !warnings_are_errors)
    warningcount++;
  else
    {
      static int warning_message = 0;

      if (warningp && !warning_message)
	{
	  verbatim ("%s: warnings being treated as errors\n", progname);
	  warning_message = 1;
	}
      errorcount++;
    }

  return 1;
}

/* Print a diagnostic MSGID on FILE.  This is just fprintf, except it
   runs its second argument through gettext.  */

void
fnotice VPARAMS ((FILE *file, const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  FILE *file;
  const char *msgid;
#endif
  va_list ap;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  file = va_arg (ap, FILE *);
  msgid = va_arg (ap, const char *);
#endif

  vfprintf (file, _(msgid), ap);
  va_end (ap);
}


/* Print a fatal I/O error message.  Argument are like printf.
   Also include a system error message based on `errno'.  */

void
fatal_io_error VPARAMS ((const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *msgid;
#endif
  va_list ap;
  output_state os;

  os = output_buffer_state (diagnostic_buffer);
  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  msgid = va_arg (ap, const char *);
#endif

  output_printf (diagnostic_buffer, "%s: %s: ", progname, xstrerror (errno));
  output_buffer_ptr_to_format_args (diagnostic_buffer) = &ap;
  output_buffer_text_cursor (diagnostic_buffer) = _(msgid);
  output_format (diagnostic_buffer);
  diagnostic_finish ((output_buffer *)global_dc);
  output_buffer_state (diagnostic_buffer) = os;
  va_end (ap);
  exit (FATAL_EXIT_CODE);
}

/* Issue a pedantic warning MSGID.  */

void
pedwarn VPARAMS ((const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *msgid;
#endif
  va_list ap;
  diagnostic_context dc;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  msgid = va_arg (ap, const char *);
#endif

  set_diagnostic_context
    (&dc, msgid, &ap, input_filename, lineno, !flag_pedantic_errors);
  report_diagnostic (&dc);
  va_end (ap);
}

/* Issue a pedantic waring about DECL.  */

void
pedwarn_with_decl VPARAMS ((tree decl, const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  tree decl;
  const char *msgid;
#endif
  va_list ap;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  decl = va_arg (ap, tree);
  msgid = va_arg (ap, const char *);
#endif
  /* We don't want -pedantic-errors to cause the compilation to fail from
     "errors" in system header files.  Sometimes fixincludes can't fix what's
     broken (eg: unsigned char bitfields - fixing it may change the alignment
     which will cause programs to mysteriously fail because the C library
     or kernel uses the original layout).  There's no point in issuing a
     warning either, it's just unnecessary noise.  */
  if (!DECL_IN_SYSTEM_HEADER (decl))
    diagnostic_for_decl (decl, msgid, &ap, !flag_pedantic_errors);
  va_end (ap);
}

/* Same as above but within the context FILE and LINE.  */

void
pedwarn_with_file_and_line VPARAMS ((const char *file, int line,
				     const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *file;
  int line;
  const char *msgid;
#endif
  va_list ap;
  diagnostic_context dc;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  file = va_arg (ap, const char *);
  line = va_arg (ap, int);
  msgid = va_arg (ap, const char *);
#endif

  set_diagnostic_context (&dc, msgid, &ap, file, line, !flag_pedantic_errors);
  report_diagnostic (&dc);
  va_end (ap);
}

/* Just apologize with MSGID.  */

void
sorry VPARAMS ((const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *msgid;
#endif
  va_list ap;
  output_state os;

  os = output_buffer_state (diagnostic_buffer);
  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  msgid = va_arg (ap, const char *);
#endif
  ++sorrycount;
  output_set_prefix
    (diagnostic_buffer, context_as_prefix (input_filename, lineno, 0));
  output_printf (diagnostic_buffer, "sorry, not implemented: ");
  output_buffer_ptr_to_format_args (diagnostic_buffer) = &ap;
  output_buffer_text_cursor (diagnostic_buffer) = _(msgid);
  output_format (diagnostic_buffer);
  diagnostic_finish ((output_buffer *)global_dc);
  output_buffer_state (diagnostic_buffer) = os;
  va_end (ap);
}

/* Called when the start of a function definition is parsed,
   this function prints on stderr the name of the function.  */

void
announce_function (decl)
     tree decl;
{
  if (! quiet_flag)
    {
      if (rtl_dump_and_exit)
	verbatim ("%s ", IDENTIFIER_POINTER (DECL_NAME (decl)));
      else
        verbatim (" %s", (*decl_printable_name) (decl, 2));
      fflush (stderr);
      output_needs_newline (diagnostic_buffer) = 1;
      record_last_error_function ();
    }
}

/* The default function to print out name of current function that caused
   an error.  */

void
default_print_error_function (context, file)
     diagnostic_context *context;
     const char *file;
{
  if (error_function_changed ())
    {
      char *prefix = file ? build_message_string ("%s: ", file) : NULL;
      output_state os;

      os = output_buffer_state (context);
      output_set_prefix ((output_buffer *)context, prefix);
      
      if (current_function_decl == NULL)
          output_add_string ((output_buffer *)context, _("At top level:"));
      else
	{
	  if (TREE_CODE (TREE_TYPE (current_function_decl)) == METHOD_TYPE)
            output_printf
              ((output_buffer *)context, "In member function `%s':",
               (*decl_printable_name) (current_function_decl, 2));
	  else
            output_printf
              ((output_buffer *)context, "In function `%s':",
               (*decl_printable_name) (current_function_decl, 2));
	}
      output_add_newline ((output_buffer *)context);

      record_last_error_function ();
      output_buffer_to_stream ((output_buffer *)context);
      output_buffer_state (context) = os;
      free ((char*) prefix);
    }
}

/* Prints out, if necessary, the name of the current function
  that caused an error.  Called from all error and warning functions.
  We ignore the FILE parameter, as it cannot be relied upon.  */

void
report_error_function (file)
  const char *file ATTRIBUTE_UNUSED;
{
  report_problematic_module ((output_buffer *)global_dc);
  (*print_error_function) (global_dc, input_filename);
}

void
error_with_file_and_line VPARAMS ((const char *file, int line,
				   const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *file;
  int line;
  const char *msgid;
#endif
  va_list ap;
  diagnostic_context dc;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  file = va_arg (ap, const char *);
  line = va_arg (ap, int);
  msgid = va_arg (ap, const char *);
#endif

  set_diagnostic_context (&dc, msgid, &ap, file, line, /* warn = */ 0);
  report_diagnostic (&dc);
  va_end (ap);
}

void
error_with_decl VPARAMS ((tree decl, const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  tree decl;
  const char *msgid;
#endif
  va_list ap;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  decl = va_arg (ap, tree);
  msgid = va_arg (ap, const char *);
#endif

  diagnostic_for_decl (decl, msgid, &ap, /* warn = */ 0);
  va_end (ap);
}

void
error_for_asm VPARAMS ((rtx insn, const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  rtx insn;
  const char *msgid;
#endif
  va_list ap;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  insn = va_arg (ap, rtx);
  msgid = va_arg (ap, const char *);
#endif

  diagnostic_for_asm (insn, msgid, &ap, /* warn = */ 0);
  va_end (ap);
}

/* Report an error message.  The arguments are like that of printf.  */

void
error VPARAMS ((const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *msgid;
#endif
  va_list ap;
  diagnostic_context dc;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  msgid = va_arg (ap, const char *);
#endif

  set_diagnostic_context
    (&dc, msgid, &ap, input_filename, lineno, /* warn = */ 0);
  report_diagnostic (&dc);
  va_end (ap);
}

/* Likewise, except that the compilation is terminated after printing the
   error message.  */

void
fatal_error VPARAMS ((const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *msgid;
#endif
  va_list ap;
  diagnostic_context dc;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  msgid = va_arg (ap, const char *);
#endif

  set_diagnostic_context
    (&dc, msgid, &ap, input_filename, lineno, /* warn = */ 0);
  report_diagnostic (&dc);
  va_end (ap);

  fnotice (stderr, "compilation terminated.\n");
  exit (FATAL_EXIT_CODE);
}

/* Report a compiler error at the current line number.  Allow a front end to
   intercept the message.  */

static void (*internal_error_function) PARAMS ((const char *, va_list *));

/* Set the function to call when a compiler error occurs.  */

void
set_internal_error_function (f)
     void (*f) PARAMS ((const char *, va_list *));
{
  internal_error_function = f;
}

void
internal_error VPARAMS ((const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *msgid;
#endif
  va_list ap;
  diagnostic_context dc;

  VA_START (ap, msgid);

  /* If we have been issuing diagnostics tentatively, issue them for
     real now.  They may be illuminating when it comes to diagnosing
     the crash.  */
  while (diagnostic_buffer->ds->next != NULL) 
    diagnostic_commit (diagnostic_buffer);

#ifndef ANSI_PROTOTYPES
  msgid = va_arg (ap, const char *);
#endif

  if (errorcount > 0 || sorrycount > 0)
    {
      fnotice (stderr, "%s:%d: confused by earlier errors, bailing out\n",
	       input_filename, lineno);
      exit (FATAL_EXIT_CODE);
    }

  if (internal_error_function != 0)
    (*internal_error_function) (_(msgid), &ap);
  
  set_diagnostic_context
    (&dc, msgid, &ap, input_filename, lineno, /* warn = */0);
  report_diagnostic (&dc);
  va_end (ap);

  fnotice (stderr,
"Please submit a full bug report,\n\
with preprocessed source if appropriate.\n\
See %s for instructions.\n", GCCBUGURL);
  exit (FATAL_EXIT_CODE);
}

void
_fatal_insn (msgid, insn, file, line, function)
     const char *msgid;
     rtx insn;
     const char *file;
     int line;
     const char *function;
{
  error ("%s", _(msgid));

  /* The above incremented error_count, but isn't an error that we want to
     count, so reset it here.  */
  errorcount--;

  debug_rtx (insn);
  fancy_abort (file, line, function);
}

void
_fatal_insn_not_found (insn, file, line, function)
     rtx insn;
     const char *file;
     int line;
     const char *function;
{
  if (INSN_CODE (insn) < 0)
    _fatal_insn ("Unrecognizable insn:", insn, file, line, function);
  else
    _fatal_insn ("Insn does not satisfy its constraints:",
		insn, file, line, function);
}

void
warning_with_file_and_line VPARAMS ((const char *file, int line,
				     const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *file;
  int line;
  const char *msgid;
#endif
  va_list ap;
  diagnostic_context dc;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  file = va_arg (ap, const char *);
  line = va_arg (ap, int);
  msgid = va_arg (ap, const char *);
#endif

  set_diagnostic_context (&dc, msgid, &ap, file, line, /* warn = */ 1);
  report_diagnostic (&dc);
  va_end (ap);
}

void
warning_with_decl VPARAMS ((tree decl, const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  tree decl;
  const char *msgid;
#endif
  va_list ap;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  decl = va_arg (ap, tree);
  msgid = va_arg (ap, const char *);
#endif

  diagnostic_for_decl (decl, msgid, &ap, /* warn = */ 1);
  va_end (ap);
}

void
warning_for_asm VPARAMS ((rtx insn, const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  rtx insn;
  const char *msgid;
#endif
  va_list ap;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  insn = va_arg (ap, rtx);
  msgid = va_arg (ap, const char *);
#endif

  diagnostic_for_asm (insn, msgid, &ap, /* warn = */ 1);
  va_end (ap);
}

void
warning VPARAMS ((const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *msgid;
#endif
  va_list ap;
  diagnostic_context dc;

  VA_START (ap, msgid);

#ifndef ANSI_PROTOTYPES
  msgid = va_arg (ap, const char *);
#endif

  set_diagnostic_context
    (&dc, msgid, &ap, input_filename, lineno, /* warn = */ 1);
  report_diagnostic (&dc);
  va_end (ap);
}

/* Flush the diagnostic_buffer to the associated stream.  */

static void
diagnostic_finish (buffer)
     output_buffer *buffer;
{
  output_buffer_to_stream (buffer);
  clear_diagnostic_info (buffer);
  fflush (output_buffer_attached_stream (buffer));
}

/* Helper subroutine of output_verbatim and verbatim. Do the approriate
   settings needed by BUFFER for a verbatim formatting.  */

static void
output_do_verbatim (buffer, msgid, args_ptr)
     output_buffer *buffer;
     const char *msgid;
     va_list *args_ptr;
{
  output_state os;

  os = output_buffer_state (buffer);
  output_prefix (buffer) = NULL;
  diagnostic_prefixing_rule (buffer) = DIAGNOSTICS_SHOW_PREFIX_NEVER;
  output_buffer_text_cursor (buffer) = _(msgid);
  output_buffer_ptr_to_format_args (buffer) = args_ptr;
  output_set_maximum_length (buffer, 0);
  output_format (buffer);
  output_buffer_state (buffer) = os;
}

/* Output MESSAGE verbatim into BUFFER.  */

void
output_verbatim VPARAMS ((output_buffer *buffer, const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  output_buffer *buffer;
  const char *msgid;
#endif
  va_list ap;

  VA_START (ap, msgid);
#ifndef ANSI_PROTOTYPES
  buffer = va_arg (ap, output_buffer *);
  msg = va_arg (ap, const char *);
#endif
  output_do_verbatim (buffer, msgid, &ap);
  va_end (ap);
}

/* Same as above but use diagnostic_buffer.  */

void
verbatim VPARAMS ((const char *msgid, ...))
{
#ifndef ANSI_PROTOTYPES
  const char *msgid;
#endif
  va_list ap;

  VA_START (ap, msgid);
#ifndef ANSI_PROTOTYPES
  msgid = va_arg (ap, const char *);
#endif
  output_do_verbatim (diagnostic_buffer, msgid, &ap);
  output_buffer_to_stream (diagnostic_buffer);
  va_end (ap);
}

/* Report a diagnostic message (an error or a warning) as specified by
   DC.  This function is *the* subroutine in terms of which front-ends
   should implement their specific diagnostic handling modules.  The
   front-end independent format specifiers are exactly those described
   in the documentation of output_format.  */

void
report_diagnostic (dc)
     diagnostic_context *dc;
{
  output_state os;

  if (diagnostic_lock++)
    error_recursion ();

  if (count_error (diagnostic_is_warning (dc)))
    {
      os = output_buffer_state (diagnostic_buffer);
      diagnostic_msg = diagnostic_message (dc);
      diagnostic_args = diagnostic_argument_list (dc);
      (*diagnostic_starter (dc)) (diagnostic_buffer, dc);
      output_format (diagnostic_buffer);
      (*diagnostic_finalizer (dc)) (diagnostic_buffer, dc);
      diagnostic_finish ((output_buffer *)global_dc);
      output_buffer_state (diagnostic_buffer) = os;
    }

  diagnostic_lock--;
}

/* Inform the user that an error occurred while trying to report some
   other error.  This indicates catastrophic internal inconsistencies,
   so give up now.  But do try to flush out the previous error.
   This mustn't use internal_error, that will cause infinite recursion.  */

static void
error_recursion ()
{
  if (diagnostic_lock < 3)
    diagnostic_finish ((output_buffer *)global_dc);

  fnotice (stderr,
	   "Internal compiler error: Error reporting routines re-entered.\n");
  fnotice (stderr,
"Please submit a full bug report,\n\
with preprocessed source if appropriate.\n\
See %s for instructions.\n", GCCBUGURL);
  exit (FATAL_EXIT_CODE);
}

/* Given a partial pathname as input, return another pathname that
   shares no directory elements with the pathname of __FILE__.  This
   is used by fancy_abort() to print `Internal compiler error in expr.c'
   instead of `Internal compiler error in ../../GCC/gcc/expr.c'.  */

const char *
trim_filename (name)
     const char *name;
{
  static const char this_file[] = __FILE__;
  const char *p = name, *q = this_file;

  /* First skip any "../" in each filename.  This allows us to give a proper
     reference to a file in a subdirectory.  */
  while (p[0] == '.' && p[1] == '.'
	 && (p[2] == DIR_SEPARATOR
#ifdef DIR_SEPARATOR_2
	     || p[2] == DIR_SEPARATOR_2
#endif
	     ))
    p += 3;

  while (q[0] == '.' && q[1] == '.'
	 && (q[2] == DIR_SEPARATOR
#ifdef DIR_SEPARATOR_2
	     || p[2] == DIR_SEPARATOR_2
#endif
	     ))
    q += 3;

  /* Now skip any parts the two filenames have in common.  */
  while (*p == *q && *p != 0 && *q != 0)
    p++, q++;

  /* Now go backwards until the previous directory separator.  */
  while (p > name && p[-1] != DIR_SEPARATOR
#ifdef DIR_SEPARATOR_2
	 && p[-1] != DIR_SEPARATOR_2
#endif
	 )
    p--;

  return p;
}

/* Report an internal compiler error in a friendly manner and without
   dumping core.  */

void
fancy_abort (file, line, function)
     const char *file;
     int line;
     const char *function;
{
  internal_error ("Internal compiler error in %s, at %s:%d",
		  function, trim_filename (file), line);
}

/* Setup DC for reporting a diagnostic MESSAGE (an error or a WARNING),
   using arguments pointed to by ARGS_PTR, issued at a location specified
   by FILE and LINE.  */

void
set_diagnostic_context (dc, msgid, args_ptr, file, line, warn)
     diagnostic_context *dc;
     const char *msgid;
     va_list *args_ptr;
     const char *file;
     int line;
     int warn;
{
  memset (dc, 0, sizeof (diagnostic_context));
  diagnostic_message (dc) = _(msgid);
  diagnostic_argument_list (dc) = args_ptr;
  diagnostic_file_location (dc) = file;
  diagnostic_line_location (dc) = line;
  diagnostic_is_warning (dc) = warn;
  diagnostic_starter (dc) = diagnostic_starter (global_dc);
  diagnostic_finalizer (dc) = diagnostic_finalizer (global_dc);
}

void
report_problematic_module (buffer)
     output_buffer *buffer;
{
  struct file_stack *p;

  if (output_needs_newline (buffer))
    {
      output_add_newline (buffer);
      output_needs_newline (buffer) = 0;
    }

  if (input_file_stack && input_file_stack->next != 0
      && error_module_changed ())
    {
      for (p = input_file_stack->next; p; p = p->next)
	if (p == input_file_stack->next)
	  output_verbatim
            (buffer, "In file included from %s:%d", p->name, p->line);
	else
	  output_verbatim
            (buffer, ",\n                 from %s:%d", p->name, p->line);
      output_verbatim (buffer, ":\n");
      record_last_error_module ();
    }
}

static void
default_diagnostic_starter (buffer, dc)
     output_buffer *buffer;
     diagnostic_context *dc;
{
  report_error_function (diagnostic_file_location (dc));
  output_set_prefix (buffer,
                     context_as_prefix (diagnostic_file_location (dc),
                                        diagnostic_line_location (dc),
                                        diagnostic_is_warning (dc)));
}

static void
default_diagnostic_finalizer (buffer, dc)
     output_buffer *buffer;
     diagnostic_context *dc __attribute__((__unused__));
{
  output_destroy_prefix (buffer);
}

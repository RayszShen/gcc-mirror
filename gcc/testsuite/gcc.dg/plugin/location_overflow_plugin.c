/* Plugin for testing how gracefully we degrade in the face of very
   large source files.  */

#include "config.h"
#include "gcc-plugin.h"
#include "system.h"
#include "coretypes.h"
#include "spellcheck.h"
#include "diagnostic.h"

int plugin_is_GPL_compatible;

static location_t base_location;

/* Callback handler for the PLUGIN_START_UNIT event; pretend
   we parsed a very large include file.  */

static void
on_start_unit (void */*gcc_data*/, void */*user_data*/)
{
  /* Act as if we've already parsed a large body of code;
     so that we can simulate various fallbacks in libcpp:

     0x50000001 > LINE_MAP_MAX_LOCATION_WITH_PACKED_RANGES:
     this will trigger the creation of line maps with range_bits == 0
     so that all ranges will be stored in the ad-hoc lookaside.

     0x60000001 > LINE_MAP_MAX_LOCATION_WITH_COLS:
     this will trigger the creation of line maps with column_bits == 0
     and hence we will immediately degrade to having locations in which
     column number is 0. */
  line_table->highest_location = base_location;
}

/* We add some extra testing during diagnostics by chaining up
   to the finalizer.  */

static diagnostic_finalizer_fn original_finalizer = NULL;

static void
verify_unpacked_ranges  (diagnostic_context *context,
			 diagnostic_info *diagnostic)
{
  /* Verify that the locations are ad-hoc, not packed. */
  location_t loc = diagnostic_location (diagnostic);
  gcc_assert (IS_ADHOC_LOC (loc));

  /* We're done testing; chain up to original finalizer.  */
  gcc_assert (original_finalizer);
  original_finalizer (context, diagnostic);
}

static void
verify_no_columns  (diagnostic_context *context,
		    diagnostic_info *diagnostic)
{
  /* Verify that the locations have no columns. */
  location_t loc = diagnostic_location (diagnostic);
  gcc_assert (LOCATION_COLUMN (loc) == 0);

  /* We're done testing; chain up to original finalizer.  */
  gcc_assert (original_finalizer);
  original_finalizer (context, diagnostic);
}

int
plugin_init (struct plugin_name_args *plugin_info,
	     struct plugin_gcc_version */*version*/)
{
  /* Read VALUE from -fplugin-arg-location_overflow_plugin-value=<VALUE>
     in hexadecimal form into base_location.  */
  for (int i = 0; i < plugin_info->argc; i++)
    {
      if (0 == strcmp (plugin_info->argv[i].key, "value"))
	base_location = strtol (plugin_info->argv[i].value, NULL, 16);
    }

  if (!base_location)
    error_at (UNKNOWN_LOCATION, "missing plugin argument");

  register_callback (plugin_info->base_name,
		     PLUGIN_START_UNIT,
		     on_start_unit,
		     NULL); /* void *user_data */

  /* Hack in additional testing, based on the exact value supplied.  */
  original_finalizer = diagnostic_finalizer (global_dc);
  switch (base_location)
    {
    case LINE_MAP_MAX_LOCATION_WITH_PACKED_RANGES + 1:
      diagnostic_finalizer (global_dc) = verify_unpacked_ranges;
      break;

    case LINE_MAP_MAX_LOCATION_WITH_COLS + 1:
      diagnostic_finalizer (global_dc) = verify_no_columns;
      break;

    default:
      error_at (UNKNOWN_LOCATION, "unrecognized value for plugin argument");
    }

  return 0;
}

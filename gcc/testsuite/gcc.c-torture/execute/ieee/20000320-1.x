if {[istarget "m68k-*-*"] && [check_effective_target_coldfire_fpu]} {
    # ColdFire FPUs require software handling of subnormals.  We are
    # not aware of any system that has this.
    set torture_execute_xfail "m68k-*-*"
}

return 0

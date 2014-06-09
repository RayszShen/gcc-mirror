if [istarget "spu-*-*"] {
    # No Inf/NaN support on SPU.
    return 1
}
if [istarget "powerpc-ibm-aix*"] {
    # z'7f7fffff' value not preserved by lfs instruction.
    return 1
}
add-ieee-options
return 0

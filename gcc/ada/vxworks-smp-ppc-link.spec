*lib:
+ %{mrtp:%{!shared: \
     -L%:getenv(WIND_BASE /target/lib_smp/usr/lib/ppc/PPC32/common) \
   }}

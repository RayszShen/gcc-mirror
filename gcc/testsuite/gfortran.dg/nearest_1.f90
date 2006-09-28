! { dg-do run }
! PR fortran/27021
! Original code submitted by Dominique d'Humieres
! Converted to Dejagnu for the testsuite by Steven G. Kargl
program chop
  real o, t, td, tu, x, y
  o = 1.
  t = tiny(o)
  td = nearest(t,-1.0)
  x = td/2.0
  y = nearest(tiny(o),-1.0)/2.0
  if (abs(x - y) /= 0.) call abort
end program chop


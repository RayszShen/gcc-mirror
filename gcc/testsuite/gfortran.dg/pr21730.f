! PR fortran/21730
! { dg-do run }
      character*2 a
      character*4 b
      character*6 c
      parameter (a="12")
      parameter (b = a)
      write (c,'("#",A,"#")') b
      if (c .ne. '#12  #') call abort
      end


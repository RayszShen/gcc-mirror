// { dg-do preprocess }
// { dg-options "-std=gnu99 -fdiagnostics-show-option -Wtraditional -Werror=long-long" }
/* { dg-message "some warnings being treated as errors" "" {target "*-*-*"} 0 } */
#if 0LL  // { dg-error "traditional C rejects the \"LL\" suffix .-Wlong-long." }
         // { dg-error "use of C99 long long integer constant .-Wlong-long." "use long long" { target *-*-* } 4 }
#endif

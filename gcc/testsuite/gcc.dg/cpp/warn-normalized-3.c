// { dg-do preprocess }
// { dg-options "-std=gnu99 -fdiagnostics-show-option -fextended-identifiers -Werror=normalized=" }
/* { dg-message "some warnings being treated as errors" "" {target "*-*-*"} 0 } */
        // { dg-prune-output ".*-Werror=normalized=: Set -Wnormalized=nfc.*" }
\u0F43  // { dg-error "`.U00000f43' is not in NFC .-Wnormalized=." }

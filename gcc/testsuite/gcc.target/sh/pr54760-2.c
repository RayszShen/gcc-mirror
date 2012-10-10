/* Check that thread pointer relative memory accesses are converted to
   gbr displacement address modes.  If we see a gbr register store
   instruction something is not working properly.  */
/* { dg-do compile { target "sh*-*-*" } } */
/* { dg-options "-O1" } */
/* { dg-skip-if "" { "sh*-*-*" } { "-m5*"} { "" } }  */
/* { dg-final { scan-assembler-times "stc\tgbr" 0 } } */

/* ---------------------------------------------------------------------------
  Simple GBR load.
*/
#define func(name, type, disp)\
  int \
  name ## _tp_load (void) \
  { \
    type* tp = (type*)__builtin_thread_pointer (); \
    return tp[disp]; \
  }

func (test00, int, 0)
func (test01, int, 5)
func (test02, int, 255)

func (test03, short, 0)
func (test04, short, 5)
func (test05, short, 255)

func (test06, char, 0)
func (test07, char, 5)
func (test08, char, 255)

func (test09, unsigned int, 0)
func (test10, unsigned int, 5)
func (test11, unsigned int, 255)

func (test12, unsigned short, 0)
func (test13, unsigned short, 5)
func (test14, unsigned short, 255)

func (test15, unsigned char, 0)
func (test16, unsigned char, 5)
func (test17, unsigned char, 255)

#undef func

/* ---------------------------------------------------------------------------
  Simple GBR store.
*/
#define func(name, type, disp)\
  void \
  name ## _tp_store (int a) \
  { \
    type* tp = (type*)__builtin_thread_pointer (); \
    tp[disp] = (type)a; \
  }

func (test00, int, 0)
func (test01, int, 5)
func (test02, int, 255)

func (test03, short, 0)
func (test04, short, 5)
func (test05, short, 255)

func (test06, char, 0)
func (test07, char, 5)
func (test08, char, 255)

func (test09, unsigned int, 0)
func (test10, unsigned int, 5)
func (test11, unsigned int, 255)

func (test12, unsigned short, 0)
func (test13, unsigned short, 5)
func (test14, unsigned short, 255)

func (test15, unsigned char, 0)
func (test16, unsigned char, 5)
func (test17, unsigned char, 255)

#undef func

/* ---------------------------------------------------------------------------
  Arithmetic on the result of a GBR load.
*/
#define func(name, type, disp, op, opname)\
  int \
  name ## _tp_load_arith_ ##opname (int a) \
  { \
    type* tp = (type*)__builtin_thread_pointer (); \
    return tp[disp] op a; \
  }

#define funcs(op, opname) \
  func (test00, int, 0, op, opname) \
  func (test01, int, 5, op, opname) \
  func (test02, int, 255, op, opname) \
  func (test03, short, 0, op, opname) \
  func (test04, short, 5, op, opname) \
  func (test05, short, 255, op, opname) \
  func (test06, char, 0, op, opname) \
  func (test07, char, 5, op, opname) \
  func (test08, char, 255, op, opname) \
  func (test09, unsigned int, 0, op, opname) \
  func (test10, unsigned int, 5, op, opname) \
  func (test11, unsigned int, 255, op, opname) \
  func (test12, unsigned short, 0, op, opname) \
  func (test13, unsigned short, 5, op, opname) \
  func (test14, unsigned short, 255, op, opname) \
  func (test15, unsigned char, 0, op, opname) \
  func (test16, unsigned char, 5, op, opname) \
  func (test17, unsigned char, 255, op, opname) \

funcs (+, plus)
funcs (-, minus)
funcs (*, mul)
funcs (&, and)
funcs (|, or)
funcs (^, xor)

#undef funcs
#undef func

/* ---------------------------------------------------------------------------
  Arithmetic of the result of two GBR loads.
*/
#define func(name, type, disp0, disp1, op, opname)\
  int \
  name ## _tp_load_load_arith_ ##opname (void) \
  { \
    type* tp = (type*)__builtin_thread_pointer (); \
    return tp[disp0] op tp[disp1]; \
  }

#define funcs(op, opname) \
  func (test00, int, 0, 5, op, opname) \
  func (test02, int, 1, 255, op, opname) \
  func (test03, short, 0, 5, op, opname) \
  func (test05, short, 1, 255, op, opname) \
  func (test06, char, 0, 5, op, opname) \
  func (test08, char, 1, 255, op, opname) \
  func (test09, unsigned int, 0, 5, op, opname) \
  func (test11, unsigned int, 1, 255, op, opname) \
  func (test12, unsigned short, 0, 5, op, opname) \
  func (test14, unsigned short, 1, 255, op, opname) \
  func (test15, unsigned char, 0, 5, op, opname) \
  func (test17, unsigned char, 1, 255, op, opname) \

funcs (+, plus)
funcs (-, minus)
funcs (*, mul)
funcs (&, and)
funcs (|, or)
funcs (^, xor)

#undef funcs
#undef func

/* ---------------------------------------------------------------------------
  GBR load GBR store copy.
*/

#define func(name, type, disp0, disp1)\
  void \
  name ## _tp_copy (void) \
  { \
    type* tp = (type*)__builtin_thread_pointer (); \
    tp[disp0] = tp[disp1]; \
  }

func (test00, int, 0, 5)
func (test02, int, 1, 255)
func (test03, short, 0, 5)
func (test05, short, 1, 255)
func (test06, char, 0, 5)
func (test08, char, 1, 255)
func (test09, unsigned int, 0, 5)
func (test11, unsigned int, 1, 255)
func (test12, unsigned short, 0, 5)
func (test14, unsigned short, 1, 255)
func (test15, unsigned char, 0, 5)
func (test17, unsigned char, 1, 255)

#undef func

/* ---------------------------------------------------------------------------
  GBR load, arithmetic, GBR store
*/

#define func(name, type, disp, op, opname)\
  void \
  name ## _tp_load_arith_store_ ##opname (int a) \
  { \
    type* tp = (type*)__builtin_thread_pointer (); \
    tp[disp] op a; \
  }

#define funcs(op, opname) \
  func (test00, int, 0, op, opname) \
  func (test01, int, 5, op, opname) \
  func (test02, int, 255, op, opname) \
  func (test03, short, 0, op, opname) \
  func (test04, short, 5, op, opname) \
  func (test05, short, 255, op, opname) \
  func (test06, char, 0, op, opname) \
  func (test07, char, 5, op, opname) \
  func (test08, char, 255, op, opname) \
  func (test09, unsigned int, 0, op, opname) \
  func (test10, unsigned int, 5, op, opname) \
  func (test11, unsigned int, 255, op, opname) \
  func (test12, unsigned short, 0, op, opname) \
  func (test13, unsigned short, 5, op, opname) \
  func (test14, unsigned short, 255, op, opname) \
  func (test15, unsigned char, 0, op, opname) \
  func (test16, unsigned char, 5, op, opname) \
  func (test17, unsigned char, 255, op, opname) \

funcs (+=, plus)
funcs (-=, minus)
funcs (*=, mul)
funcs (&=, and)
funcs (|=, or)
funcs (^=, xor)

// 1999-08-11 bkoz

// Copyright (C) 1999, 2000 Free Software Foundation
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// 27.6.1.3 unformatted input functions

#include <istream>
#include <sstream>
#include <fstream>
#ifdef DEBUG_ASSERT
  #include <assert.h>
#endif

bool test01() {

  typedef std::ios::traits_type traits_type;

  bool test = true;
  const std::string str_01;
  const std::string str_02("soul eyes: john coltrane quartet");
  std::string strtmp;

  std::stringbuf isbuf_03(str_02, std::ios_base::in);
  std::stringbuf isbuf_04(str_02, std::ios_base::in);

  std::istream is_00(NULL);
  std::istream is_03(&isbuf_03);
  std::istream is_04(&isbuf_04);
  std::ios_base::iostate state1, state2, statefail, stateeof;
  statefail = std::ios_base::failbit;
  stateeof = std::ios_base::eofbit;

  // istream& read(char_type* s, streamsize n)
  char carray[60] = "";
  state1 = is_04.rdstate();
  is_04.read(carray, 0);
  state2 = is_04.rdstate();
  test &= state1 == state2;

  state1 = is_04.rdstate();
  is_04.read(carray, 9);
  state2 = is_04.rdstate();
  test &= state1 == state2;
  test &= !strncmp(carray, "soul eyes", 9);
  test &= is_04.peek() == ':';

  state1 = is_03.rdstate();
  is_03.read(carray, 60);
  state2 = is_03.rdstate();
  test &= state1 != state2;
  test &= static_cast<bool>(state2 & stateeof); 
  test &= static_cast<bool>(state2 & statefail); 
  test &= !strncmp(carray, "soul eyes: john coltrane quartet", 35);


  // istream& ignore(streamsize n = 1, int_type delim = traits::eof())
  state1 = is_04.rdstate();
  is_04.ignore();
  test &= is_04.gcount() == 1;
  state2 = is_04.rdstate();
  test &= state1 == state2;
  test &= is_04.peek() == ' ';

  state1 = is_04.rdstate();
  is_04.ignore(0);
  test &= is_04.gcount() == 0;
  state2 = is_04.rdstate();
  test &= state1 == state2;
  test &= is_04.peek() == ' ';

  state1 = is_04.rdstate();
  is_04.ignore(5, traits_type::to_int_type(' '));
  test &= is_04.gcount() == 1;
  state2 = is_04.rdstate();
  test &= state1 == state2;
  test &= is_04.peek() == 'j';

  // int_type peek()
  state1 = is_04.rdstate();
  test &= is_04.peek() == 'j';
  test &= is_04.gcount() == 0;
  state2 = is_04.rdstate();
  test &= state1 == state2;

  is_04.ignore(30);
  state1 = is_04.rdstate();
  test &= is_04.peek() == traits_type::eof();
  test &= is_04.gcount() == 0;
  state2 = is_04.rdstate();
  test &= state1 == state2;


  // istream& putback(char c)
  is_04.clear();
  state1 = is_04.rdstate();
  is_04.putback('|');
  test &= is_04.gcount() == 0;
  state2 = is_04.rdstate();
  test &= state1 == state2;
  test &= is_04.peek() == '|';

  // istream& unget()
  is_04.clear();
  state1 = is_04.rdstate();
  is_04.unget();
  test &= is_04.gcount() == 0;
  state2 = is_04.rdstate();
  test &= state1 == state2;
  test &= is_04.peek() == 'e';
  
  // int sync()
  int i = is_00.sync();

#ifdef DEBUG_ASSERT
  assert(test);
#endif
 
  return test;
}

bool test02(void)
{
  typedef std::char_traits<char>	traits_type;

  bool test = true;
  const char str_lit01[] = "			    sun*ra 
                            and his myth science arkestra present
                            angles and demons @ play
                            the nubians of plutonia";
  std::string str01(str_lit01);
  std::string strtmp;

  std::stringbuf sbuf_04(str01, std::ios_base::in);

  std::istream is_00(NULL);
  std::istream is_04(&sbuf_04);
  std::ios_base::iostate state1, state2, statefail, stateeof;
  statefail = std::ios_base::failbit;
  stateeof = std::ios_base::eofbit;
  std::streamsize count1, count2;
  char carray1[400] = "";

  // istream& getline(char* s, streamsize n, char delim)
  // istream& getline(char* s, streamsize n)
  state1 = is_00.rdstate();
  is_00.getline(carray1, 20, '*');
  state2 = is_00.rdstate();  
  test &= is_04.gcount() == 0;
  test &= state1 != state2;
  test &= bool(state2 & statefail);

  state1 = is_04.rdstate();
  is_04.getline(carray1, 1, '\t'); // extracts, throws away
  state2 = is_04.rdstate();  
  test &= is_04.gcount() == 1;
  test &= state1 == state2;
  test &= state1 == 0;
  test &= !traits_type::compare("", carray1, 1);

  state1 = is_04.rdstate();
  is_04.getline(carray1, 20, '*');
  state2 = is_04.rdstate();  
  test &= is_04.gcount() == 10;
  test &= state1 == state2;
  test &= state1 == 0;
  test &= !traits_type::compare("\t\t    sun", carray1, 10);

  state1 = is_04.rdstate();
  is_04.getline(carray1, 20);
  state2 = is_04.rdstate();  
  test &= is_04.gcount() == 4;
  test &= state1 == state2;
  test &= state1 == 0;
  test &= !traits_type::compare("ra ", carray1, 4);

  state1 = is_04.rdstate();
  is_04.getline(carray1, 65);
  state2 = is_04.rdstate();  
  test &= is_04.gcount() == 64;
  test &= state1 != state2;
  test &= state2 == statefail;
  test &= !traits_type::compare("                            and his myth science arkestra presen", carray1, 65);

  is_04.clear();
  state1 = is_04.rdstate();
  is_04.getline(carray1, 120, '|');
  state2 = is_04.rdstate();  
  test &= is_04.gcount() == 106;
  test &= state1 != state2;
  test &= state2 == stateeof;

  is_04.clear();
  state1 = is_04.rdstate();
  is_04.getline(carray1, 100, '|');
  state2 = is_04.rdstate();  
  test &= is_04.gcount() == 0; 
  test &= state1 != state2;
  test &= static_cast<bool>(state2 & stateeof);
  test &= static_cast<bool>(state2 & statefail);

#ifdef DEBUG_ASSERT
  assert(test);
#endif
 
  return test;
}

bool test03(void)
{
  typedef std::char_traits<char>	traits_type;

  bool test = true;
  const char str_lit01[] = "   sun*ra 
			   & his arkestra, featuring john gilmore: 
                         jazz in silhouette: images and forecasts of tomorrow";
  std::string str01(str_lit01);
  std::string strtmp;

  std::stringbuf sbuf_03;
  std::stringbuf sbuf_04(str01, std::ios_base::in);
  std::stringbuf sbuf_05(str01, std::ios_base::in);

  std::istream is_00(NULL);
  std::istream is_04(&sbuf_04);
  std::istream is_05(&sbuf_05);
  std::ios_base::iostate state1, state2, statefail, stateeof;
  statefail = std::ios_base::failbit;
  stateeof = std::ios_base::eofbit;
  std::streamsize count1, count2;
  char carray1[400] = "";

  // int_type get()
  // istream& get(char*, streamsize, char delim)
  // istream& get(char*, streamsize)
  // istream& get(streambuf&, char delim)
  // istream& get(streambuf&)
  is_00.get(carray1, 2);
  test &= static_cast<bool>(is_00.rdstate() & statefail); 
  test &= is_00.gcount() == 0;

  is_04.get(carray1, 4);
  test &= !(is_04.rdstate() & statefail);
  test &= !traits_type::compare(carray1, "   ", 4);
  test &= is_04.gcount() == 3;

  is_04.clear();
  is_04.get(carray1 + 3, 200);
  test &= !(is_04.rdstate() & statefail);
  test &= !(is_04.rdstate() & stateeof);
  test &= !traits_type::compare(carray1, str_lit01, 10);
  test &= is_04.gcount() == 7;

  is_04.clear();
  is_04.get(carray1, 200);
  test &= !(is_04.rdstate() & stateeof);
  test &= static_cast<bool>(is_04.rdstate() & statefail); // delimiter
  test &= is_04.gcount() == 0;
  is_04.clear();
  is_04.get(carray1, 200, '[');
  test &= static_cast<bool>(is_04.rdstate() & stateeof);
  test &= !(is_04.rdstate() & statefail);
  test &= is_04.gcount() == 125;
  is_04.clear();  
  is_04.get(carray1, 200);
  test &= static_cast<bool>(is_04.rdstate() & stateeof);
  test &= static_cast<bool>(is_04.rdstate() & statefail); 
  test &= is_04.gcount() == 0;

  std::stringbuf sbuf_02(std::ios_base::in);
  is_05.clear();
  is_05.get(sbuf_02);
  test &= is_05.gcount() == 0;
  test &= static_cast<bool>(is_05.rdstate() & statefail); 
  test &= !(is_05.rdstate() & stateeof); 

  is_05.clear();
  is_05.get(sbuf_03);
  test &= is_05.gcount() == 10;
  test &= sbuf_03.str() == "   sun*ra ";
  test &= !(is_05.rdstate() & statefail); 
  test &= !(is_05.rdstate() & stateeof); 

  is_05.clear();
  is_05.get(sbuf_03, '|');
  test &= is_05.gcount() == 125;
  test &= sbuf_03.str() == str_lit01;
  test &= !(is_05.rdstate() & statefail); 
  test &= static_cast<bool>(is_05.rdstate() & stateeof); 

  is_05.clear();
  is_05.get(sbuf_03, '|');
  test &= is_05.gcount() == 0;
  test &= static_cast<bool>(is_05.rdstate() & stateeof); 
  test &= static_cast<bool>(is_05.rdstate() & statefail); 

#ifdef DEBUG_ASSERT
  assert(test);
#endif
 
  return test;
}

// http://sourceware.cygnus.com/ml/libstdc++/2000-q1/msg00177.html
void test06()
{
  bool test = true;

  const std::string str_00("Red_Garland_Qunitet-Soul_Junction");
  std::string strtmp;
  char c_array[str_00.size() + 4];

  std::stringbuf isbuf_00(str_00, std::ios_base::in);
  std::istream is_00(&isbuf_00);
  std::ios_base::iostate state1, state2, statefail, stateeof;
  statefail = std::ios_base::failbit;
  stateeof = std::ios_base::eofbit;

  state1 = stateeof | statefail;
  test &= is_00.gcount() == 0;
  is_00.read(c_array, str_00.size() + 1);
  test &= is_00.gcount() == str_00.size();
  test &= is_00.rdstate() == state1;

  is_00.read(c_array, str_00.size());
  test &= is_00.rdstate() == state1;

#ifdef DEBUG_ASSERT
  assert(test);
#endif
}

int main()
{
  test01();
  test02();
  test03();
  test06();
  return 0;
}









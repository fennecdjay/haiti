enum box_type
{
  BOX_NONE,
  BOX_TODO,
  BOX_SKIP,
  BOX_TEST,
  BOX_DONE
};
typedef unsigned int  uint;
typedef unsigned char bool;
typedef char*         cstr;
typedef cstr*         pstr;

extern uint n_dirs;
extern uint n_file;
extern uint n_tags;
extern pstr dirs;
extern pstr file;
extern pstr tags;


extern bool find(cstr str, pstr src, uint n);
extern const cstr file_ext(cstr s);
extern void haiti_print_file();
extern void haiti_print_tags();

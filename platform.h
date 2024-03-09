
__attribute__((noreturn)) void panic(const char *dbg_msg);
void debug(const char *dbg_msg);
void debug_putch(char ch);

#define CHECK(cond) do { if (!(cond)) panic(#cond); } while(0)


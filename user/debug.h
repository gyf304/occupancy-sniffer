#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef DEBUG
#define dbg_printf(...) os_printf(__VA_ARGS__)
#endif

#ifndef DEBUG
#define dbg_printf(...) while(0){}
#endif

#endif
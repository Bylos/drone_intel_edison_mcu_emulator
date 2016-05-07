#ifndef __DEBUG_H__
#define __DEBUG_H__

#define EMPTY		""
#define INFO		"[INFO :]"
#define MODE		"[MODE :]"
#define WARNING		"[WARN :]"
#define ERROR		"[ERROR:]"

void debug_print(float time, const char* level, const char* message);

#endif /* __DEBUG_H__ */

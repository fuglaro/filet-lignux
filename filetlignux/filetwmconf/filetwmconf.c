/* filetwmconf.c: Source config file for filetwm's config plugin.
Build and install config with:
cc -shared -fPIC filetwmconf.c -o ~/.config/filetwmconf.so
*/
#include <unistd.h>
#define S(T, N, V) extern T N; N = V;
#define P(T, N, ...) extern T* N;static T _##N[]=__VA_ARGS__;N=_##N

void config(void) {
	S(char*, font, "monospane:bold:size=5");
	P(char*, launcher,
		{ "launcher", "monospace:size=5", "#dddddd", "#111111", "#335577", NULL });
}

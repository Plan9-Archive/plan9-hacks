#include <u.h>
#include <libc.h>

u32int
gcd_iter(u32int u, u32int v) {
  u32int t;
  while (v) {
    t = u; 
    u = v; 
    v = t % v;
  }
  return u < 0 ? -u : u; /* abs(u) */
}

void
main(int argc, char **argv)
{
	u32int u = 15;
	u32int v = 40;
	u32int r = gcd_iter(15, 40);
	print("%ud\n", r);
}

#include <u.h>
#include <libc.h>

void
main()
{

	const double R=44100; // sample rate (samples per second)
	const double C=261.625565; // frequency of middle-C (hertz)
	const double F=R/256; // bytebeat frequency of 1*t due to 8-bit truncation (hertz)
	const double V=127; // a volume constant

	int t;
	for(t=0; ; t++ ){
		char temp = (sin(t*2*PI/R*C)+1)*V; // pure middle C sine wave
		// uint8_t temp = t/F*C; // middle C saw wave (bytebeat style)
		// uint8_t temp = (t*5&t>>7)|(t*3&t>>10); // viznut bytebeat composition
		write(1, &temp, 1);
	}
}



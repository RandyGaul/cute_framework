#include <cute.h>
using namespace Cute;

#define echo(...) echo_impl(String("") + __VA_ARGS__)

void echo_impl(String s)
{
	printf("%s\n", s.c_str());
}

int main(int argc, char* argv[])
{
	echo(1);
	echo(10.0f);
	v2 v = V2(0,0);
	echo("The vector is: " + v.x + ", " + v.y);
	return 0;
}

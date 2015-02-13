#include <stdio.h>
#include <Windows.h>
#include <vector>
#include <string>

using namespace std;

int main(int argc, char **argv)
{
	for (int i = 1; ; i += 1) {
		printf("Loop %d\n", i);
		//int rtn = system("C:/Ht/tests/msvs12/x64/Debug/ioblk.exe");
		int rtn = system("C:/Ht/tests/msvs12/x64/Release/ioblk.exe");

		if (rtn > 0)
			exit(0);
	}
}

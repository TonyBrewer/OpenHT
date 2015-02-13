#include "Ht.h"
using namespace Ht;

int main(int argc, char **argv)
{
	CHtHif *pHtHif = new CHtHif();

	delete pHtHif;

	printf("PASSED\n");
}

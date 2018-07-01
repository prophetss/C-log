#include "log.h"

int main()
{
	int i;
	for (i = 1; i < 100; i++)
	{
		write_log(LOG_RUN, "Welcome - The Apache Portable Runtime Project");
	}
}
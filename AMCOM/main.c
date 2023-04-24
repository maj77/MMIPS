#include <stdio.h>
#include "amcom_unit_test.h"
//#include <stdint.h>

/*
void func(const void* data){
    printf("data = %d", ((uint8_t*)data)[1]);
}
*/

int main()
{
    printf("Running AMCOM unit test...\n");
    if (true == AMCOM_UnitTest()) {
        printf("---------------------------\n");
        printf("      !!! SUCCESS !!!\n");
        printf("---------------------------\n");
		return 0;
	}
	
    printf("---------------------------\n");
    printf("        TEST FAILED\n");
    printf("---------------------------\n");
	return 1;
	/*
	uint8_t arr[]= {2,4,5,7,8,9};
	func(arr);
	*/
}
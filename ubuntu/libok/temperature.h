#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


float getTempData()
{

float max = 32767;
srand( (unsigned)time(NULL) );

float fTemp =(rand()%50+250)*0.1f;
printf("iTemp:%f\n",fTemp);

return fTemp;

}


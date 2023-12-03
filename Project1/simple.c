//Matthew Voggel Program, for Project 1 - Q2.3
//function for array of 10 numbers, calculate avg of sq. roots, and prints
#include<math.h>
#include<stdio.h>
int main()
{
  int numArray[10], size = 10;
  double sum=0.0,avg;
  printf("\nEnter 10 array elements for calculations:");
  for (int i =0; i < size; ++i)
    {
      scanf("%d",&numArray[i]);
    }
  printf("\nArray elements:");
  for(int i = 0; i < size; ++i)
    {
      printf("%d " ,&numArray[i]);
    }
  for (int i = 0; i < size; ++i)
    {
      sum+= sqrt(numArray[i]);
    }  
  avg=sum/10.0;
  printf("\nThe average =%lf",avg);
  printf("\n");
  return 0; 
} 


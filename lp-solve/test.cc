#include "LP.h"
#include <stdio.h>
#include <iostream>

int main(){
	FILE * f = fopen("MR.csv","r");
  std::vector< std::vector <std::pair<unsigned,float> > > mrc(1);
	unsigned size;
	float mr;
  while(fscanf(f,"%u\t%f\n",&size,&mr)!=EOF){
	  mrc[0].push_back(std::pair<unsigned,float>(size,mr));
	}

	float lat;

	unsigned L1_size, L2_size;

	std::cin >> lat ;

	gen_solve_lp(mrc,L1_size,L2_size,lat);

	printf("L1 size: %u\t L2 size: %u\n",L1_size,L2_size);
	
}

#include "LP.h"
#include <iostream>

float lat_th = 1.01;

char * get_char_str(const std::string& str){
  char * writable = new char[str.size() + 1];
  std::copy(str.begin(), str.end(), writable);
  writable[str.size()] = '\0';
	return writable;
}

int gen_solve_lp (std::vector< std::vector <std::pair<unsigned,float> > > &mrc, unsigned &L1_size, unsigned &L2_size, float lat = 0){
  std::vector<float> min_lats;

	lprec *lp;
	int Ncol, *colno = NULL, j, ret = 0;
	REAL *row = NULL;
	Ncol = mrc.size()*6; /* for each memcached, mr, size, latency & total latency, total cost */

	lp = make_lp(0, Ncol);

	if(lp == NULL)
		ret = 1; /* Couldn't construct a new model */

	if(ret==0) {
		std::string L1mr("mr.L1");
		std::string L2mr("mr.L2");
		std::string L1size("size.L1");
		std::string L2size("size.L2i");
		std::string lat("lat");
		std::string cost("cost");

		for(int i=0; i< mrc.size(); ++i){
			std::string i_str = "."+std::to_string(i);
			std::string L1size_i_str = L1size+i_str;
			std::string L2size_i_str = L2size+i_str;
			std::string L1mr_i_str = L1mr+i_str;
			std::string L2mr_i_str = L2mr+i_str;
			std::string lat_i_str = lat+i_str;
			std::string cost_i_str = cost+i_str;

	    set_col_name(lp, 6*i+1, get_char_str(L1size_i_str));
	    set_col_name(lp, 6*i+2, get_char_str(L2size_i_str));
	    set_col_name(lp, 6*i+3, get_char_str(L1mr_i_str));
	    set_col_name(lp, 6*i+4, get_char_str(L2mr_i_str));
			set_col_name(lp, 6*i+5, get_char_str(lat_i_str));
			set_col_name(lp, 6*i+6, get_char_str(cost_i_str));
	  }

		/*create space large enough for one row */

		colno = (int *) malloc(Ncol * sizeof(*colno));
		row = (REAL *) malloc(Ncol * sizeof(*row));

		if((colno==NULL) || (row == NULL))
			ret = 2;

	}

	//std::cerr << ret << "\tcreated the model\n";

	if(ret == 0){
	  set_add_rowmode(lp, TRUE); /*makes building the model faster if it is done rows by row*/

		for(int i=0; i < mrc.size(); ++i){
		  for(int j=0 ; j < mrc[i].size() -1 ; ++j ){
				if(ret == 0){
				  auto p1 = mrc[i][j];
				  auto p2 = mrc[i][j+1];
					//std::cerr << "(" << p1.first << "," << p1.second << ")\t("<< p2.first << "," << p2.second <<")\n";
					{ 
			      int k = 0;
				    colno[k] = 6*i+1;
				    row[k++] = p1.second - p2.second;

				    colno[k] = 6*i+3;
				    row[k++] = p2.first - p1.first;

				    /* add the row to lpsolve */
				    if(!add_constraintex(lp, k, row, colno, GE, p1.second * p2.first - p1.first * p2.second))
					    ret = 3;
					}

					{ 
			      int k = 0;
 
						colno[k] = 6*i+2;
						row[k++] = p1.second - p2.second;

				    colno[k] = 6*i+4;
				    row[k++] = p2.first - p1.first;

				    /* add the row to lpsolve */
				    if(!add_constraintex(lp, k, row, colno, GE, p1.second * p2.first - p1.first * p2.second))
					    ret = 3;
					}



			  }
			}

			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+1;
				row[k++] = 1;

				if(!add_constraintex(lp, k, row, colno, GE, mrc[i].front().first))
						ret = 3;
			}

			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+2;
				row[k++] = 1;

				if(!add_constraintex(lp, k, row, colno, GE, mrc[i].front().first))
						ret = 3;
			}

			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+1;
				row[k++] = 1;

				if(!add_constraintex(lp, k, row, colno, LE, mrc[i].back().first))
						ret = 3;
			}

			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+2;
				row[k++] = 1;

				if(!add_constraintex(lp, k, row, colno, LE, mrc[i].back().first))
						ret = 3;
			}


			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+1;
				row[k++] = -1;

				colno[k] = 6*i+2;
				row[k++] = 1;
				
				if(!add_constraintex(lp, k, row, colno, GE, 0))
						ret = 3;
			}



			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+3; /* mr.L1 */
				row[k++] = 0.2 - 0.1; /* KVD lat - DRAM lat */

				colno[k] = 6*i+4; /* mr.L2 */
				row[k++] = 10-0.2; /*backend lat - KVD lat */

				colno[k] = 6*i+5; /* lat */
				row[k++] = -1;

				if(!add_constraintex(lp, k, row, colno, EQ, -0.1))
						ret = 3;
			}

      //min_lats.push_back(lat_th*(mrc[i].back().second*10 + (1-mrc[i].back().second)*0.1));
      //min_lats.push_back(0.75);
			min_lats.push_back(lat);

			/* especial for our workload latency <= 0.67 */

			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+5;
				row[k++] = 1;


				if(!add_constraintex(lp, k, row, colno, LE, min_lats[i]))
					ret = 3;
			}

		  if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+6;
				row[k++] = -1;

				colno[k] = 6*i+1;
				row[k++] = (10.0-0.68) / 1024;

				colno[k] = 6*i+2;
				row[k++] = 0.68 / 1024;

				if(!add_constraintex(lp, k, row, colno, EQ, 0))
					ret = 3;
			}

		}
	}
	
	//std::cerr << ret << "\tadded MR constraints\n";

	//if(ret == 0){

	//  /*total DRAM size constraint */
	//	int k = 0;
	//	for(int i=0; i<total_memcached_count; ++i){
	//	  colno[k] = 6*i+1;
	//		row[k++] = 1;
	//	}

	//	if(!add_constraintex(lp, k, row, colno, LE, 2*1024))
	//		ret = 3;

	//	/*total KVD size constraint */
	//	k = 0;
	//	for(int i=0; i<total_memcached_count; ++i){
	//	  colno[k] = 6*i+2;
	//		row[k++] = 1;
	//	}

	//	if(!add_constraintex(lp, k, row, colno, LE, 20*1024))
	//		ret = 3;

	//}

	//std::cerr << ret << "\tadded DRAM constraints\n";

	if(ret == 0){

		set_add_rowmode(lp, FALSE); /*rowmode should be turned off again when done building the model */

		/* for now, set the objective function to be the total cost */
		int k = 0;
		for(int i=0; i < mrc.size(); ++i){
		  colno[k] = 6*i+6;
			row[k++] = 1;
		}

		if(!set_obj_fnex(lp, k, row, colno))
			ret = 4;
	}


	if(ret == 0) {
		/* set the object direction to maximize */

		set_minim(lp);

		/* just out of curioucity, now show the model in lp format on screen */
		/* this only works if this is a console application. If not, use write_lp and a filename */
		/* write_LP(lp, stdout); */
		write_lp(lp, "model.lp");

		/* I only want to see important messages on screen while solving */
		set_verbose(lp, IMPORTANT);

		/* Now let lpsolve calculate a solution */
		ret = solve(lp);
		/*
		if(ret == OPTIMAL)
			ret = 0;
		else
			ret = 5;
			*/
	}

	//std::cerr << ret << "\tobjective function set\tsolving:\n" ;

	if(ret == 0) {
		/* a solution is calculated, now lets get some results */

		/* objective value */
		fprintf(stderr,"Objective value: %f\n", get_objective(lp));
		fprintf(stderr,"latency bound: %f\n", min_lats[0]);

		/* variable values */
		get_variables(lp, row);
		for(j = 0; j < Ncol; j++)
			fprintf(stderr,"%s: %f\n", get_col_name(lp, j + 1), row[j]);

		L1_size = (unsigned)ceil(row[0]);
		L2_size = (unsigned)ceil(row[1]);

		/* we are done now */
	}

	/* free allocated memory */
	if(row != NULL)
		free(row);
	if(colno != NULL)
		free(colno);

	if(lp != NULL) {
		/* clean up such that all used memory by lpsolve is freed */
		delete_lp(lp);
	}

	return ret;

}



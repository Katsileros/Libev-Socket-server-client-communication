#include <iostream>

struct solver
{
	int N;
	int data[];
	int flag;
	int executed;
	
	solver()
	{
		std::cout << "Created solver object" << std::endl;
		flag = 0;
		executed = 0;
	}

	int run(int n)
	{
		flag = 0;
		N = n;
		std::cout << "Solver is running" << std::endl;
		solve();
		executed = 0;
		flag = 1; //finished
		stop();
		return 0;
	}
	
	int stop()
	{
		std::cout << "Solver stopped" << std::endl;
		return 0;
	}
	
	int solve()
	{
		executed = 1;
		sleep(10);
		
		//~ for(int i=0;i<N;i++)
		//~ {
			//~ data[i] = i;
		//~ }
		return 0;
	}
	
	int* getData()
	{ 
		return data; 
	}

};

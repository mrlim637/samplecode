/*
*Course: CS 100 Fall 2013
*
* First Name: Timothy
* Last Name: Lim
* Username: tlim007
* email address: tlim007@ucr.edu
*
*
* Assignment: hw5
*
* I hereby certify that the contents of this file represent
* my own original individual work. Nowhere herein is there
* code from any outside resources such as another individual,
* a website, or publishings unless specifically designated as
* permissible by the instructor or TA. */

#include <iostream>
#include <vector>
#include <sstream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

using namespace std;

void close_pipes(int const pipes[][2], int total_pipes)
{
	for(int i = 0; i < total_pipes; ++i)
	{
	   if(pipes[i][STDIN_FILENO] >= 0) 
		   close(pipes[i][STDIN_FILENO]);
	   if(pipes[i][STDOUT_FILENO] >= 0) 
			close(pipes[i][STDOUT_FILENO]);
	}
}

int execute(char *argv[], vector<string>& args, int pipes[][2], int pipe_num, bool wait)
{
    string in = "";
    string out = "";
    int in_file;
    int out_file;
    bool input = false;
    bool output = false;
	
	//Parses for input and output redirection
    for(int i = 0; i < args.size(); ++i)
    {
        if(args.at(i) == "<")
        {
            in = args.at(i + 1);
            input = true;
        }

        if(args.at(i) == ">")
        {
            out = args.at(i + 1);
            output = true;
        }
    }
	
	in_file = open(in.c_str(), O_RDONLY, S_IREAD);

	out_file = open(out.c_str(), O_CREAT |  O_WRONLY | O_TRUNC, S_IRWXU);

    int status;
    int pid = fork();
    switch (pid)
    {
        case -1:
        {
            cout << "Fork failed\n";
            return -1;
        }
        case 0:
        {
            
            //Pipe things through or input redirection
            if(pipes[pipe_num][STDIN_FILENO] >= 0) 
            {
                dup2(pipes[pipe_num][STDIN_FILENO], STDIN_FILENO); 
			}
			
			if(input)
			{
				dup2(in_file, STDIN_FILENO);
			}
				
			
            if(pipes[pipe_num][STDOUT_FILENO] >= 0)
            {
                dup2(pipes[pipe_num][STDOUT_FILENO], STDOUT_FILENO);
			}
                
			if(output)
			{
				dup2(out_file, STDOUT_FILENO);
			}
				
            close_pipes(pipes, 32);
            close(in_file);
			
           if(execvp(argv[0], argv) == -1)
           {
               perror("execvp failed");
           }
            exit(1);
        }
	}

    return pid;
}

int splitargs(string input, vector<vector<string> >& args)

{
    stringstream ss(input);
    string split;

    vector<string> command;

    int i = 1;
    //Splits input up into seperate vectors
    while(ss >> split)
    {
        if(split == "|")
        {
            args.push_back(command);

            i++;
            command.clear();
            continue;
        }
        command.push_back(split);

    }
    
    args.push_back(command);
    
    return i;

}

//Converts vector into char *argv[]
void convertchar(vector<string>& args, char *argv[])
{   
    
    int i;
    
    for(i = 0; i < args.size(); ++i)
    {
		if(args.at(i) == "<" || args.at(i) == ">")
		{
			break;
		}
		
		char * arg = new char[args.at(i).size() + 1];
		strcpy(arg, args.at(i).c_str());
		argv[i] = arg;
	}
    argv[i] = '\0';
  

}

int main()
{
    int pipes[32][2];
    int children[32];
    
    for(;;)
    {
        string input;
        char *argv[64];
        vector<vector<string> > args;
        
        int num_exec;
        int pipe_num;
        bool wait = true;


        cout << "% " << flush;
        getline(cin, input);
        cout << endl;
        num_exec = splitargs(input, args);
		
		//Reset pipes
        for(pipe_num = 0; pipe_num<32; ++pipe_num)
        {
			pipes[pipe_num][STDIN_FILENO] = -1; 
			pipes[pipe_num][STDOUT_FILENO] = -1;
        }
        
        //Create pipes
        for(pipe_num = 0; pipe_num < (num_exec - 1); ++pipe_num)
         {
             int p[2];
             pipe(p);
             pipes[pipe_num][STDOUT_FILENO] = p[STDOUT_FILENO];
             pipes[(pipe_num + 1)][STDIN_FILENO] = p[STDIN_FILENO];
         }
        
        //Run execs
        for(pipe_num = 0; pipe_num < num_exec; ++pipe_num)
        {
            convertchar(args.at(pipe_num), argv);
            
            //Checks for &
            for(int i = 0; i < args.at(pipe_num).size(); ++i)
            {
				if(args.at(pipe_num).at(i) == "&")
					wait = false;
			}
            
            //Store PIDs and execute
		    children[pipe_num] = execute(argv, args.at(pipe_num), pipes, 
										 pipe_num, wait);

        }
        
        close_pipes(pipes, 32);
        
        //Waits for children to finish
        if(wait)
			for(pipe_num=0; pipe_num < num_exec; ++pipe_num)
			{
				 int status;
				 waitpid(children[pipe_num], &status, 0);
			}
   
    }
}

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <dirent.h>
#include <sstream>

using namespace std;

const int MAXDATASIZE = 999999;
const int QUEUE = 10;
const unsigned char isFile =0x8;

void doWork(int new_fd)
{
	int numbytes;
	
	char buf[MAXDATASIZE];
	memset(&buf, 0, sizeof(buf));
	
	//Receive the directory
	numbytes = recv(new_fd, buf, sizeof(buf), 0);
	buf[numbytes] = '\0';
	//Print out directory
	char path[255];
	memset(&path, 0, sizeof(path));
	strcat(path, buf);
	//strcat(path, "/");
	
	//No directory passed we exit
	if(numbytes <= 0)
	{
		exit(1);
	}
	else
	{
		
		DIR *dir;
		struct dirent *ent;
		
		
		//Open up directory
		if((dir = opendir(path)) != NULL)
		{
			//Read what is in the directory
			while((ent = readdir(dir)) != NULL)
			{
				//If it is a file then we work on it
				if(ent->d_type == isFile)
				{	
					//Create path to file
					char newpath[255];
					memset(&newpath, 0, sizeof(newpath));
					strcat(newpath, path);
					strcat(newpath, "/");
					strcat(newpath, ent->d_name);
					
					//Open file to be read
					int input_file = open(newpath, O_RDONLY, S_IREAD);
					
					int total_bytes_read = 0;
					
					char filebuf[MAXDATASIZE];
					memset(&filebuf, 0, sizeof(filebuf));
					
					while(1)
					{
						
						int bytes_read = read(input_file, filebuf, sizeof(filebuf));
						
						total_bytes_read += bytes_read;
						if (bytes_read == 0) // We're done reading from the file
							break;
						else if (bytes_read == -1)
						{
							total_bytes_read = 0;
							break;
						}
					}
					
					string test = ent->d_name;
					//Write the name of the file to client
					if(write(new_fd, test.c_str(), test.size()) == -1)
					{
						cout << "Server: write error\n";
					}
					else
					{
						cout << "Server: Sending File: " << test << endl;
					}
					
					usleep(500000);
					
					//Write the size of the file to client
					stringstream ss;
					string str;
					
					ss << total_bytes_read;
					ss >> str;
					
					if((write(new_fd, str.c_str(), str.size())) == -1)
					{
						cout << "Server: write error\n";
					}
					
					usleep(500000);
					
					//Write the file to the client
					if(total_bytes_read == 0)
						write(new_fd, filebuf, total_bytes_read);
						
					ifstream in(newpath);
					string contents((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());

					int bytes_written = write(new_fd, contents.c_str(), contents.size());
					
					usleep(500000);
				}
			}
			
			close(new_fd);
			exit(1);
		}
	}
	
	close(new_fd);
}

void startServer()
{
	int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size;
    
	int port = 22759;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd <= -1)
        cout << "Server: socket() failed\n";
    else
        cout << "Server: socket() success\n";

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), 0, 8);

    if((bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_un))) == -1)
    {
        cout << "Server: bind() failed\n";
        exit(1);
    }
    else
        cout << "Server: bind() success\n";
       
    if((listen(sockfd, QUEUE)) <= -1)
    {
        cout << "Server: listen() failed\n";
        exit(1);
    }
    else
        cout << "Server: listen() success\n";

    sin_size = sizeof(struct sockaddr_un);

	//When we get a new accept we fork to do the work
	while(1)
	{
		cout << "Listening\n";
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, 
					  (socklen_t *)&sin_size);
					  
		int pid = fork();

		switch(pid)
		{
			case -1:
			{
				cout << "Fork failed\n";
				exit(1);
				break;
			}
			case 0:
			{
				close(sockfd);
				doWork(new_fd);
				exit(1);
				break;     
			}
		}
            
	}
                  
    
    cout << "Server ending\n";
    close(new_fd);
    close(sockfd);
}

int main()
{ 
    startServer();
}

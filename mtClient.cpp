#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdio.h>
#include <iostream>
#include <fcntl.h>
#include <pthread.h>
#include <sstream>

using namespace std;

const int MAXDATASIZE = 999999;
pthread_mutex_t client_mutex;

struct threadInfo
{
	char* hostname;
	char* directory;
	long threadid;
};

void *startClient(void* threadInfo)
{	
	struct threadInfo* tInfo = (struct threadInfo*) threadInfo;
	
	int sockfd, numbytes;
	struct hostent *he;
	struct sockaddr_in their_addr;
	char buf[MAXDATASIZE];
	
	int port = 22759;
	
	if((he=gethostbyname(tInfo->hostname)) == NULL)
    {
        cout << "Client: Unable to get host\n";
        exit(1);
    }

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		cout << "Client: socket() failed\n";
	}
	else
		cout << "Client: socket() success\n";
    
	their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(port);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), '\0', 8);


	if((connect(sockfd, (struct sockaddr *)&their_addr,  sizeof(struct sockaddr_un))) == -1)
	{
		cout << "Client: connect() failed\n";
        exit(1);
	}
	else
		cout << "Client: connect() success\n";
	
	//Send directory
	if(write(sockfd, tInfo->directory, strlen(tInfo->directory)) == -1)
	{
		cout << "Server: write error\n";
	}
	else
	{
		cout << "Server : write success\n";
	}
		
	//Create path and in current directory
	char path[255];
	memset(&path, 0, sizeof(path));
	strcat(path, "./thread");
	stringstream ss;
	
	ss << tInfo->threadid;
	
	strcat(path, ss.str().c_str());
	strcat(path, "/");
	
	//Create the directory
	
	mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	
	while(1)
	{	
		char buf[MAXDATASIZE];
		char filepath[255];
        memset(&buf, 0, sizeof(buf));
        memset(&filepath, 0, sizeof(filepath));
        
        //Recieve name of the file
        numbytes = 0;
        numbytes = recv(sockfd, buf, sizeof(buf), 0);
        buf[numbytes] = '\0';
        
        if(numbytes <= 0)
        {
			cout << "We are done done\n";
			close(sockfd);
			pthread_mutex_destroy(&client_mutex);
			pthread_exit(NULL);

            exit(1);
        }
        
        //Append file to directory
		strcat(filepath, path);
		strcat(filepath, buf);
        
        //Open the file and create it
        int fd = open(filepath, O_CREAT |  O_WRONLY | O_TRUNC, S_IRWXU);
        
        //Recieve the size of the file
        int size;
        memset(&buf, 0, sizeof(buf));
        numbytes = 0;
        numbytes = recv(sockfd, buf, sizeof(buf), 0);
        buf[numbytes] = '\0';
        
        size = atoi(buf);
        
        //Create a buf in the new size
        char filebuf[size];
        
        //Recieves the file and writes it to the FD
        int total_bytes = size;
        numbytes = 0;
        do
        {
			numbytes = recv(sockfd, filebuf, sizeof(filebuf), 0);
			total_bytes -= numbytes;
			
		}while(total_bytes != 0);
		buf[numbytes] = '\0';
        //Received the file
        int bytes_read = size;
        
		int bytes_written;
		
		//Write what to recievd to actual file
		char *p = filebuf;	
		while(bytes_read > 0)
		{
			bytes_written = write(fd, filebuf, bytes_read);
			if (bytes_written <= 0) 
			{
				break;
			}
			bytes_read -= bytes_written;
			p += bytes_written;
		}
        
	}
	
	pthread_exit(NULL);
    close(sockfd);
}


int main(int argc, char *argv[])
{
	pthread_t threads[10];
    pthread_mutex_init(&client_mutex, NULL);
	struct threadInfo *tInfo = new threadInfo;
	
	tInfo->hostname = argv[1];
	tInfo->directory = argv[2];
	

    
    for (long t = 0; t < 10; t++) 
    {

		tInfo->threadid = t;

		if(int rc = pthread_create(&threads[t], NULL, startClient, tInfo))
		{
				cout << "Error; return code from pthread_create() is" << rc << endl;
				exit(-1);
		}
		usleep(500000);
    }


    for (int i = 0; i < 10; ++i)
		pthread_join(threads[i], NULL);
		
    pthread_mutex_destroy(&client_mutex);
    
	//startClient(argc, argv);
    return 0;
}

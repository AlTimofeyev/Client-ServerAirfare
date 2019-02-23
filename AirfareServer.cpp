/*
  Author: Al Timofeyev
  Date:   2/12/2019
  Desc:   The server end of this client/server program Used
          to assign seats to the clients.
          You can either start it with a row and column of
          seats available as command line parameters, or no
          parameters at all.
          Please read the ReadMe.txt file.
*/

#include <iostream>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

using namespace std;

// Function Prototypes
void printSeats();
void deleteSeats();
bool allSeatsTaken();
void * connectToClient(void * connfd);
bool assignSeat(int row, int column);
void sendSeatMap(int connection);

#define DEFAULT_ROWS 10
#define DEFAULT_COLUMNS 10
#define PORT 8465

int** seats;
int rows = DEFAULT_ROWS;        // Height
int columns = DEFAULT_COLUMNS;  // Width
int listenfd;   // This will be the server socket.
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[])
{
  // If the number of Rows x Columns was provided.
  if(argc > 2)
  {
    rows = stoi(argv[1]);
    columns = stoi(argv[2]);

    seats = new int*[rows];
    for(int height = 0; height < rows; height++)
    {
      seats[height] = new int[columns];
      for(int width = 0; width < columns; width++)
        seats[height][width] = 0;
    }
  }
  // Else, if only the program name is provided,
  // assign the default seating arrangement.
  else
  {
    seats = new int*[rows];
    for(int height = 0; height < rows; height++)
    {
      seats[height] = new int[columns];
      for(int width = 0; width < columns; width++)
        seats[height][width] = 0;
    }
  }

  // ********** Connect Socket **********
  // This will hold the socket information.
  struct sockaddr_in server_addr;

  listenfd = 0;       // This will be the server socket.
  int connectfd = 0;  // This will be the connected client.
  int opt = 1;        // Used for setsocket.

  // Initialize (create) the socket.
  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  // Forcefully attaching socket to the PORT.
  // This is so that the server can be stopped and restarted without error.
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
  {
    perror("\nError : setsockopt");
    return 1;
  }

  // Set the socket information.
  memset(&server_addr, '0', sizeof(server_addr));   // Set memory of server_addr.
  server_addr.sin_family = AF_INET;                 // Communication domain: IPv4 Protocol.
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // IP address (since it's local-use INADDR_ANY).
  server_addr.sin_port = htons(PORT);               // Port number.

  // Attach/bind socket to PORT.
  if(bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("\nError : Socket binding failed");
    return 1;
  }

  // Place socket into listening (passive) mode and set a
  // queue size of 20 where clients can wait.
  listen(listenfd, 20);

  // Print the initial status of the seat map.
  cout << "The Current Seating Map:\n";
  printSeats();

  // Constantly wait for a connection to the socket.
  while(!(allSeatsTaken()))
  {
    connectfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

    if(allSeatsTaken())
    {
      // Notify client of successful connection to server.
      char *msg = (char*) "Server Connection Successful!";
      write(connectfd, msg, 100);

      // Send client the current seating map.
      sendSeatMap(connectfd);
      break;
    }

    pthread_t thread;
    pthread_create(&thread, NULL, connectToClient, (void*)connectfd);
  }

  cout << "\n\n****************************************\n";
  cout << "******** All Seats Are Now TAKEN *******";
  cout << "\n****************************************\n";

  // Delete seating map.
  deleteSeats();
  return 0;
}


/* *****************************************************************
****************** All Function Definitions Below ******************
***************************************************************** */
void printSeats()
{
  if(seats == nullptr)
    return;

  cout << "\t";
  for(int i = 0; i < columns; i++)
    cout << i << " ";
  cout << "\n\n";

  for(int height = 0; height < rows; height++)
  {
    cout << height << "\t";
    for(int width = 0; width < columns; width++)
    {
      if(seats[height][width] == 0)
        cout << "o ";
      else
        cout << "X ";
    }
    cout << endl;
  }

  cout << endl;
}

// *****************************************************************
void deleteSeats()
{
  if(seats != nullptr)
  {
    for(int height = 0; height < rows; height++)
      delete[] seats[height];

    delete[] seats;
    seats = nullptr;
  }
}

// *****************************************************************
bool allSeatsTaken()
{
  bool taken = true;
  for(int height = 0; height < rows; height++)
  {
    for(int width = 0; width < columns; width++)
    {
      if(seats[height][width] == 0)
      {
        taken = false;
        return taken;
      }
    }
  }

  return taken;
}

// *****************************************************************
/*
  Assigns the client a seat on the plane.
  @param connfd The connection to the client.
*/
void * connectToClient(void * connfd)
{
  //cout << "dood\n\n";/////DEBUGGING

  // Setup connectedfd for read/write to client.
  int connectfd = (long) connfd;

  // Notify client of successful connection to server.
  char *msg = (char*) "Server Connection Successful!";
  write(connectfd, msg, 100);

  // Send client the current seating map.
  sendSeatMap(connectfd);

  // Recieve the mode from client.
  char mode[100];
  read(connectfd, mode, 100);

  // If client connected in manual mode.
  if(strcmp(mode, "manual") == 0)
  {
    // Get seating request from client and try to assign it.
    bool assigned = false;
    while(!(assigned))
    {
      int dim[2];
      read(connectfd, dim, sizeof(int)*2);

      pthread_mutex_lock(&mut);
      assigned = assignSeat(dim[0], dim[1]);
      pthread_mutex_unlock(&mut);

      if(assigned)
      {
        msg = (char*) "Seat successfully assigned.";

        // Print the New status of the seat map.
        cout << "\n\n\n****************************************\n";
        cout << "The Current Seating Map:\n";
        printSeats();
      }
      else
        msg = (char*) "Seat is already taken.";

      write(connectfd, msg, 100);
      sendSeatMap(connectfd);
    }
  }

  // Else if client connected in automatic mode.
  else
  {
    // Get seating request from client and try to assign it.
    bool assigned;
    while(!(allSeatsTaken()))
    {
      int dim[2];
      read(connectfd, dim, sizeof(int)*2);
      assigned = assignSeat(dim[0], dim[1]);
      if(assigned)
      {
        msg = (char*) "Seat successfully assigned.";

        // Print the New status of the seat map.
        cout << "\n\n\n****************************************\n";
        cout << "The Current Seating Map:\n";
        printSeats();
      }
      else
        msg = (char*) "Seat is already taken.";

      write(connectfd, msg, 100);
      sendSeatMap(connectfd);
    }
  }

  //write(connectfd, msg, 100);
  //sendSeatMap(connectfd);

  // Close connection to client.
  close(connectfd);
  pthread_exit(NULL);
}

// *****************************************************************
/*
  Assigns a seat on the plane based on row and column.
  Returns true if seat was successfully assigned, else
  returns false.
*/
bool assignSeat(int row, int column)
{
  // If can't assign the seat.
  if(seats[row][column] == 1)
    return false;

  else
    seats[row][column] = 1;

  return true;
}

// *****************************************************************
/*
  Sends the seat map to the client.
  @param connection The connection to the client.
*/
void sendSeatMap(int connection)
{
  // Send the dimensions of seat map first.
  int dimensions[] = {rows, columns};
  write(connection, dimensions, sizeof(int)*2);

  // Send the seat map.
  for(int i = 0; i < rows; i++)
    write(connection, seats[i], sizeof(int)*columns);
}

/*
  Author: Al Timofeyev
  Date:   2/12/2019
  Desc:   The client end of this client/server program Used
          to assign seats to the clients.
          It expects up to 2 command line parameters.
          First the mode followed by the ini file.
          If those two aren't provided, the defaults will be used.
          Please read the ReadMe.txt file.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <vector>
#include <ctime>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

using namespace std;

// Function Prototypes.
void printSeats();
void deleteSeats();
bool allSeatsTaken();
void assignSeatManual(int sockfd, char *msg);
void assignSeatAutomatic(int sockfd, char *msg);

#define DEFAULT_ADDR "127.0.0.1"
#define DEFAULT_PORT 8465
#define DEFAULT_TIMEOUT 20

int** seats;
int dimensions[2];  // Dimensions of seats[].
int rows;           // Height
int columns;        // Width

int main(int argc, char *argv[])
{
  char *sockAddr = (char*) DEFAULT_ADDR;
  int sockPort = DEFAULT_PORT;
  int timeout = DEFAULT_TIMEOUT;
  string mode = "automatic";
  string filename;

  // If there was a file provided.
  if(argc == 3)
  {
    mode = argv[1];
    filename = argv[2];

    // Open the file an make sure it exists.
    ifstream inputFile(filename);
    if(inputFile.fail())
    {
      cout << "Failed to open file: " << filename << endl;
      return 0;
    }

    // Read the file and parse the integers into numList.
    string line;

    for(int i = 1; !inputFile.eof(); i++)
    {
      getline(inputFile, line);
      istringstream stream(line);
      if(line.length() == 0)
        break;
      vector<string> words((istream_iterator<string>(stream)), istream_iterator<string>());

      if(i == 1)
      {
        sockAddr = new char[words[2].length() + 1];
        memcpy(sockAddr, words[2].c_str(), words[2].length() + 1);
      }
      else if(i == 2)
        sockPort = stoi(words[2]);
      else if(i == 3)
        timeout = stoi(words[2]);
    }
    inputFile.close();
  }

  // Else if only the mode was provided.
  else if (argc == 2)
    mode = argv[1];

  // ********** Connect Socket **********
  // This will hold the socket information.
  struct sockaddr_in server_addr;

  int sockfd = 0;   // This will connect to server socket.
  int valRead = 0;  // This will hold number of values read from server.
  char msg[1024];

  // Initialize (create) the socket.
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
      cout << "\nError : Could not create socket\n";
      return 1;
  }

  // Allocate memory.
  memset(&server_addr, '0', sizeof(server_addr)); // Set memory of server_addr.
  memset(msg, '0', sizeof(msg));                  // Set memory of msg buffer.

  // Set socket information.
  server_addr.sin_family = AF_INET;         // Communication domain: IPv4 Protocol.
  server_addr.sin_port = htons(sockPort);   // Port number.

  // Try connecting the address.
  if(inet_pton(AF_INET, sockAddr, &server_addr.sin_addr)<=0)
  {
      cout << "\nError : Invalid address / Address not supported\n";
      return 1;
  }

  // Try connecting to the server socket.
  for(int i = 0; i <= timeout; i++)
  {
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
      // If connection fails by timeout, return FAIL.
      if(i == timeout)
      {
        cout << "\nError : Connection To Server Failed\n";
        return 1;
      }
      else
      {
        sleep(1);
        continue;
      }
    }

    // Else, if the connection was finally successful, exit loop.
    else
      break;
  }

  // Notify client of successful server connection.
  valRead = read(sockfd, msg, 100);
  cout << "\n***** From Server : " << msg << " *****\n\n";

  // Send the mode to server.
  memset(msg, '0', sizeof(msg)); // Reset memory of msg buffer.
  memcpy(msg, mode.c_str(), mode.length() + 1);
  write(sockfd, msg, 100);

  // Assign a seat for the client.
  if(mode.compare("manual") == 0)
    assignSeatManual(sockfd, msg);
  else
    assignSeatAutomatic(sockfd, msg);

  // If all the seats are now taken.
  if(allSeatsTaken())
  {
    cout << "\n\n****************************************\n";
    cout << "******** All Seats Are Now TAKEN *******";
    cout << "\n****************************************\n";
  }

  // Close connection to server.
  close(sockfd);

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
  Assigns seats based on client request.
*/
void assignSeatManual(int sockfd, char *msg)
{
  // Get the seating map from the server.
  read(sockfd, dimensions, sizeof(int)*2);
  rows = dimensions[0];
  columns = dimensions[1];
  seats = new int*[rows];
  for(int height = 0; height < rows; height++)
  {
    seats[height] = new int[columns];
    read(sockfd, seats[height], sizeof(int)*columns);
  }

  // Ask client to request a seat.
  while(!(strcmp(msg, "Seat successfully assigned.") == 0 || allSeatsTaken()))
  {
    // Print the seating map to the client.
    cout << "The Current Seating Map:\n";
    printSeats();

    // Ask client what seat seat to assign thm and send to server.
    cout << "Please enter another row and column to assign: ";
    int r, c;
    while(1)
    {
      cin >> r;
      cin >> c;
      if(!(r >= rows || c >= columns || r < 0 || c < 0))
        break;

      cout << "\nOne of both of the values you entered is out of range.\n";
      cout << "Please enter a row between 0 and " << rows-1;
      cout << "\n      and a column between 0 and " << columns-1 << ": ";
    }

    // Send the row and column to server to be assigned.
    int dim[] = {r, c};
    write(sockfd, dim, sizeof(int)*2);

    // See if the seating assignment was successful.
    read(sockfd, msg, 100);
    cout << "\n\n\n****************************************\n";
    cout << msg << endl;

    // Get the seating map from the server.
    read(sockfd, dimensions, sizeof(int)*2);
    for(int height = 0; height < rows; height++)
      read(sockfd, seats[height], sizeof(int)*columns);
  }

  // Print the seating map to the client.
  cout << "The New Seating Map:\n";
  printSeats();
}

// *****************************************************************
/*
  Assigns seats automatically based on seat availability.
*/
void assignSeatAutomatic(int sockfd, char *msg)
{
  // Get the seating map from the server.
  read(sockfd, dimensions, sizeof(int)*2);
  rows = dimensions[0];
  columns = dimensions[1];
  seats = new int*[rows];
  for(int height = 0; height < rows; height++)
  {
    seats[height] = new int[columns];
    read(sockfd, seats[height], sizeof(int)*columns);
  }

  // Print the seating map to the client.
  cout << "The Current Seating Map:\n";
  printSeats();

  // Randomly generate a seat request until no seats are available.
  srand(time(0)); // So each client has a different random generator.
  while(!(allSeatsTaken()))
  {
    // Ask client what seat seat to assign thm and send to server.
    int r = rand() %rows, c = rand() % columns;
    cout << "Chosen random row and column to assign: " << r << " " << c << endl;
    sleep(2);

    // Send the row and column to server to be assigned.
    int dim[] = {r, c};
    write(sockfd, dim, sizeof(int)*2);

    // See if the seating assignment was successful.
    read(sockfd, msg, 100);
    cout << "\n\n\n****************************************\n";
    cout << msg << endl;

    // Get the seating map from the server.
    read(sockfd, dimensions, sizeof(int)*2);
    for(int height = 0; height < rows; height++)
      read(sockfd, seats[height], sizeof(int)*columns);

    // Print the seating map to the client.
    cout << "The Current Seating Map:\n";
    printSeats();
  }

  // Print the seating map to the client.
  cout << "The New Seating Map:\n";
  printSeats();
}

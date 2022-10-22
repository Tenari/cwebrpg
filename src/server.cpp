#define PENDING_CONNECTIONS_BACKLOG_COUNT 100
#define MAX_HEADER_SIZE Kilobytes(32)
#define MAX_HEADER_LINE_SIZE Kilobytes(8)
#define MAX_PATH_SIZE Kilobytes(1)
#define PUBLIC_DIR "public_html"
#define HEADER_200 "HTTP/1.0 200 OK\nServer: C-Serv v0.2\nContent-Type: %s\n\n"
#define HEADER_400 "HTTP/1.0 400 Bad Request\nServer: C-Serv v0.2\nContent-Type: %s\n\n"
#define HEADER_404 "HTTP/1.0 404 Not Found\nServer: C-Serv v0.2\nContent-Type: %s\n\n"
#define CONTENT_JSON "application/json"

global_variable int ListeningSocket;

struct server_memory {
  char *Header;
  char *HeaderLine;
  char *Path;
};

global_variable server_memory ServerMemory;

struct http_request {
	int ReturnCode;
	char *Filename;
	char *Query;
	const route *Route;
};

internal void cleanupRequest(http_request *R) {
  if (R->Filename) {
    free(R->Filename);
  }
  if (R->Query) {
    free(R->Query);
  }
  *R = (http_request){ 0 };
}

struct thread_todo {
  http_request Request;
  int ConnectionSocket;
  world *World;
  FILE *ReadFile;
  pthread_cond_t HasWork;
  pthread_mutex_t HasWorkMutex;
};

internal void serverFail(const char *Message) {
  fprintf(stderr, "%s", Message);
  exit(EXIT_FAILURE);
}

internal char * makePublicFilePath(char* Path) {
  char *Base; // Allocate some memory not in read only space to store "public_html"
  if( (Base = (char *)malloc(sizeof(char) * (strlen(Path) + 18))) == NULL) {
    serverFail("Error allocating memory to Base in publicFilePath()\n");
  }
  
  strcpy(Base, PUBLIC_DIR);  // Copy public_html to the non read only memory
  // Append the path after public_html to create the full filename
  strcat(Base, Path);

  return Base;
}

internal int transmitMessageOverSocket(int Socket, char *Message) {
  return write(Socket, Message, strlen(Message));
}

// reads the header from the request into the Header variable you pass in
internal FILE* serverReadHeaderFromSocket(int fd, char *Header, char *HeaderLine) {
    // A file stream
    FILE *sstream;
    
    // Try to open the socket to the file stream and handle any failures
    if( (sstream = fdopen(fd, "r")) == NULL)
    {
      fprintf(stderr, "Value of errno: %d\n", errno);
      fprintf(stderr, "Error opening file: %s\n", strerror(errno));
      printf("Error %d opening file descriptor %d in serverReadHeaderFromSocket(), dropping request\n", sstream, fd);
      return (FILE*)-1;
    }
    
    // Set Header and HeaderLine to null
    Header[0] = '\0';
    HeaderLine[0] = '\0';
    
    int end;                              // Int to keep track of what getline returns
    size_t size = MAX_HEADER_LINE_SIZE;   // Size variable for passing to getline
    
    // While getline is still getting data
    while( (end = getline( &HeaderLine, &size, sstream)) > 0)
    {
        // If the line it read is a caridge return and a new line we're at the end of the header so break
        if( strcmp(HeaderLine, "\r\n") == 0)
        {
            break;
        }
        
        // Append the latest line we got to Header
        strcat(Header, HeaderLine);
    }
    return sstream;
}

internal http_request parseRequest(char *Header){
  http_request Request;
  Request.ReturnCode = 400; // initialize to error code, so we never erroneously return 200-success
  Request.Route = NULL;
  Request.Query = NULL;
       
  char* Query;
  if( (Query = (char *)malloc(sizeof(char) * MAX_PATH_SIZE)) == NULL) {
    serverFail("Error allocating memory to Query in parseRequest()\n");
  }
  // Get the Path from the header
  clearString(Query, MAX_PATH_SIZE / sizeof(char));
  clearString(ServerMemory.Path, MAX_PATH_SIZE / sizeof(char));
  sscanf(Header, "GET %s HTTP/1.1", ServerMemory.Path);
  bool FoundQuery = false;
  int qi = 0;
  int PathSize = strlen(ServerMemory.Path);
  for (int i = 0; i < PathSize; i++) {
    if (FoundQuery) {
      Query[qi] = ServerMemory.Path[i];
      qi++;
    }
    if (ServerMemory.Path[i] == '?') {
      FoundQuery = true;
      ServerMemory.Path[i] = '\0'; // truncate the Path here
    }
  }
  printf("\nrequest for %s ",ServerMemory.Path);
  if (FoundQuery) {
    printf("with query %s ",Query);
    Request.Query = Query;
  }
  
  Request.Filename = makePublicFilePath(ServerMemory.Path);
  FILE * PublicPageExists = fopen(Request.Filename, "r" );
    
  // Check if its a directory traversal attack
  if ( strstr(ServerMemory.Path, "..") != NULL ) {
    Request.ReturnCode = 400;
    Request.Filename = makePublicFilePath((char *)"/400.html");
  } else if(strcmp(ServerMemory.Path, "/") == 0) {
    // If they asked for / return index.html
    Request.ReturnCode = 200;
    Request.Filename = makePublicFilePath((char *)"/index.html");
  } else if( PublicPageExists != NULL ) {
    // If they asked for a specific page and it exists
    // because we opened it sucessfully, return it 
    Request.ReturnCode = 200;
    fclose(PublicPageExists); // Close the file stream
  } else {
    Request.Filename = NULL;
    // either we find it in the routes or it's a 404
    for (int i = 0; i < RouteCount; i++) {
      if (strcmp(Routes[i].Path, ServerMemory.Path) == 0) {
        Request.ReturnCode = 200;
        Request.Route = &Routes[i];
        return Request;
      }
    }
    // If we get here the file they want doesn't exist so return a 404
    Request.ReturnCode = 404;
    Request.Filename = makePublicFilePath((char *)"/404.html");
  }
    
  return Request;
}

// returns header size in bytes
internal int printResponseHeader(int fd, int returncode, const char* contentType) {
  const char* header;
  // Print the header based on the return code
  switch (returncode)
  {
      case 200:
        header = HEADER_200;
      break;
      
      case 400:
        header = HEADER_400;
      break;
      
      case 404:
        header = HEADER_404;
      break;
  }
  if (header != NULL) {
    char interpolatedHeader[ strlen(header) + strlen(contentType) ];
    sprintf( interpolatedHeader, header, contentType );
    transmitMessageOverSocket(fd, interpolatedHeader);
    return strlen(interpolatedHeader);
  } else {
    return -1;
  }
}
// returns file size in bytes
internal int printResponseFile(int fd, char *filename) {
    /* Open the file filename and echo the contents from it to the file descriptor fd */
    
    // Attempt to open the file 
    FILE *read;
    if( (read = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "Error opening file in printFile()\n");
        exit(EXIT_FAILURE);
    }
    
    // Get the size of this file for printing out later on
    int totalsize;
    struct stat st;
    stat(filename, &st);
    totalsize = st.st_size;
    
    // Variable for getline to write the size of the line its currently printing to
    size_t size = 1;
    
    // Get some space to store each line of the file in temporarily 
    char *temp;
    if(  (temp = (char *)malloc(sizeof(char) * size)) == NULL )
    {
        fprintf(stderr, "Error allocating memory to temp in printFile()\n");
        exit(EXIT_FAILURE);
    }
    
    // Int to keep track of what getline returns
    int end;
    
    // While getline is still getting data
    while( (end = getline( &temp, &size, read)) > 0)
    {
        transmitMessageOverSocket(fd, temp);
    }
    
    // Final new line
    transmitMessageOverSocket(fd, (char *)"\n");
    
    // Free temp as we no longer need it
    free(temp);
    fclose(read);
    
    // Return how big the file we sent out was
    return totalsize;
}

// handle a long-running request when pthread_cond_signal'ed
internal void *runServerThread(void *arg) {
  thread_todo *Work = (thread_todo *)arg;

  while (true) {
    pthread_mutex_lock( &(Work->HasWorkMutex) );
    pthread_cond_wait( &(Work->HasWork), &(Work->HasWorkMutex) );

    if (Work->ConnectionSocket != 0) {
      printResponseHeader(Work->ConnectionSocket, Work->Request.ReturnCode, CONTENT_JSON);

      char * ResponseContent = (Work->Request.Route->FnPtr)(Work->World, Work->Request.Query);
      transmitMessageOverSocket(Work->ConnectionSocket, ResponseContent);
      transmitMessageOverSocket(Work->ConnectionSocket, (char *)"\n");

      fclose(Work->ReadFile);
      close(Work->ConnectionSocket);
      Work->ConnectionSocket = 0;
      cleanupRequest(&Work->Request);
    }

    pthread_mutex_unlock( &Work->HasWorkMutex );
  }
}

internal void startServer(int Port, int ThreadCount, world *World) {
  // allocate memory
  if( (ServerMemory.Header = (char *)malloc(sizeof(char) * MAX_HEADER_SIZE)) == NULL ) {
    serverFail("Error allocating Header memory\n");
  }
  if( (ServerMemory.HeaderLine = (char *)malloc(sizeof(char) * MAX_HEADER_LINE_SIZE)) == NULL ) {
    serverFail("Error allocating HeaderLine memory\n");
  }
  if( (ServerMemory.Path = (char *)malloc(sizeof(char) * MAX_PATH_SIZE)) == NULL) {
    serverFail("Error allocating Path memory\n");
  }

  // setup listening socket
  ListeningSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (ListeningSocket < 0) serverFail("Error creating listening socket.\n");

  // set all bytes in socket address structure to zero, and fill in the relevant data members
  int ConnectionSocket;
  sockaddr_in SocketAddress;
  uint SocketAddressSize = sizeof(SocketAddress);
  memset(&SocketAddress, 0, SocketAddressSize);
  SocketAddress.sin_family      = AF_INET;
  SocketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  SocketAddress.sin_port        = htons(Port);
  
  // bind to the socket address, or fail
  int BindResult = bind(ListeningSocket, (sockaddr *) &SocketAddress, SocketAddressSize);
  if (BindResult < 0) serverFail("Error calling bind()\n");

  // Listen on socket we just bound, or fail
  int ListenResult = listen(ListeningSocket, PENDING_CONNECTIONS_BACKLOG_COUNT);
  if (ListenResult == -1) serverFail("Error Listening on socket\n");
    
  timeval StartTime;
  timeval EndTime;

  printf("server starting at http://localhost:%d/ with %d threads\n", Port, ThreadCount);

  // spin off our pool of threads for responding to SlowRequests
  pthread_t Threads[ThreadCount];
  thread_todo SlowRequests[ThreadCount];
  for (int i = 0; i < ThreadCount; i++) {
    SlowRequests[i] = (thread_todo){ 0 }; // clearing all to 0
    SlowRequests[i].World        = World;
    SlowRequests[i].HasWork      = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    SlowRequests[i].HasWorkMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    if (pthread_create(&Threads[i], NULL, runServerThread, &SlowRequests[i])) {
      serverFail("Error creating thread\n");
    }
  }

  // Loop forever serving requests
  while(1) {
    // accept next connection
    ConnectionSocket = accept(
      ListeningSocket,
      (struct sockaddr *)&SocketAddress,
      &SocketAddressSize
    );
    if(ConnectionSocket == -1) serverFail("Error accepting connection\n");
    gettimeofday(&StartTime,NULL);

    // parse the header from the connection
    FILE* HeaderReadStatus = serverReadHeaderFromSocket(ConnectionSocket, ServerMemory.Header, ServerMemory.HeaderLine);
    if (HeaderReadStatus == (FILE*)-1) {
      close(ConnectionSocket);
      continue;
    }
    http_request RequestDetails = parseRequest(ServerMemory.Header);

    // pass slow routes off to threads for handling
    if (RequestDetails.Route && RequestDetails.Route->Slow) {
      bool InQueue = false;
      while(!InQueue) {
        for (int i = 0; i < ThreadCount; i++) {
          if (SlowRequests[i].ConnectionSocket == 0) {
            pthread_mutex_lock( &(SlowRequests[i].HasWorkMutex) );

            SlowRequests[i].Request = RequestDetails;
            SlowRequests[i].ConnectionSocket = ConnectionSocket;
            SlowRequests[i].ReadFile = (FILE *)HeaderReadStatus;

            pthread_cond_signal( &(SlowRequests[i].HasWork) );
            pthread_mutex_unlock( &(SlowRequests[i].HasWorkMutex) );

            InQueue = true;
            i = ThreadCount; // break out of loop
          }
        }
      }
    } else { // immediately handle fast routes
      const char * ContentType; // detect ContentType
      if (RequestDetails.Filename != NULL) {
        ContentType = "text/html";
      } else {
        ContentType = CONTENT_JSON;
      }
      printResponseHeader(ConnectionSocket, RequestDetails.ReturnCode, ContentType);
            
      if (RequestDetails.Filename != NULL) {
        printResponseFile(ConnectionSocket, RequestDetails.Filename);
      } else {
        // get the content from the RequestDetails.Route.FnPtr
        char * ResponseContent = (RequestDetails.Route->FnPtr)(World, RequestDetails.Query);
        transmitMessageOverSocket(ConnectionSocket, ResponseContent);
        transmitMessageOverSocket(ConnectionSocket, (char *)"\n");
      }

      gettimeofday(&EndTime,NULL);
      printf("served %d in %d usec\n", RequestDetails.ReturnCode, EndTime.tv_usec - StartTime.tv_usec);
      fclose((FILE *)HeaderReadStatus);
      close(ConnectionSocket);
      cleanupRequest(&RequestDetails);
    }
  }
}

// for responding to ctrl-c
internal void cleanupServer(int sig) {
  printf("Cleaning up connections and exiting.\n");

  free(ServerMemory.Header);
  free(ServerMemory.HeaderLine);
  free(ServerMemory.Path);

  // try to close the listening socket
  if (close(ListeningSocket) < 0) {
    serverFail("Error calling close()\n");
  }

  exit(EXIT_SUCCESS);
}

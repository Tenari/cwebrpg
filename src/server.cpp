#define PENDING_CONNECTIONS_BACKLOG_COUNT 100
#define PUBLIC_DIR "public_html"
#define HEADER_200 "HTTP/1.0 200 OK\nServer: C-Serv v0.2\nContent-Type: %s\n\n"
#define HEADER_400 "HTTP/1.0 400 Bad Request\nServer: C-Serv v0.2\nContent-Type: %s\n\n"
#define HEADER_404 "HTTP/1.0 404 Not Found\nServer: C-Serv v0.2\nContent-Type: %s\n\n"
#define CONTENT_JSON "application/json"

global_variable int ListeningSocket;

struct http_request {
	int ReturnCode;
	char *Filename;
	char *Query;
	const route *Route;
};

struct thread_todo {
  http_request Request;
  int ConnectionSocket;
  world *World;
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

internal char *serverReadHeaderFromSocket(int fd) {
    // A file stream
    FILE *sstream;
    
    // Try to open the socket to the file stream and handle any failures
    if( (sstream = fdopen(fd, "r")) == NULL)
    {
      serverFail("Error opening file descriptor in serverReadHeaderFromSocket()\n");
    }
    
    // Size variable for passing to getline
    size_t size = 1;
    
    char *Header;
    
    // Allocate some memory for Header and check it went ok
    if( (Header = (char *)malloc(sizeof(char) * size)) == NULL ) {
      serverFail("Error allocating memory to Header in serverReadHeaderFromSocket\n");
    }
  
    // Set Header to null    
    *Header = '\0';
    
    // Allocate some memory for tmp and check it went ok
    char *tmp;
    if( (tmp = (char *)malloc(sizeof(char) * size)) == NULL ) {
      serverFail("Error allocating memory to tmp in serverReadHeaderFromSocket\n");
    }
    // Set tmp to null
    *tmp = '\0';
    
    // Int to keep track of what getline returns
    int end;
    // Int to help use resize Header
    int oldsize = 1;
    
    // While getline is still getting data
    while( (end = getline( &tmp, &size, sstream)) > 0)
    {
        // If the line its read is a caridge return and a new line were at the end of the header so break
        if( strcmp(tmp, "\r\n") == 0)
        {
            break;
        }
        
        // Resize Header
        Header = (char *)realloc(Header, size+oldsize);
        // Set the value of oldsize to the current size of Header
        oldsize += size;
        // Append the latest line we got to Header
        strcat(Header, tmp);
    }
    
    // Free tmp a we no longer need it
    free(tmp);
    
    // Return the header
    return Header;
}

internal http_request parseRequest(char *Header){
  http_request Request;
  Request.ReturnCode = 400; // initialize to error code, so we never erroneously return 200-success
  Request.Filename = NULL;
  Request.Route = NULL;
  Request.Query = NULL;
       
  char* Path;
  if( (Path = (char *)malloc(sizeof(char) * strlen(Header))) == NULL) {
    serverFail("Error allocating memory to Path in parseRequest()\n");
  }
  char* Query;
  if( (Query = (char *)malloc(sizeof(char) * strlen(Header))) == NULL) {
    serverFail("Error allocating memory to Query in parseRequest()\n");
  }
  // Get the Path from the header
  sscanf(Header, "GET %s HTTP/1.1", Path);
  bool FoundQuery = false;
  int qi = 0;
  int PathSize = strlen(Path);
  for (int i = 0; i < PathSize; i++) {
    if (FoundQuery) {
      Query[qi] = Path[i];
      qi++;
    }
    if (Path[i] == '?') {
      FoundQuery = true;
      Path[i] = '\0'; // truncate the Path here
    }
  }
  printf("request for %s ",Path);
  if (FoundQuery) {
    printf("with query %s ",Query);
    Request.Query = Query;
  }
  
  FILE * PublicPageExists = fopen(makePublicFilePath(Path), "r" );
    
  // Check if its a directory traversal attack
  if ( strstr(Path, "..") != NULL ) {
    Request.ReturnCode = 400;
    Request.Filename = makePublicFilePath((char *)"/400.html");
  } else if(strcmp(Path, "/") == 0) {
    // If they asked for / return index.html
    Request.ReturnCode = 200;
    Request.Filename = makePublicFilePath((char *)"/index.html");
  } else if( PublicPageExists != NULL ) {
    // If they asked for a specific page and it exists
    // because we opened it sucessfully, return it 
    Request.ReturnCode = 200;
    Request.Filename = makePublicFilePath(Path);
    fclose(PublicPageExists); // Close the file stream
  } else {
    // either we find it in the routes or it's a 404
    for (int i = 0; i < RouteCount; i++) {
      if (strcmp(Routes[i].Path, Path) == 0) {
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
    
    // Return how big the file we sent out was
    return totalsize;
}

// continually checks `Work` to see if a long-running request was dropped in there
internal void *runServerThread(void *arg) {
  thread_todo *Work = (thread_todo *)arg;

  while (true) {
    // TODO go to sleep/wait without burning cpu somehow?
    if (Work->ConnectionSocket != 0) {
      printResponseHeader(Work->ConnectionSocket, Work->Request.ReturnCode, CONTENT_JSON);

      char * ResponseContent = (Work->Request.Route->FnPtr)(Work->World, Work->Request.Query);
      transmitMessageOverSocket(Work->ConnectionSocket, ResponseContent);
      transmitMessageOverSocket(Work->ConnectionSocket, (char *)"\n");

      printf("served long-running request\n");
      close(Work->ConnectionSocket);
      Work->Request = (http_request){ 0 };
      Work->ConnectionSocket = 0;
    }
  }
}

internal void startServer(int Port, int ThreadCount, world *World) {
  int ConnectionSocket;
  sockaddr_in SocketAddress;
  uint SocketAddressSize = sizeof(SocketAddress);

  ListeningSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (ListeningSocket < 0) serverFail("Error creating listening socket.\n");

  // set all bytes in socket address structure to zero, 
  // and fill in the relevant data members
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
    SlowRequests[i].World = World;
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
    char * Header = serverReadHeaderFromSocket(ConnectionSocket);
    http_request RequestDetails = parseRequest(Header);
    free(Header); // Free header now were done with it

    // pass slow routes off to threads for handling
    if (RequestDetails.Route && RequestDetails.Route->Slow) {
      bool InQueue = false;
      while(!InQueue) {
        for (int i = 0; i < ThreadCount; i++) {
          if (SlowRequests[i].ConnectionSocket == 0) {
            SlowRequests[i].Request = RequestDetails;
            SlowRequests[i].ConnectionSocket = ConnectionSocket;
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
      close(ConnectionSocket);
    }
  }
}

// for responding to ctrl-c
internal void cleanupServer(int sig) {
  printf("Cleaning up connections and exiting.\n");

  // try to close the listening socket
  if (close(ListeningSocket) < 0) {
    serverFail("Error calling close()\n");
  }

  exit(EXIT_SUCCESS);
}

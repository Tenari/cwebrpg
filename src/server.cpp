#define PENDING_CONNECTIONS_BACKLOG_COUNT 100
#define MAX_HEADER_SIZE Kilobytes(32)
#define MAX_HEADER_VALUE_SIZE 512
#define WEBSOCKET_KEY_LEN 256
#define MAX_HEADER_LINE_SIZE Kilobytes(8)
#define MAX_PATH_SIZE Kilobytes(1)
#define LONG_REQUEST_TIMEOUT_S 45
#define PUBLIC_DIR "public_html"
#define HEADER_200 "HTTP/1.0 200 OK\nServer: C-Serv v0.2\nContent-Type: %s\n\n"
#define HEADER_400 "HTTP/1.0 400 Bad Request\nServer: C-Serv v0.2\nContent-Type: %s\n\n"
#define HEADER_404 "HTTP/1.0 404 Not Found\nServer: C-Serv v0.2\nContent-Type: %s\n\n"
#define HEADER_WEBSOCKET "HTTP/1.1 101 Switching Protocols\nUpgrade: websocket\nConnection: Upgrade\nSec-WebSocket-Accept: "
#define CONTENT_JSON "application/json"
#define CONTENT_PNG "image/png"

global_variable int ListeningSocket;

struct http_request {
	int ReturnCode;
  bool IsPng;
	char *Filename;
	char *Query;
	const route *Route;
};

struct streaming_request {
  http_request Request;
  int ConnectionSocket;
  time_t StartedAt;
};

struct websocket_connection {
  int Socket;
  time_t StartedAt;
  entity* Player;
};

struct server_memory {
  char *Header;
  char *HeaderLine;
  char *Path;
  uchar *WebSocketKey;
  uchar WebSocketKeyResponse[WEBSOCKET_KEY_LEN];
  uchar CookieUsername[32];
  streaming_request * Streams;
  int StreamCount;
  world *World;
  websocket_connection * WebsocketConnections;
  int WebsocketCount;
};
global_variable server_memory ServerMemory;

internal void setSocketNonBlocking(int sock) {
  int opt;

  opt = fcntl(sock, F_GETFL);
  if (opt < 0) {
      printf("fcntl(F_GETFL) fail.");
  }
  opt |= O_NONBLOCK;
  if (fcntl(sock, F_SETFL, opt) < 0) {
      printf("fcntl(F_SETFL) fail.");
  }
}
internal void cleanupRequest(http_request *R) {
  if (R->Filename) {
    free(R->Filename);
  }
  if (R->Query) {
    free(R->Query);
  }
  *R = (http_request){ 0 };
}

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

internal int transmitMessageOverSocket(int Socket, char *Message, bool AddNewlineAtEnd) {
  if (!AddNewlineAtEnd) {
    return send(Socket, Message, strlen(Message), 0);
  } else {
    send(Socket, Message, strlen(Message), 0);
    return send(Socket, (char*)"\n", 1, 0);
  }
}

// reads the header from the request into the ServerMemory.Header
internal void serverReadHeaderFromSocket(int fd) {
  // Set Header and HeaderLine to null
  ServerMemory.Header[0] = '\0';
  ServerMemory.HeaderLine[0] = '\0';
  
  bool FoundEOF = false;
  while (!FoundEOF) {
    bool FoundNewline = false;
    int LinePos = 0;
    int BytesRead;
    while (!FoundNewline) {
      BytesRead = read(fd, ServerMemory.HeaderLine + LinePos, 1);
      if (BytesRead > 0) {
        if (ServerMemory.HeaderLine[LinePos] == '\n') {
          FoundNewline = true;
        } else {
          LinePos = LinePos + BytesRead;
        }
      } else {
        // EOF
        FoundNewline = true;
        FoundEOF = true;
      }
    }
    ServerMemory.HeaderLine[LinePos+1] = '\0'; // terminate the string
    // HeaderLine now contains a full line
    //printf("%s", ServerMemory.HeaderLine);
    if (stringsMatch(ServerMemory.HeaderLine, (char*)"Sec-WebSocket-Key: ")) {
      clearUString(ServerMemory.WebSocketKey, MAX_HEADER_VALUE_SIZE / sizeof(char));
      sscanf(ServerMemory.HeaderLine, "Sec-WebSocket-Key: %s\n", ServerMemory.WebSocketKey);
      generateWebsocketAcceptHeader(ServerMemory.WebSocketKey, ServerMemory.WebSocketKeyResponse);
    }
    if (stringsMatch(ServerMemory.HeaderLine, (char*)"Cookie: ")) {
      clearUString((uchar*)ServerMemory.CookieUsername, 32);
      parseUsernameFromCookie(ServerMemory.HeaderLine, ServerMemory.CookieUsername);
    }

    // If the line it read is a caridge return and a new line we're at the end of the header so break
    if (ServerMemory.HeaderLine[0] == '\r' && ServerMemory.HeaderLine[1] == '\n') {
      break;
    }

    // Append the latest line we got to Header
    strcat(ServerMemory.Header, ServerMemory.HeaderLine);
  }
}

internal http_request parseRequest(char *Header){
  http_request Request;
  Request.IsPng = false;
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
  int PathSize = strlen(ServerMemory.Path);

  if (
      PathSize >= 4 &&
      ServerMemory.Path[PathSize-4] == '.' &&
      ServerMemory.Path[PathSize-3] == 'p' &&
      ServerMemory.Path[PathSize-2] == 'n' &&
      ServerMemory.Path[PathSize-1] == 'g'
     ) {
    Request.IsPng = true;
  }

  bool FoundQuery = false;
  int qi = 0;
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
    transmitMessageOverSocket(fd, interpolatedHeader, false);
    return strlen(interpolatedHeader);
  } else {
    return -1;
  }
}
internal void printResponseFile(int Socket, char *filename) {
    /* Open the file filename and echo the contents from it to the Socket */
    
    // Attempt to open the file 
    int FileDescriptor;
    if( (FileDescriptor = open(filename, O_RDONLY)) == -1) {
      fprintf(stderr, "Error opening file in printFile()\n");
      exit(EXIT_FAILURE);
    }

    char WriteBuffer[1024];
    int BytesRead = 0;
    while((BytesRead = read(FileDescriptor, WriteBuffer, sizeof(WriteBuffer))) > 0) {
      send(Socket, WriteBuffer, BytesRead, 0);
    }
    
    // Final new line
    transmitMessageOverSocket(Socket, (char *)"\n", false);
    
    close(FileDescriptor);
}

// loop through all long-running httpStreams and send new gamestate
internal void *updateLongRunningHTTPStreams(void *arg) {
  timeval CurrentTime;

  while (true) {
    pthread_mutex_lock( &(ServerMemory.World->HasUpdatedMutex) );
    pthread_cond_wait( &(ServerMemory.World->HasUpdated), &(ServerMemory.World->HasUpdatedMutex) );

    gettimeofday(&CurrentTime,NULL);
    for (int i = 0; i < ServerMemory.StreamCount; i++) {
      streaming_request * Stream = &(ServerMemory.Streams[i]);
      if (Stream->ConnectionSocket != 0) {
        char * ResponseContent = (Stream->Request.Route->FnPtr)(ServerMemory.World, Stream->Request.Query);
        transmitMessageOverSocket(Stream->ConnectionSocket, ResponseContent, false);
        transmitMessageOverSocket(Stream->ConnectionSocket, (char *)"\n\n", false);
        free(ResponseContent);

        if ((CurrentTime.tv_sec - Stream->StartedAt) > LONG_REQUEST_TIMEOUT_S) {
          close(Stream->ConnectionSocket);
          Stream->ConnectionSocket = 0;
          cleanupRequest(&Stream->Request);
        }
      }
    }

    pthread_mutex_unlock( &(ServerMemory.World->HasUpdatedMutex) );
  }
}

internal void writeAndFreeToWebsocket(int Socket, uchar* Message) {
  uint64_t MsgLen = (uint64_t)strlen((char*)Message);
  assert(MsgLen < 4096);
  uchar Buffer[4096];
  clearUString(Buffer, 4096);
  uint64_t bytes = websocket_server_wrap((void *)Buffer, (void *)Message, MsgLen, (uchar) 1, (uchar) 1, (uchar) 1, (uchar) 0);
  free(Message);
  send(Socket, Buffer, bytes, 0);
}

// loop through all websockets and read user input onto game state input buffer
// also handles clients closing the connection
internal void *readWebsocketConnections(void *arg) {
  while (true) {
    usleep(1000); // 1ms

    for (int i = 0; i < ServerMemory.WebsocketCount; i++) {
      websocket_connection * Websocket = &(ServerMemory.WebsocketConnections[i]);
      if (Websocket->Socket != 0) {
        // read user input
        uchar UserInput[MAX_USER_INPUT_LEN];
        clearUString(UserInput, MAX_USER_INPUT_LEN);
        int BytesRead;
        // read the payload descriptor bytes
        BytesRead = read(Websocket->Socket, UserInput, 2);
        if (BytesRead < 0) {
          // there were no waiting bytes to read
          UserInput[0] = '\0';
        } else {
          uint OpCode = UserInput[0] & 0b00001111;
          if (OpCode == 8) {
            close(Websocket->Socket);
            *Websocket = (websocket_connection){ 0 };// clear to 0
            printf("cleared websocket_connection\n");
          } else if (OpCode == 0) {
            printf("calculated OpCode %d \n", OpCode);
            close(Websocket->Socket);
            *Websocket = (websocket_connection){ 0 };// clear to 0
            printf("cleared websocket_connection\n");
          } else { // assume Text opcode
            int PayloadLen = UserInput[1] & (0x7F);
            // correct PayloadLen if its one of the special codes from rfc6455
            if (PayloadLen == 126) {// https://datatracker.ietf.org/doc/html/rfc6455 defines len 126 as special signal meaning the next 2 bytes are the actual length
              BytesRead = read(Websocket->Socket, UserInput, 2);
              if (BytesRead < 0) serverFail("wtf, we read a PayloadLen of 126, but then got no data afterwards");

              PayloadLen = UserInput[0];
              PayloadLen = (PayloadLen << 8) | UserInput[1];
            } else if (PayloadLen == 127) {
                BytesRead = read(Websocket->Socket, UserInput, 8);
                if (BytesRead < 0) serverFail("wtf, we read a PayloadLen of 127, but then got no data afterwards");

                PayloadLen = UserInput[0];
                for (i = 1; i <= 7; i++) {
                  PayloadLen = ((PayloadLen << 8) | UserInput[i]);
                }
            } else {
              // the super short message (less than 126 bytes) means that PayloadLen is alredy correct
            }

            // get masks from the next 4 bytes
            char Masks[4];
            BytesRead = read(Websocket->Socket, Masks, 4);
            if (BytesRead < 0) serverFail("wtf, we failed trying to read the masks");

            int TmpPayloadLen = PayloadLen;
            int j = 0;
            while (true) {
              if (TmpPayloadLen < MAX_USER_INPUT_LEN) {
                BytesRead = read(Websocket->Socket, UserInput, TmpPayloadLen);
              } else {
                BytesRead = read(Websocket->Socket, UserInput, MAX_USER_INPUT_LEN);
              }
              if (BytesRead < 0) serverFail("wtf, we failed trying to read the actual payload");

              for (int i = 0; i < BytesRead; i++) {
                UserInput[i] = (UserInput[i] ^ Masks[j % 4]);
                j++;
              }

              TmpPayloadLen = TmpPayloadLen - BytesRead;

              if (j == PayloadLen) break;
            }

            // UserInput now contains the unmasked input string

            for (uint i = 0; i < MAX_PLAYERS; i++) {
              if (ServerMemory.World->Inputs[i].Id == 0) {
                copyUString((uchar*)UserInput, ServerMemory.World->Inputs[i].Text, MAX_USER_INPUT_LEN);
                ServerMemory.World->Inputs[i].Id = Websocket->Player->Id;
                i = MAX_PLAYERS;
              }
            }
          }
        }
      }
    }
  }
}

internal void * writeWebsocketConnections(void *arg) {
  while (true) {
    pthread_mutex_lock( &(ServerMemory.World->HasUpdatedMutex) );
    pthread_cond_wait( &(ServerMemory.World->HasUpdated), &(ServerMemory.World->HasUpdatedMutex) );

    for (int i = 0; i < ServerMemory.WebsocketCount; i++) {
      websocket_connection * Websocket = &(ServerMemory.WebsocketConnections[i]);
      if (Websocket->Socket != 0) {
        // send updated gamestate if the world is updated
        uchar * Message = (uchar *)printVisibleWorldState(ServerMemory.World, Websocket->Player->Name);
        writeAndFreeToWebsocket(Websocket->Socket, Message);
      }
    }

    pthread_mutex_unlock( &(ServerMemory.World->HasUpdatedMutex) );
  }
}

internal void handleWebsocketStart(int Socket, time_t StartTime) {
  // immediately respond with appropriate handshake headers
  transmitMessageOverSocket(Socket, (char*)HEADER_WEBSOCKET, false);
  transmitMessageOverSocket(Socket, (char*)ServerMemory.WebSocketKeyResponse, true);

  // immediately send the gamestate
  uchar * Message = (uchar *)printVisibleWorldState(ServerMemory.World, (char*)ServerMemory.CookieUsername);
  writeAndFreeToWebsocket(Socket, Message);

  // set socket flags
  setSocketNonBlocking(Socket);

  // add this websocket to the list
  bool InQueue = false;
  for (int i = 0; i < ServerMemory.WebsocketCount; i++) {
    if (ServerMemory.WebsocketConnections[i].Socket == 0) {
      ServerMemory.WebsocketConnections[i].StartedAt = StartTime;
      ServerMemory.WebsocketConnections[i].Player = findPlayer(ServerMemory.World, (char*)ServerMemory.CookieUsername);
      ServerMemory.WebsocketConnections[i].Socket = Socket;

      InQueue = true;
      i = ServerMemory.WebsocketCount; // break out of loop
    }
  }
  if (!InQueue) serverFail("Error adding Websocket connection to Queue\n");
}

internal void startServer(int Port, int ThreadCount, world *World) {
  timeval StartTime;
  timeval EndTime;

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
  if( (ServerMemory.WebSocketKey = (uchar *)malloc(sizeof(uchar) * MAX_HEADER_VALUE_SIZE)) == NULL) {
    serverFail("Error allocating WebSocketKey memory\n");
  }
  ServerMemory.StreamCount = ThreadCount;
  if( (ServerMemory.Streams = (streaming_request *)malloc(sizeof(streaming_request) * ThreadCount)) == NULL) {
    serverFail("Error allocating Streams memory\n");
  }
  ServerMemory.World = World;
  ServerMemory.WebsocketCount = ThreadCount;
  if( (ServerMemory.WebsocketConnections = (websocket_connection *)malloc(sizeof(websocket_connection) * ThreadCount)) == NULL) {
    serverFail("Error allocating WebsocketCount memory\n");
  }

  // spin off thread for responding to slow requests (either http or websockets)
  pthread_t HTTPThread;
  pthread_t WebsocketThread;
  pthread_t WebsocketWriteThread;
  for (int i = 0; i < ServerMemory.StreamCount; i++) {
    ServerMemory.Streams[i] = (streaming_request){ 0 }; // clearing all to 0
    ServerMemory.WebsocketConnections[i] = (websocket_connection){ 0 }; // clearing all to 0
  }
  if (pthread_create(&HTTPThread, NULL, updateLongRunningHTTPStreams, NULL)) {
    serverFail("Error creating thread\n");
  }
  if (pthread_create(&WebsocketThread, NULL, readWebsocketConnections, NULL)) {
    serverFail("Error creating thread\n");
  }
  if (pthread_create(&WebsocketThread, NULL, writeWebsocketConnections, NULL)) {
    serverFail("Error creating thread\n");
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
    
  printf("server starting at http://localhost:%d/ with %d max-websocket connections\n", Port, ThreadCount);
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

    clearUString(ServerMemory.WebSocketKeyResponse, WEBSOCKET_KEY_LEN);

    // parse the header from the connection
    serverReadHeaderFromSocket(ConnectionSocket);

    if (ServerMemory.WebSocketKeyResponse[0] != '\0') {
      handleWebsocketStart(ConnectionSocket, StartTime.tv_sec);
      continue;
    }
    http_request RequestDetails = parseRequest(ServerMemory.Header);

    // start to respond, with header
    const char * ContentType; // detect ContentType
    if (RequestDetails.Filename != NULL) {
      if (RequestDetails.IsPng) {
        ContentType = CONTENT_PNG;
      } else {
        ContentType = "text/html";
      }
    } else if (RequestDetails.Route && RequestDetails.Route->Slow) {
      ContentType = "text/plain";
    } else {
      ContentType = CONTENT_JSON;
    }
    printResponseHeader(ConnectionSocket, RequestDetails.ReturnCode, ContentType);

    // pass slow routes off to thread for handling
    if (RequestDetails.Route && RequestDetails.Route->Slow) {
      bool InQueue = false;
      while(!InQueue) {
        for (int i = 0; i < ServerMemory.StreamCount; i++) {
          if (ServerMemory.Streams[i].ConnectionSocket == 0) {
            ServerMemory.Streams[i].Request = RequestDetails;
            ServerMemory.Streams[i].ConnectionSocket = ConnectionSocket;
            ServerMemory.Streams[i].StartedAt = StartTime.tv_sec;

            InQueue = true;
            i = ServerMemory.StreamCount; // break out of loop
          }
        }
      }
    } else { // immediately handle fast routes
      if (RequestDetails.Filename != NULL) {
        printResponseFile(ConnectionSocket, RequestDetails.Filename);
      } else {
        // get the content from the RequestDetails.Route.FnPtr
        char * ResponseContent = (RequestDetails.Route->FnPtr)(World, RequestDetails.Query);
        transmitMessageOverSocket(ConnectionSocket, ResponseContent, true);
        free(ResponseContent);
      }

      gettimeofday(&EndTime,NULL);
      printf("served %d in %d usec\n", RequestDetails.ReturnCode, EndTime.tv_usec - StartTime.tv_usec);
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

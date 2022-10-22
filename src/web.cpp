internal void parseUsernameFromQuery(char* Query, char Name[32]) {
  Name[0] = '\0';
  if (Query == NULL) {
    return;
  }
  int QuerySize = strlen(Query);
  bool NameFound = false;
  int NameIndex = 0;
  for (int i=0; i< QuerySize; i++) {
    if (NameFound) {
      if (Query[i] == '&') {
        NameFound = false;
      } else {
        Name[NameIndex] = Query[i];
        NameIndex++;
      }
    }
    if (Query[i] == 'u' && i < QuerySize && Query[i+1] == '=') {
      i++;
      NameFound = true;
    }
  }
  Name[NameIndex] = '\0';
}

internal char parseCharFromQuery(char* Query, char Param[32]) {
  if (Query == NULL) {
    return 0;
  }
  int QuerySize = strlen(Query);
  int ParamLen = strlen(Param);
  bool ParamFound = false;
  for (int i=0; i< QuerySize; i++) {
    if (ParamFound) {
      return Query[i];
    } else {
      bool AllMatched = true;
      for (int j =0; j< ParamLen; j++) {
        if (Query[i+j] != Param[j]) {
          AllMatched = false;
        }
      }
      if (AllMatched) {
        i += ParamLen - 1;
        ParamFound = true;
      }
    }
  }
  return 0; // never found it
}

internal ulong parseUnixTimeFromQuery(char* Query) {
  if (Query == NULL) {
    return 0;
  }
  int QuerySize = strlen(Query);
  char Param[32] = "last_known=";
  int ParamLen = strlen(Param);
  bool ParamFound = false;
  ulong UnixTime = 0;
  for (int i=0; i< QuerySize; i++) {
    if (ParamFound) {
      if (Query[i] == '&') {
        ParamFound = false;
        i = QuerySize; // break loop
      } else {
        UnixTime = (UnixTime * 10) + (Query[i] - '0');
      }
    } else {
      bool AllMatched = true;
      for (int j =0; j< ParamLen; j++) {
        if (Query[i+j] != Param[j]) {
          AllMatched = false;
        }
      }
      if (AllMatched) {
        i += ParamLen - 1;
        ParamFound = true;
      }
    }
  }
  return UnixTime;
}

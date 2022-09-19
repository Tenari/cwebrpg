internal void parseUsernameFromQuery(char* Query, char Name[32]) {
  Name[0] = '\0';
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
    if (Query[i] == 'u' && Query[i+1] == '=') {
      i++;
      NameFound = true;
    }
  }
  Name[NameIndex] = '\0';
}

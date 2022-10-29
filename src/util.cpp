#define lengthOf(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Num) ((Num) * 1024)
#define Megabytes(Num) (Kilobytes(Num) * 1024)
// you'll have to use this one like Gigabytes((ulong)4) or else the 32bit int will overflow
#define Gigabytes(Num) (Megabytes(Num) * 1024)

#define assert(Thing) if (!(Thing)) {*(int *)0 = 0;}

#define MAX_USER_INPUT_LEN 32

internal void clearString(char *str, int len) {
  for (int i = 0; i < len; i++) {
    str[i] = '\0';
  }
}

internal void clearUString(uchar *str, int len) {
  for (int i = 0; i < len; i++) {
    str[i] = '\0';
  }
}

// copy str into str2
internal void copyUString(uchar *str, uchar *str2, int len) {
  for (int i = 0; i < len; i++) {
    str2[i] = str[i];
  }
}

internal void appendToString(char *str, char *str2) {
  int Len = strlen(str);
  int Len2 = strlen(str2);
  int i;
  for (i = 0; i < Len2; i++) {
    str[Len+i] = str2[i];
  }
  str[Len+i] = '\0';
}

internal void appendToUString(uchar *str, uchar *str2) {
  appendToString((char*)str, (char*)str2);
}

internal int countString(char *str) {
  bool NullFound = false;
  int i = 0;
  while(!NullFound) {
    if (str[i] == '\0') return i;
    i++;
  }
  return 0;
}

internal int countUString(uchar *str) {
  return countString((char*)str);
}

// true if ALL of str2 matches the beginning of str1
internal bool stringsMatch(char*str1, char*str2) {
  int Len2 = strlen(str2);
  bool Matches = true;
  for (int i = 0; i < Len2; i++) {
    if (str1[i] != str2[i]) {
      Matches = false;
      i = Len2;
    }
  }
  return Matches;
}

internal int parseIntFromCharStar(char* Str, int Len) {
  int Result = 0;
  for (int i=0; i < Len; i++) {
    if (Str[i] == '\0' || Str[i] == ' ' || Str[i] > '9' || Str[i] < '0') {
      break;
    }
    Result = (Result * 10) + (Str[i] - '0');
  }
  return Result;
}

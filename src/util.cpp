#define lengthOf(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Num) ((Num) * 1024)
#define Megabytes(Num) (Kilobytes(Num) * 1024)
// you'll have to use this one like Gigabytes((ulong)4) or else the 32bit int will overflow
#define Gigabytes(Num) (Megabytes(Num) * 1024)

#define assert(Thing) if (!(Thing)) {*(int *)0 = 0;}

internal void clearString(char *str, int len) {
  for (int i = 0; i < len; i++) {
    str[i] = '\0';
  }
}

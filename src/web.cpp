internal void parseChar32FromQuery(char* Query, char Result[32], char Param[32]) {
  Result[0] = '\0';
  if (Query == NULL) {
    return;
  }
  int QuerySize = strlen(Query);
  int ParamLen = strlen(Param);
  bool ParamFound = false;
  int ResultIndex = 0;
  for (int i=0; i< QuerySize; i++) {
    if (ParamFound) {
      if (Query[i] == '&') {
        ParamFound = false;
      } else {
        Result[ResultIndex] = Query[i];
        ResultIndex++;
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
  Result[ResultIndex] = '\0';
}

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

internal uint parseIntFromQuery(char* Query, char Param[32]) {
  if (Query == NULL) {
    return 0;
  }
  int QuerySize = strlen(Query);
  int ParamLen = strlen(Param);
  bool ParamFound = false;
  uint Result = 0;
  for (int i=0; i< QuerySize; i++) {
    if (ParamFound) {
      if (Query[i] == '&' || Query[i] == '\n') {
        ParamFound = false;
        i = QuerySize; // break loop
      } else {
        Result = (Result * 10) + (Query[i] - '0');
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
  return Result; // never found it
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

global_variable const uchar base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out: a 256-long uchar array where the result is placed
 * Returns: 1 on success or -1 on failure
 */
internal int base64Encode(const uchar *src, size_t len, uchar out[256]) {
	uchar *pos;
	const uchar *end, *in; 
	size_t olen;
	int line_len;

	olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen += olen / 72; /* line feeds */
	olen++; /* nul termination */
	if (olen < len)
		return -1; /* integer overflow */
  if (olen > 256)
    return -1; /* size not supported */

	end = src + len;
	in = src;
	pos = out;
	line_len = 0;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
			*pos++ = '\n';
			line_len = 0;
		}
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) |
					      (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
		line_len += 4;
	}

	if (line_len)
		*pos++ = '\n';

	*pos = '\0';
  return 1;
}

internal void parseUsernameFromCookie(char* Cookie, uchar Name[32]) {
  int Size = strlen(Cookie);
  bool NameFound = false;
  int NameIndex = 0;
  uchar *Trigger = (uchar*)"username=";
  int TriggerLen = 9;

  for (int i=0; i< Size; i++) {
    if (NameFound) {
      if (Cookie[i] == ';' || Cookie[i] == ' ' || Cookie[i] == '\n' || Cookie[i] == '\r') {
        NameFound = false;
      } else {
        Name[NameIndex] = Cookie[i];
        NameIndex++;
      }
    } else {
      bool AllMatched = true;
      for (int j =0; j< TriggerLen; j++) {
        if (Cookie[i+j] != Trigger[j]) {
          AllMatched = false;
        }
      }
      if (AllMatched) {
        i += TriggerLen - 1;
        NameFound = true;
      }
    }
  }
  Name[NameIndex] = '\0';
}


// header value is placed in Result
internal void generateWebsocketAcceptHeader(uchar Key[256], uchar Result[256]) {
  int Len = 256;
  uchar temp[Len];
  clearString((char*)Result, Len);
  clearString((char*)temp, Len);
  uchar MagicAppendStr[37] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  appendToUString(Key, MagicAppendStr);

  sha1digest(temp, NULL, Key, countUString(Key));

  base64Encode(temp, countUString(temp), Result);
}

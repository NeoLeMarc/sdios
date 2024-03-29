int strcmp(const char *s1, const char *s2);
int strpos(char * string, char search);
//char * strcpy (char *s1, const char *s2);


/* fast strcmp -- Copyright (C) 2003 Thomas M. Ogrisegg <tom@hi-tek.fnord.at> */
int strcmp (const char *s1, const char *s2)
{
    while (*s1 && *s1 == *s2)
        s1++, s2++;
    return (*s1 - *s2);
}

/* fast strcpy -- Copyright (C) 2003 Thomas M. Ogrisegg <tom@hi-tek.fnord.at> */
/*
char * strcpy (char *s1, const char *s2) {
    char           *res = s1;
    while ((*s1++ = *s2++));
    return (res);
}
*/

// find position of char in string
int strpos (char * string, char search){
    int i = 0;
    while( (*string != '\n') && (*string != '\0'))
        if(*(string++) == search)
            return i;
        else
            i++;
    return -1;
}

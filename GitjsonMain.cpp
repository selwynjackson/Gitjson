#pragma hdrstop
#pragma argsused

#ifdef _WIN32
#include <tchar.h>
#else
  typedef char _TCHAR;
  #define _tmain main
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *getFileExt(char *s)
{
	int i;

	for (i = strlen(s) - 1; i; --i) {
		if (s[i] == '.')
            return s + i;
	}
    return "";
}
void changeFileExt(char *s, const char *t)
{
	int i;

	for (i = strlen(s) - 1; i; --i) {
		if (s[i] == '.')
			break;
	}
    if (s[i] != '.' || (i <= 2 && s[0] == '.')) {
//        strcat(s, ".");
        strcat(s, t);
    }
    else {
    	strcpy (s+i,t);
    }
}
char *extractFileName(char *s)
{
	int i;

	for (i = strlen(s) - 1; i; --i) {
		if (s[i] == '\\') {
            ++i;
			break;
        }
	}
    return s+i;
}

int getLabel(char *filename, char *label)
{
    FILE *fp;
    int i;
    char s[512];
    int ret = -1;
    char *p;

    if ((fp = fopen(filename, "r")) != 0) {
        for (i = 0; i < 20; ++i) {
            if (fgets(s, sizeof(s), fp) == 0) {
                break;
            }
            if ((p = strstr(s, "\"label\": \"")) != 0) {
                strcpy(label, p+10);
                if ((p = strchr(label, '\"')) != 0) {
                    *p = 0;
                    ret = 0;
                    break;
                }
            }
        }
    }
    return ret;
}




int _tmain(int argc, _TCHAR* argv[]) 
{
    if (argc < 2) {
        printf("Gitjson - Version 1.0 - Convert NodeRED json for use by Git and vica versa\n");
        printf("Text \'\\n\' is converted to \'CRLF >> tab tab\'\n");
        printf("Use: Gitjson <sourcefile.json> [destination file.gitjson] to convert\n");
        printf("     Gitjson <sourcefile.gitjson> [destination file.json] to unconvert\n");
        printf("     Gitjson <flows*.json> <destination.gitjson> where destination is extracted from file at the first label\n");
        printf("Written by Selwyn Jackson\n\n");
        return 0;
    }

    char src[512];
    char dest[512];
    FILE *sp, *dp;
    int ret = 0;
    unsigned int fileSize;
    bool convert;   // 1 to convert, 0 to unconvert

    strcpy (src, argv[1]);
    if ((sp = fopen(src, "r")) == 0) {
        printf("Cannot open source file %s", src);
        ret = 1;
    }
    else {
        fseek(sp, 0, SEEK_END);
        fileSize = ftell(sp);
        fclose(sp);
        convert = !stricmp(getFileExt(src), ".json");
    }
    if (ret == 0) {
        if (argc >= 3) {
            strcpy(dest, argv[2]);
        }
        else if (convert && strncmp(src, "flows", 5) == 0 && getLabel(src, dest) == 0) {
            changeFileExt(dest, ".gitjson");
        }
        else {
            strcpy(dest, src);
            changeFileExt(dest, convert ? ".gitjson" : ".json");
        }
        if ((dp = fopen(dest, "w")) == 0) {
            printf("Cannot open destination file %s", dest);
            ret = 2;
        }
        else {
            fclose(dp);
        }
    }
    if (ret == 0) {
        printf("%s %s to %s\n", convert ? "Convert" : "Unconvert", src, dest);
    }

	return 0;
}

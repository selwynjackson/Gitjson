#pragma hdrstop
#pragma argsused

#ifdef _WIN32
#include <tchar.h>
#else
  typedef char _TCHAR;
  #define _tmain main
#endif

#include <windows.h>
#include <winreg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>

int convert(FILE *dp, FILE *sp, const char *tag, const char *value);

unsigned int fileSize;

//=========================================================================
//  Description:    Get a pointer to the file extension
//  Parameters:     file name
//  Returns:        Pointer
//=========================================================================
char *getFileExt(char *s)
{
	int i;

	for (i = strlen(s) - 1; i > ((s[0] == '.') ? 2 : 1); --i) {
		if (s[i] == '.')
            return s + i;
	}
    return s + strlen(s);
}

//=========================================================================
//  Description:    Change a file extension
//  Parameters:     filename
//                  extension
//  Returns:        None
//=========================================================================
void changeFileExt(char *s, const char *t)
{
    s = getFileExt(s);
    strcpy (s, t);
}

//=========================================================================
//  Description:    Extract filename from full path
//  Parameters:     path
//  Returns:        filename
//=========================================================================
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

//=========================================================================
//  Description:    Retrive the first label field in json file
//  Parameters:     filename
//                  pointer for label
//  Returns:        Non-zero for an error
//=========================================================================
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


//=========================================================================
//  Description:    Save a registry key
//  Parameters:     name
//                  value
//  Returns:        Non-zero for an error
//=========================================================================
int saveRegistryKey(const char *name, char *value)
{
    int ret = 1;
    HKEY keySoftware, keyExID, keySub;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE", 0, KEY_ALL_ACCESS, &keySoftware) == ERROR_SUCCESS) {
        if (RegCreateKey(keySoftware, "IDwiz", &keyExID) == ERROR_SUCCESS) {      // create a sub key
            if (RegCreateKey(keyExID, "Gitjson", &keySub) == ERROR_SUCCESS) {      // create a sub key
                if (RegSetValueEx(keySub, name, 0, REG_SZ, (BYTE*)value, strlen(value)) == ERROR_SUCCESS) {  // create a sub key with a value
                    ret = 0;
                }
                RegCloseKey(keySub);
            }
            RegCloseKey(keyExID);
        }
        RegCloseKey(keySoftware);
    }
    return ret;
}

//=========================================================================
//  Description:    Get key value from registry
//  Parameters:     key - name of key
//					def - default value
//  Returns:        Value
//=========================================================================
const char *getRegistryKey(const char *key, const char *def)
{
    static char value[512];
    HKEY handle;
    DWORD size = sizeof(value);
    BOOL found = 0;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\IDwiz\\Gitjson", 0, KEY_READ, &handle) == ERROR_SUCCESS) {
        if (RegQueryValueEx(handle, key, 0, 0, (BYTE*)value, &size) == ERROR_SUCCESS) {
            value[size] = 0;    // force an end for empty value
            found = 1;
        }
		RegCloseKey(handle);
    }
    if (found) {
    	return value;
    }
    return def;
}


//=========================================================================
//  Description:    Main
//  Parameters:     argc, argv
//  Returns:        Exit code
//=========================================================================
int _tmain(int argc, _TCHAR* argv[])
{
    if (argc < 2) {
        printf("Gitjson - Version 1.0 - Convert NodeRED json for use by Git and vica versa\n");
        printf("Text \'\\n\' is converted to \'LF >> tab tab\'\n");
        printf("Use: Gitjson <sourcefile.json> <destinationfile>\n");
        printf("     Gitjson <sourcefile.json> - to convert to sourcefile.gitjson\n");
        printf("     Gitjson <sourcefile.gitjson> - to unconvert to sourcefile.json\n");
        printf("     Gitjson <flows*.json> to convert to <project><label.gitjson> - where label is extracted from json file\n");
        printf("     Gitjson /p <project> - to set project directory\n");
        printf("     Gitjson /p - to clear project directory\n");
        printf("Project is currently set to \'%s\'\n", getRegistryKey("project", ""));
        printf("Written by Selwyn Jackson\n\n");
        return 0;
    }

    char src[512];
    char dest[512];
    char label[100];
    FILE *sp = 0;
    FILE *dp = 0;
    int ret = 0;
    bool flag;   // 1 to convert, 0 to unconvert

    // set project path
    if (stricmp(argv[1], "/p") == 0) {
        dest[0] = 0;
        if (argc >= 3) {
            strcpy(dest, argv[2]);
        }
        saveRegistryKey("project", dest);
        printf("Gitjson project set to \"%s\"\n", dest);
        return 0;
    }

    strcpy (src, argv[1]);
    if ((sp = fopen(src, "r")) == 0) {
        printf("Cannot open source file %s", src);
        ret = 1;
    }
    else {
        fseek(sp, 0, SEEK_END);
        fileSize = ftell(sp);
        fseek(sp, 0, SEEK_SET);
        flag = !stricmp(getFileExt(src), ".json");
    }
    if (ret == 0) {
        if (argc >= 3) {
            strcpy(dest, argv[2]);
        }
        else if (flag && strncmp(extractFileName(src), "flows", 5) == 0 && getLabel(src, label) == 0) {
            strcpy(dest, getRegistryKey("project", ""));
            strcpy(dest + strlen(dest), label);
            changeFileExt(dest, ".gitjson");
        }
        else {
            strcpy(dest, src);
            changeFileExt(dest, flag ? ".gitjson" : ".json");
        }
        if ((dp = fopen(dest, "w")) == 0) {
            printf("Cannot open destination file %s", dest);
            ret = 2;
        }
    }
    if (ret == 0) {
        printf("%s %s to %s\n", flag ? "Convert" : "Unconvert", src, dest);
        if (flag) {
            ret = convert(dp, sp, "\\n", "\n>>\t\t");
        }
        else {
            ret = convert(dp, sp, "\n>>\t\t", "\\n");
        }
        if (ret) {
            printf("ERROR %d processing file\n", ret);
        }
    }
    if (sp) {
        fclose(sp);
    }
    if (dp) {
        fclose(dp);
    }
	return ret;
}


//=========================================================================
//  Description:    Convert file
//  Parameters:     destination, source file pointers
//                  tag is replaced by value
//  Returns:        Non-zero for an error
//=========================================================================
int convert(FILE *dp, FILE *sp, const char *tag, const char *value)
{
    #define BUFFER_SIZE 512
    #define REPLACE_LENGTH 2
    char *buffer = new char[BUFFER_SIZE+1];
    if (buffer == 0) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    int length = 0;
    int i;
    int end = 0;
    char *p;
    for (; !end;) {
        if (length <= strlen(tag)) {
            i = fread(buffer + length, 1, BUFFER_SIZE - length, sp);
            if (i == 0) {
                end = 1;
            }
            length += i;
            buffer[length] = 0;
        }
        if ((p = strstr(buffer, tag)) != 0) {
            i = p - buffer;
            fwrite(buffer, 1, i, dp);
            fwrite(value, 1, strlen(value), dp);
            i += strlen(tag);
            strcpy(buffer, buffer + i);
            length -= i;
        }
        else {
            i = length - strlen(tag);
            fwrite(buffer, 1, i, dp);
            strcpy(buffer, buffer + i);
            length -= i;
        }
    }
    if (length > 0) {
        fwrite(buffer, 1, length, dp);
    }
    delete [] buffer;

    return 0;
}



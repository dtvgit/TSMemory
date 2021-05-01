/*******************************************************************
                   filename treating module
 *******************************************************************/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "filename.h"

#ifdef _WIN32
#define PATH_DELIMITER '\\'
#else
#define PATH_DELIMITER '/'
#endif

int cut_suffix(char *filepath)
{
	int i,n;
	
	n = strlen(filepath);

	for(i=n-1;i>=0;i--){
		if(filepath[i] == '.'){
			filepath[i] = '\0';
			return 1;
		}else if(filepath[i] == PATH_DELIMITER){
			return 0;
		}
	}

	return 0;
}

int check_suffix(char *filepath, char *suffix)
{
	int i;
	char *sfx;

	sfx = read_suffix(filepath);

	i = 0;
	while(sfx[i] && suffix[i]){
		if( toupper(sfx[i]) != toupper(suffix[i]) ){
			return 0;
		}
		i += 1;
	}

	if(sfx[i] == suffix[i]){
		return 1;
	}
	
	return 0;
}

char *read_suffix(char *filepath)
{
	int i,n;
	
	n = strlen(filepath);

	for(i=n-1;i>=0;i--){
		if(filepath[i] == '.'){
			return filepath + i;
		}else if(filepath[i] == PATH_DELIMITER){
			return filepath + n;
		}
	}

	return filepath + n;
}


#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

char DEFAULT_SHELL[] = "/bin/sh";
char SG_PATH[] = "/usr/bin/sg";
char * DEFAULT_ARGV[] = {DEFAULT_SHELL,NULL};

char ESCAPED_QUOTE[] = "\\'";

char * shellescapeargv(char **argv) {
	size_t retbufsize = 1;
	size_t ESCAPED_QUOTE_len = sizeof(ESCAPED_QUOTE) - 1;
	size_t copysize;
	char * param;
	char * nextparam;
	char ** scanargv = argv;
	char * escaped;
	char * retescaped;
	
	/* First, compute the size beforehand */
	while (*scanargv != NULL) {
		param = *scanargv;
		/* Size of the param */
		retbufsize += strlen(param);
		
		/* One byte for each needed space to separate params */
		if(argv!=scanargv) {
			retbufsize += 1;
		}
		
		/* All the escaped single quotes */
		while((param = strchr(param, '\''))!= NULL) {
			retbufsize += ESCAPED_QUOTE_len - 1;
			/* Skip the found single quote */
			param++;
		}
		scanargv++;
	}

	/* Reserve the memory block */
	retescaped = (char *)malloc(retbufsize);
	retescaped[retbufsize-1] = '\0';
	escaped = retescaped;

	/* And now fill it in */
	scanargv = argv;
	while (*scanargv != NULL) {
		/* One byte for each needed space to separate params */
		if(argv!=scanargv) {
			escaped[0] = ' ';
			escaped++;
		}

		param = *scanargv;
		
		/* All the escaped single quotes */
		while((nextparam = strchr(param, '\''))!= NULL) {
			copysize = nextparam - param;
			if (copysize > 0) {
				memcpy(escaped, param, copysize);
				escaped += copysize;
			}
			memcpy(escaped, ESCAPED_QUOTE, ESCAPED_QUOTE_len);
			escaped += ESCAPED_QUOTE_len;
			param = nextparam + 1;
		}
		/* And the rest */
		copysize = strlen(param);
		if(copysize > 0) {
			memcpy(escaped, param, copysize);
			escaped += copysize;
		}
		scanargv++;
	}

	return retescaped;
}

int main(int argc, char ** argv)
{
	uid_t user_uid;
	gid_t user_gid;
	uid_t effective_uid;
	gid_t effective_gid;
	struct stat statbuf;
	int statret;
	int retval;
	char cmdbuffer[4096];
	char ** newargv;
	char * pathval;
	char * pathtoken;
	char * pathexe;
	
	user_uid = getuid();
	effective_uid = geteuid();

	statret = -1;
	if(user_uid != effective_uid && effective_uid == 0) {
		/* Elevate privileges */
		user_gid = getgid();
		effective_gid = getegid();
		retval = setuid(effective_uid);
		if(retval!=0) {
			perror("setuid");
		}

		/* Create group */
		snprintf(cmdbuffer, sizeof(cmdbuffer), "groupadd -g %d -o dyngroup", user_gid);
		retval = system(cmdbuffer);
		if(retval!=0) {
			perror("groupadd");
		}

		/* Create user */
		snprintf(cmdbuffer, sizeof(cmdbuffer), "useradd -g %d -u %d -o -m dynuser", user_gid, user_uid);
		retval = system(cmdbuffer);
		if(retval!=0) {
			perror("useradd");
		}
		
		/* Peek the docker group details */
		statret = stat("/run/docker.sock", &statbuf);
		if(statret == 0) {
			snprintf(cmdbuffer, sizeof(cmdbuffer), "groupadd -g %d -o dyndocker", statbuf.st_gid);
			retval = system(cmdbuffer);
			if(retval!=0) {
				perror("groupadd docker");
			}
			snprintf(cmdbuffer, sizeof(cmdbuffer), "usermod -a -G %d dynuser", statbuf.st_gid);
			retval = system(cmdbuffer);
			if(retval!=0) {
				perror("usermod");
			}
		/*
		} else {
			perror("stat");
		*/
		}
		
		/* Drop privileges */
		setuid(user_uid);

		setegid(user_gid);
		seteuid(user_uid);
	}
	
	if(argc > 1) {
		if(statret == 0) {
			newargv = malloc(sizeof(char*)*4);
			newargv[0] = SG_PATH;
			newargv[1] = "dyndocker";
			newargv[2] = shellescapeargv(argv+1);
			newargv[3] = NULL;
			execv(SG_PATH, newargv);
			perror("QUACK dyndocker!");
			while (*newargv != NULL) {
			    printf("SEE %s\n", *newargv);
			    newargv++;
			}
		} else {
			/*
			 *  Check whether it is reachable
			 */
			statret = stat(argv[1], &statbuf);
			pathexe = NULL;
			if(statret == 0) {
				pathexe = argv[1];
			} else if(argv[1][0] != '/') {
				/* Now, iterate over PATH */
				pathval=getenv("PATH");
				if(pathval != NULL) {
					pathval = strdup(pathval);
					for(pathtoken=strtok(pathval, ":"); pathtoken; pathtoken=strtok(NULL, ":")) {
						snprintf(cmdbuffer, sizeof(cmdbuffer), "%s/%s", pathtoken, argv[1]);
						statret = stat(cmdbuffer, &statbuf);
						if(statret == 0) {
							pathexe = cmdbuffer;
							break;
						}
					}
					free(pathval);
				}
			}
			if(pathexe!=NULL) {
				newargv = malloc(sizeof(char*)*argc);
				memcpy(newargv, argv + 1, sizeof(char*)*(argc-1));
				newargv[argc-1] = NULL;
				newargv[0] = pathexe;
				execv(pathexe, newargv);
			}
			
			perror("QUACK exec!");
			/* Skip this one */
			argv++;
			while (*argv != NULL) {
			    printf("SEE %s\n", *argv);
			    argv++;
			}
		}
	} else {
		execv(DEFAULT_SHELL, DEFAULT_ARGV);
		perror("QUACK default!");
	}
	return 1;
}
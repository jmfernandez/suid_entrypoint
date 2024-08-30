#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

char DEFAULT_SHELL[] = "/bin/sh";
char SG_PATH[] = "/usr/bin/sg";
char * DEFAULT_ARGV[] = {DEFAULT_SHELL,NULL};

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
			newargv = malloc(sizeof(char*)*(argc+2));
			memcpy(newargv+2, argv+1, sizeof(char*)*(argc-1));
			newargv[argc+1] = NULL;
			newargv[0] = SG_PATH;
			newargv[1] = "dyndocker";
			execv(SG_PATH, newargv);
			perror("QUACK dyndocker!");
		} else {
			/*
			 *  Check whether it is reachable
			 */
			statret = stat(argv[1], &statbuf);
			if(statret == 0) {
				pathexe = argv[1];
			} else {
				pathexe = NULL;
				/* Now, iterate over PATH */
				pathval=getenv("PATH");
				if(pathval != NULL) {
					for(pathtoken=strtok(pathval, ":"); pathtoken; pathtoken=strtok(pathval, ":")) {
						snprintf(cmdbuffer, sizeof(cmdbuffer), "%s/%s", pathtoken, argv[1]);
						statret = stat(cmdbuffer, &statbuf);
						if(statret == 0) {
							pathexe = cmdbuffer;
							break;
						}
					}
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
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <lastlog.h>
#include <pwd.h>
#include <stdlib.h>
#include <time.h>

long log_pid[100] = {0,};
int outlog_cnt = 0;

struct utmp last_log={0.};
int isLastLog = 0;


void kill_tmp(int ind, char *who, char* terminal, char* datetime, char* newUsername, char* newTerminal, char* newDatetime){
	struct utmp utmp_ent;
	int deleted_log = 0;
	int size;
	char *tmp_data;
	char *p;
	char* name;
	if (ind==0){
		name = "/var/log/wtmp";
	} else {
		name = "/var/run/utmp";
	}
	
	int i;
	
	FILE *fp = fopen(name, "r");

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);

	tmp_data = (char *)malloc(size+1);
	memset(tmp_data, 0, size+1);
	fseek(fp, 0, SEEK_SET);
	fread(tmp_data, size, 1, fp);
	fclose(fp);

	p = tmp_data;
	fp=fopen(name,"w");

	if (strlen(newUsername) != 0){
		long replaced_pid = -1;
		while (*p != 0) {

			int isRemoved = 0;
			memcpy(&utmp_ent, p, sizeof(utmp_ent));

			if(strlen(utmp_ent.ut_name) == strlen(who) && 
				strncmp(utmp_ent.ut_name, who, strlen(who)) == 0 &&
				strncmp(utmp_ent.ut_line, terminal, strlen(terminal)) == 0 &&
				strlen(utmp_ent.ut_line) == strlen(terminal)){
				
				char buffer[20] = {0, };
				time_t t = utmp_ent.ut_tv.tv_sec;
				struct tm *timeinfo;
				timeinfo = localtime(&t);
				strftime(buffer, 20, "%m%d%y:%H%M", timeinfo);

				if(strncmp(buffer, datetime, 11)==0){
					struct tm tm;
					strptime(newDatetime, "%m%d%y:%H%M", &tm);
					tm.tm_hour -= 1;
					time_t new_t = mktime(&tm);
					utmp_ent.ut_tv.tv_sec = new_t;
					memcpy(utmp_ent.ut_name, newUsername, strlen(newUsername)+1);
					memcpy(utmp_ent.ut_line, newTerminal, strlen(newTerminal)+1);
					replaced_pid = utmp_ent.ut_pid;
				}

			}

			if(utmp_ent.ut_pid == replaced_pid){
				memcpy(utmp_ent.ut_line, newTerminal, strlen(newTerminal)+1);
			}

			if(strlen(utmp_ent.ut_name) == strlen(who) && 
				strncmp(utmp_ent.ut_name, who, strlen(who)) == 0  && ind==0){
				memcpy(&last_log, &utmp_ent, sizeof(utmp_ent));
				isLastLog = 1;
			}

			fwrite( &utmp_ent, sizeof(utmp_ent),1,fp);

			p = p+sizeof(utmp_ent);
		}
	} else if (strlen(terminal) == 0){
		while (*p != 0) {
			memcpy(&utmp_ent, p, sizeof(utmp_ent));
			if(strlen(utmp_ent.ut_name) == strlen(who) && 
				strncmp(utmp_ent.ut_name, who, strlen(who)) == 0){
				log_pid[outlog_cnt] = utmp_ent.ut_pid;
				outlog_cnt++;
				
			} else {
				int isLogoutLog = 0;
				for(i = 0; i < outlog_cnt; i++){
					if(utmp_ent.ut_pid == log_pid[i]){
						isLogoutLog = 1;
						break;
					}
				}
				if(!isLogoutLog){

					if(strlen(utmp_ent.ut_name) == strlen(who) && 
						strncmp(utmp_ent.ut_name, who, strlen(who)) == 0  && ind==0){
						memcpy(&last_log, &utmp_ent, sizeof(utmp_ent));
						isLastLog = 1;
					}

					fwrite(&utmp_ent, sizeof(utmp_ent),1,fp);


				}

			}
			p = p+sizeof(utmp_ent);
		}
	} else {
		while (*p != 0) {
			int isRemoved = 0;
			memcpy(&utmp_ent, p, sizeof(utmp_ent));

			if(strlen(utmp_ent.ut_name) == strlen(who) && 
				strncmp(utmp_ent.ut_name, who, strlen(who)) == 0 &&
				strncmp(utmp_ent.ut_line, terminal, strlen(terminal)) == 0 &&
				strlen(utmp_ent.ut_line) == strlen(terminal)){
				
				char buffer[20] = {0, };
				time_t t = utmp_ent.ut_tv.tv_sec;
				struct tm * timeinfo;
				timeinfo = localtime(&t);
				strftime(buffer, 20, "%m%d%y:%H%M", timeinfo);
				if(strncmp(buffer, datetime, 10)==0){
					isRemoved=1;
					log_pid[outlog_cnt] = utmp_ent.ut_pid;
					outlog_cnt++;
				}

			} else {
				for(i = 0; i < outlog_cnt; i++){
					if(utmp_ent.ut_pid == log_pid[i]){
						isRemoved = 1;
					}
				}
			}
			if (!isRemoved){
				if(strlen(utmp_ent.ut_name) == strlen(who) && 
					strncmp(utmp_ent.ut_name, who, strlen(who)) == 0 && ind==0){
					memcpy(&last_log, &utmp_ent, sizeof(utmp_ent));
					isLastLog = 1;
				}
				fwrite( &utmp_ent, sizeof(utmp_ent),1,fp);
				
				
			}
			p = p+sizeof(utmp_ent);
		}
	}

	fclose(fp);

	free(tmp_data);
}
 
void kill_lastlog(char *who, char* terminal, char* datetime, char* newUsername, char* newTerminal, char* newDatetime){
	struct passwd *pwd;
	struct lastlog newll;
	int fp;


	if ((pwd=getpwnam(who))!=NULL) {

		if ((fp = open("/var/log/lastlog", O_RDWR)) >= 0) {
			lseek(fp, (long)pwd->pw_uid * sizeof (struct lastlog), 0);
			if(isLastLog){
				newll.ll_time = last_log.ut_tv.tv_sec;
				memcpy(newll.ll_line, last_log.ut_line, strlen(last_log.ut_line));
				memcpy(newll.ll_host, last_log.ut_host, strlen(last_log.ut_host));
			} else {

			}
			
			write(fp, (char *)&newll, sizeof( newll ));
			close(fp);
		}

	} else printf("%s: user not found\n",who);
}



void usage(){
	printf("usage: sudo zap -A username\n");
	printf("usage: sudo zap -a username -t tty -d mmddyy:hhmm\n");
	printf("usage: sudo zap -R username newname -t tty pts -d mmddyy:hhmm mmddyy:hhmm\n");
}

int main(int  argc, char *argv[]){
	char username[20] = {0,};
	char terminal[20] = {0,};
	char datetime[20] = {0,};
	char newUsername[20] = {0,};
	char newTerminal[20] = {0,};
	char newDatetime[20] = {0,};

	if (argc < 3) {
		usage();
		return 0;
	}
	if (argc == 3 && strcmp (argv[1], "-A") == 0) {

		kill_tmp(0,argv[2], terminal, datetime, newUsername, newTerminal, newDatetime);
		kill_tmp(1,argv[2], terminal, datetime, newUsername, newTerminal, newDatetime);
		kill_lastlog(argv[2], terminal, datetime, newUsername, newTerminal, newDatetime);

	} else if (argc == 7){
		int c;
		while((c = getopt(argc, argv, "a:t:d:")) != -1) {
			switch(c) {
				case 'a':
					memcpy(username, optarg, strlen(optarg));
					break;
				case 't':
					memcpy(terminal, optarg, strlen(optarg));
					break;
				case 'd':
					memcpy(datetime, optarg, strlen(optarg));
					break;
			}
		}

		if(!strlen(username) || !strlen(terminal) || !strlen(datetime)){
			usage();
			return 0;
		} else {
			kill_tmp(0, username, terminal, datetime, newUsername, newTerminal, newDatetime);
			kill_tmp(1, username, terminal, datetime, newUsername, newTerminal, newDatetime);
			kill_lastlog(username, terminal, datetime, newUsername, newTerminal, newDatetime);
		}
	} else if (argc == 10){
		int c;

		while((c = getopt(argc, argv, "R:t:d:")) != -1) {
			switch(c) {
				case 'R':
					memcpy(username, optarg, strlen(optarg));
					memcpy(newUsername, argv[optind], strlen(argv[optind]));
					break;
				case 't':
					memcpy(terminal, optarg, strlen(optarg));
					memcpy(newTerminal, argv[optind], strlen(argv[optind]));
					break;
				case 'd':
					memcpy(datetime, optarg, strlen(optarg));
					memcpy(newDatetime, argv[optind], strlen(argv[optind]));
					break;
			}
		}
		if(!strlen(username) || !strlen(terminal) || !strlen(datetime)){
			usage();
			return 0;
		} else {
			kill_tmp(0, username, terminal, datetime, newUsername, newTerminal, newDatetime);
			kill_tmp(1, username, terminal, datetime, newUsername, newTerminal, newDatetime );
			kill_lastlog(username, terminal, datetime, newUsername, newTerminal, newDatetime);
		}
	} else{
		usage();
	}
}
